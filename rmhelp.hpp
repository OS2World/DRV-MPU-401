/* RMHELP.HPP
*/

#ifndef  __RMHELP_H__
#define  __RMHELP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS2_INCLUDED
#define INCL_NOPMAPI
#include <os2.h>
#endif

#include <rmcalls.h>

#ifdef __cplusplus
}
#endif

class DRIVER {
public:
   DRIVER(DRIVERSTRUCT *);
   ~DRIVER(void);
   int success(void) { return fRegistered; };

private:
   unsigned int fRegistered;        // True if this driver is registered with RM
   HDRIVER hdriver;

   friend class ADAPTER;
   friend class DEVICE;
   friend class RESOURCE;
   friend class PORT_RESOURCE;
   friend class IRQ_RESOURCE;
};

class RESOURCE;

class ADAPTER {
public:
   ADAPTER(ADAPTERSTRUCT *, DRIVER *);
   ~ADAPTER(void);
   int success(void) { return fRegistered; };

   int add(RESOURCE *);
//   int remove(RESOURCE *);  // not yet supported

   // Returns TRUE if the port range is NOT currently allocated
   int CheckPort(USHORT usBase, USHORT usNumPorts, USHORT usFlags, USHORT usAddrLines);

   // Returns TRUE if the IRQ is NOT currently allocated
   int CheckIRQ(USHORT usIRQ, USHORT usPCIIRQ, USHORT usFlags);
private:
   unsigned int fRegistered;        // True if this adapter is registered with RM
   HADAPTER hadapter;
   DRIVER *pdriver;

   friend class DEVICE;
   friend class RESOURCE;
   friend class PORT_RESOURCE;
   friend class IRQ_RESOURCE;
};

class DEVICE {
public:
   DEVICE(DEVICESTRUCT *, ADAPTER *);
   ~DEVICE(void);
   int success(void) { return fRegistered; };

   int regizter(void);
   void remove(void);
   int add(RESOURCE *);
//   int remove(RESOURCE *);  // not yet supported

private:
   unsigned int fRegistered;        // True if this device is registered with RM
   HDEVICE hdevice;
   ADAPTER *padapter;

   RESOURCE *presHead;       // pointer to a list of pointers to RESOURCE objects
   unsigned uNumResources;   // the # of resources for this device
};

class RESOURCE {
public:
   int regizter(void);
protected:
   RESOURCE(ULONG ulType);
   ~RESOURCE(void);

   ADAPTER *padapter;   // non-null if this resource belongs to an ADAPTER
   DEVICE *pdevice;    // non-null if this resource belongs to an DEVICE
   RESOURCESTRUCT resource;

   RESOURCE *pnext;

   unsigned int fRegistered;        // True if this resource is registered with RM
   HRESOURCE hresource;
   friend class DEVICE;
   friend class ADAPTER;
};

class PORT_RESOURCE : public RESOURCE {
public:
   PORT_RESOURCE(USHORT usBase, USHORT usNumPorts, USHORT usFlags, USHORT usAddrLines);
};

class IRQ_RESOURCE : public RESOURCE {
public:
   IRQ_RESOURCE(USHORT usIRQ, USHORT usPCIIRQ, USHORT usFlags);
};

#endif
