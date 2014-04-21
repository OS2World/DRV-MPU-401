#ifndef MONO_INCLUDED
#define MONO_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void MonoInit(void);
void MonoWrite(char __far *);
extern unsigned int fRing3;

#ifdef __cplusplus
}
#endif

#endif
