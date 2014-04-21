/* MPUTRACE.CPP
*/

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#define INCL_NOPMAPI
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#include <os2.h>

#define OPEN_FLAG (  )
#define OPEN_MODE ( OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE )

#define KEYLEN   128

char abKey[KEYLEN+1] =
   {"REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_"};

ULONG ulFileSize = 0;         // The file to stamp
BYTE *pbFile = NULL;

ULONG ulKeyFileSize = 0;      // Our private key
BYTE *pbKeyFile = NULL;

ULONG ulRegFileSize = 0;      // Contains the registration data
BYTE *pbRegFile = NULL;

unsigned ReadFile(PSZ pszFileName, PULONG pulFileSize, BYTE **ppbFile)
{
   APIRET rc;
   HFILE hfile = 0;
   ULONG ulAction = 0;

   printf("Reading %s\n", pszFileName);
   rc = DosOpen(pszFileName, &hfile, &ulAction, 0, 0,
                OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READONLY,
                NULL);
   if (rc) {
      printf("Error %d while trying to open file\n", rc);
      return FALSE;
   }

   rc = DosSetFilePtr(hfile, 0, FILE_END, pulFileSize);
   if (rc) {
      printf("Error %d while trying to determine file size\n", rc, pszFileName);
      return FALSE;
   }
   if (!*pulFileSize) {
      printf("File is zero bytes long\n", *pulFileSize);
      return FALSE;
   }

   ULONG ulBytesRead = 0;
   rc = DosSetFilePtr(hfile, 0, FILE_BEGIN, &ulBytesRead);
   if (rc || ulBytesRead) {
      printf("Error %d while trying to reset file pointer\n", rc, pszFileName);
      return FALSE;
   }

   *ppbFile = (BYTE *) malloc(*pulFileSize);
   if (!*ppbFile) {
      printf("Could not allocate %d bytes for file\n", *pulFileSize);
      return FALSE;
   }

   rc = DosRead(hfile, *ppbFile, *pulFileSize, &ulBytesRead);
   if (rc) {
      printf("Error reading %d of %d bytes\n", ulBytesRead, *pulFileSize);
      return FALSE;
   }

   if (ulBytesRead != *pulFileSize) {
      printf("Only read %d of %d bytes\n", ulBytesRead, *pulFileSize);
      return FALSE;
   }

   rc = DosClose(hfile);
   if (rc) {
      printf("Error %d closing file.\n", rc);
      return FALSE;
   }

   return TRUE;
}

unsigned WriteFile(PSZ pszFileName)
{
   APIRET rc;
   HFILE hfile = 0;
   ULONG ulAction = 0;

   printf("Writing %s\n", pszFileName);
   rc = DosOpen(pszFileName, &hfile, &ulAction, 0, 0,
                OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY,
                NULL);
   if (rc) {
      printf("Error %d while trying to open file\n", rc);
      return FALSE;
   }

   ULONG ulBytesWritten = 0;
   rc = DosWrite(hfile, pbFile, ulFileSize, &ulBytesWritten);
   if (rc) {
      printf("Error writing %d of %d bytes\n", ulBytesWritten, ulFileSize);
      return FALSE;
   }

   if (ulBytesWritten != ulFileSize) {
      printf("Only wrote %d of %d bytes\n", ulBytesWritten, ulFileSize);
      return FALSE;
   }

   rc = DosClose(hfile);
   if (rc) {
      printf("Error %d closing file.\n", rc);
      return FALSE;
   }

   return TRUE;
}

BYTE *FindKeySpace(void)
{
   printf("Searching for keyspace...\n");

   unsigned i=0;
   while (i <= ulFileSize - KEYLEN) {
      BYTE *pb = (BYTE *) memchr(pbFile+i, abKey[0], ulFileSize - i);
      if (pb) {
         if (memcmp(pb, abKey, KEYLEN) == 0)
            return pb;
         unsigned l = (unsigned) pb - ((unsigned) pbFile + i);
         i += l;
      } else
         return NULL;
      i++;
   }

   return NULL;
}

void WriteKey(BYTE *pbKey, BYTE *pbRegData)
{
   printf("Registration Data:\n%s\n", pbRegData);
}

int main(int argc, char **argv)
{

   printf("REGSTAMP - Theta Band Software Registration Stamp\n");
   printf("INTERNAL USE ONLY!\n");

   if (argc != 3) {
      printf("%s <file_to_stamp> <file_with_reg_info>\n", argv[0]);
      return 1;
   }

   if (!ReadFile(argv[1], &ulFileSize, &pbFile))
      return 1;

   if (!ReadFile(argv[2], &ulRegFileSize, &pbRegFile))
      return 1;

   BYTE *pb = FindKeySpace();
   if (pb) {
      WriteKey(pb, pbRegFile);
//      WriteFile(argv[1]);
   }

   return 0;
}
