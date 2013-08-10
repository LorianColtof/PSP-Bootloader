#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef FW150
#include "kmodlib/kmodlib.h"
#endif

#include "config.h"

#ifndef FW150

static SceUID s_kmodlibMod = -1;

BOOL loadKmodlib()
{
	s_kmodlibMod = pspSdkLoadStartModule( KMODLIB_PATH,
                                        PSP_MEMORY_PARTITION_KERNEL );
  if ( s_kmodlibMod < 0 )
  {
    ERROR_LOG( "Failed to load %s [%08x]\n", KMODLIB_PATH, (int)s_kmodlibMod );
    return FALSE;
  }

  return TRUE;
}

BOOL unloadKmodlib()
{
  int rt, rtMod;

  if ( s_kmodlibMod < 0 )
  {
    return TRUE;
  }

  rt = sceKernelStopModule( s_kmodlibMod, 0, NULL, &rtMod, NULL );
  if ( rt < 0 || rtMod < 0 )
  {
    ERROR_LOG( "Failed to stop %s [%08x,%d]\n", KMODLIB_PATH, rt, rtMod );
    return FALSE;
  } 

  rt = sceKernelUnloadModule( s_kmodlibMod );
  if ( rt < 0 )
  {
    ERROR_LOG( "Failed to unload %s [%08x]\n", KMODLIB_PATH, rt );
    return FALSE;
  }

  s_kmodlibMod = -1;
  return TRUE;
}
#endif
