/*-----------------------------------------------------------------------------
 *  PSP Bootloader 0.1 
 *  Created by Lorian Coltof
 *  Based on PSPBoot by Jackson Mo
 *---------------------------------------------------------------------------*/

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


#include "config.h"
#include "configloader.h"
#include "menu.h"
#include "kernel.h"
#ifndef FW150
#include "kmodlibloader.h"
#endif


/*---------------------------------------------------------------------------*/
/* Module info                                                               */
/*---------------------------------------------------------------------------*/
#ifndef FW150
PSP_MODULE_INFO( "PSPBootloader", PSP_MODULE_USER, MAJOR_VERSION, MINOR_VERSION );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER );
PSP_HEAP_SIZE_KB( 8 * 1024 );   /* 8M heap size */
#else
PSP_MODULE_INFO( "PSPBootloader", 0x1000, MAJOR_VERSION, MINOR_VERSION );
PSP_MAIN_THREAD_ATTR( 0 );
#endif

ConfigWrapper cfg;
int main(int argc, char *argv[])
{
	pspDebugScreenInit();
	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlReadBufferPositive(&pad, 1); 
	
	/* Print the banner */
	pspDebugScreenSetTextColor( RED );
  	printf( BANNER );
  	pspDebugScreenSetTextColor( GREEN );

#ifndef FW150
	  /* Load the kmodlib */
	  if ( !loadKmodlib() )
	  {
	    sleep( DELAY_BEFORE_EXIT );
	    sceKernelExitGame();
	  }
	  DEBUG_LOG("Successfully loaded kmodlib\n");
#endif
	cfg.timeout = -1;
	if(!loadConfig(&cfg, CONFIG_FILE))
	{
		ERROR_LOG("Failed to load config file\n");
		unloadKmodlib();
		goto EXIT_ERROR;
	}
	DEBUG_LOG("Successfully loaded config file\n");
	
	
	
	int index;
	if(cfg.timeout == -1) cfg.timeout = 5; // default timeout
	else if(cfg.timeout == 0) index = 0; // don't show the menu, boot first menuentry

	if(pad.Buttons & PSP_CTRL_RTRIGGER) cfg.timeout = -1; //Skip the timeout

	// menuCreate returns the index in menuentries which we need to boot
	if(cfg.timeout > 0 || cfg.timeout == -1)
		index = menuCreate(cfg.menuentriescount, cfg.menuentries, cfg.timeout);
	else
	{
		pspDebugScreenSetTextColor(GREEN);	
		printf("Booting %s...\n", cfg.menuentries[index]->paramKernel);
	}

	if(index == -1) // When circle is pressed
	{
		DEBUG_LOG("Exitting...\n");
		unloadKmodlib();
		sleep(2*1000*1000);
		sceKernelExitGame();
		return 0;
	}
	
	bootKernel(cfg.menuentries[index]);
	//Should not return
	
	// if anything goes wrong
	ERROR_LOG("Error booting kernel\n");
	unloadKmodlib();
	goto EXIT_ERROR;

	sleep( DELAY_BEFORE_EXIT );
	sceKernelExitGame();
	return 0;
EXIT_ERROR:
	ERROR_LOG("Exitting...\n");
	sleep( DELAY_BEFORE_EXIT );
	//sleep(2 * 1000 * 1000);
	sceKernelExitGame();
	return 0;
}
