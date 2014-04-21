/* RMHELP.CPP

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Feb-96  Timur Tabi   Creation
*/

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

#include <include.h>

#define INCL_NOPMAPI
#include <os2.h>

#include "malloc.h"
#include "rmhelp.hpp"  // brings in rmcalls.h

/*----------------------------------------------*/
/* GLOBAL VARS FOR RM                           */
/*                                              */
/* RM.LIB needs these declared                  */
/*----------------------------------------------*/
extern "C" PFN             RM_Help               = 0L;  /*VPNP*/
extern "C" PFN             RM_Help0              = 0L;  /*VPNP*/
extern "C" PFN             RM_Help3              = 0L;  /*VPNP*/
extern "C" ULONG           RMFlags               = 0L;  /*VPNP*/

DRIVER::DRIVER(DRIVERSTRUCT *pds)
{
   ASSERT(pds);
   hdriver = 0;

   fRegistered = RMCreateDriver(pds, &hdriver) == RMRC_SUCCESS;
   ASSERT(fRegistered);
}

DRIVER::~DRIVER(void)
{
   if (fRegistered) {
      RMDestroyDriver(hdriver);
      fRegistered = FALSE;
      hdriver = 0;
   }
}

ADAPTER::ADAPTER(ADAPTERSTRUCT *pas, DRIVER *_pdriver)
{
   ASSERT(pas);
   ASSERT(_pdriver);

   hadapter = 0;
   pdriver = _pdriver;

   fRegistered = RMCreateAdapter(pdriver->hdriver,
                    &hadapter, pas, NULL, NULL) == RMRC_SUCCESS;
   ASSERT(fRegistered);
}

ADAPTER::~ADAPTER(void)
{
   if (fRegistered) {
      RMDestroyAdapter(pdriver->hdriver, hadapter);
      fRegistered = FALSE;
      hadapter = 0;
   }
}

int ADAPTER::CheckPort(USHORT usBase, USHORT usNumPorts, USHORT usFlags, USHORT usAddrLines)
{
   HRESOURCE hresource;
   RESOURCESTRUCT resource;

   resource.ResourceType = RS_TYPE_IO;
   resource.Reserved = 0;
   resource.IOResource.BaseIOPort     = usBase;
   resource.IOResource.NumIOPorts     = usNumPorts;
   resource.IOResource.IOFlags        = usFlags;
   resource.IOResource.IOAddressLines = usAddrLines;

   unsigned int f = RMAllocResource(pdriver->hdriver, &hresource, &resource) == RMRC_SUCCESS;
   if (f) {
      APIRET rc = RMDeallocResource(pdriver->hdriver, hresource);
      ASSERT(rc == RMRC_SUCCESS);
   }

   return f;
}

int ADAPTER::CheckIRQ(USHORT usIRQ, USHORT usPCIIRQ, USHORT usFlags)
{
   HRESOURCE hresource;
   RESOURCESTRUCT resource;

   resource.ResourceType = RS_TYPE_IRQ;
   resource.Reserved = 0;
   resource.IRQResource.IRQLevel       = usIRQ;
   resource.IRQResource.PCIIrqPin      = usPCIIRQ;
   resource.IRQResource.IRQFlags       = usFlags;
   resource.IRQResource.Reserved       = 0;
   resource.IRQResource.pfnIntHandler  = NULL;

   unsigned int f = (RMAllocResource(pdriver->hdriver, &hresource, &resource) == RMRC_SUCCESS);
   if (f) {
      APIRET rc = RMDeallocResource(pdriver->hdriver, hresource);
      ASSERT(rc == RMRC_SUCCESS);
   }

   return f;
}

int ADAPTER::add(RESOURCE *pres)
{
   ASSERT(pres);

   if (!pres)
      return FALSE;

   if (pres->padapter || pres->pdevice)
      return FALSE;

   APIRET rc = RMAllocResource(pdriver->hdriver, &pres->hresource, &pres->resource);
   if (rc != RMRC_SUCCESS)
      return FALSE;

   rc = RMModifyResources(pdriver->hdriver, hadapter, RM_MODIFY_ADD, pres->hresource);
   if (rc != RMRC_SUCCESS) {
      RMDeallocResource(pdriver->hdriver, pres->hresource);
      return FALSE;
   }

   pres->padapter = this;
   pres->fRegistered = TRUE;

   return TRUE;
}

DEVICE::DEVICE(DEVICESTRUCT *pds, ADAPTER *_padapter)
{
   ASSERT(pds);
   ASSERT(_padapter);

   hdevice = 0;
   padapter = _padapter;

   presHead = NULL;
   uNumResources = 0;

   static unsigned uDeviceNum = 0;
   static ADJUNCT adj;
   adj.pNextAdj = NULL;
   adj.AdjLength = ADJ_HEADER_SIZE + sizeof(USHORT);
   adj.AdjType = ADJ_DEVICE_NUMBER;
   adj.Device_Number = uDeviceNum++;

   pds->pAdjunctList = &adj;
   fRegistered = RMCreateDevice(padapter->pdriver->hdriver,
                    &hdevice, pds, padapter->hadapter, NULL) == RMRC_SUCCESS;
   ASSERT(fRegistered);
}

DEVICE::~DEVICE(void)
{
   if (fRegistered) {
      RMDestroyDevice(padapter->pdriver->hdriver, hdevice);
      fRegistered = FALSE;
      hdevice = 0;
   }
}

int DEVICE::add(RESOURCE *pres)
{
   ASSERT(pres);

   if (!pres)
      return FALSE;

   if (pres->padapter || pres->pdevice)
      return FALSE;

   APIRET rc = RMAllocResource(padapter->pdriver->hdriver, &pres->hresource, &pres->resource);
   if (rc != RMRC_SUCCESS)
      return FALSE;

   rc = RMModifyResources(padapter->pdriver->hdriver, hdevice, RM_MODIFY_ADD, pres->hresource);
   if (rc != RMRC_SUCCESS) {
      RMDeallocResource(padapter->pdriver->hdriver, pres->hresource);
      return FALSE;
   }

   pres->pdevice = this;
   pres->padapter = padapter;
   pres->fRegistered = TRUE;

   return TRUE;;
}

RESOURCE::RESOURCE(ULONG ulType)
{
   ASSERT(ulType);
   padapter = NULL;
   pdevice = NULL;

   hresource = 0;
   resource.ResourceType = ulType;
   resource.Reserved = 0;
   pnext = NULL;
   fRegistered = FALSE;
}

RESOURCE::~RESOURCE(void)
{
   ASSERT(padapter);
   ASSERT(padapter->pdriver);
   ASSERT(padapter->pdriver->hdriver);

   if (fRegistered) {
      APIRET rc = RMDeallocResource(padapter->pdriver->hdriver, hresource);
      ASSERT(rc == RMRC_SUCCESS);
   }
}

PORT_RESOURCE::PORT_RESOURCE(USHORT usBase, USHORT usNumPorts, USHORT usFlags, USHORT usAddrLines)
   : RESOURCE(RS_TYPE_IO)
{
   resource.IOResource.BaseIOPort     = usBase;
   resource.IOResource.NumIOPorts     = usNumPorts;
   resource.IOResource.IOFlags        = usFlags;
   resource.IOResource.IOAddressLines = usAddrLines;
}

IRQ_RESOURCE::IRQ_RESOURCE(USHORT usIRQ, USHORT usPCIIRQ, USHORT usFlags)
   : RESOURCE(RS_TYPE_IRQ)
{
   resource.IRQResource.IRQLevel       = usIRQ;
   resource.IRQResource.PCIIrqPin      = usPCIIRQ;
   resource.IRQResource.IRQFlags       = usFlags;
   resource.IRQResource.Reserved       = 0;
   resource.IRQResource.pfnIntHandler  = NULL;
}


