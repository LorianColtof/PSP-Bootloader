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




static int menu_x;
static int menu_y;
static int mesg_x;
static int top_mesg_y;
static int bottom_mesg_y;

static inline void printMenuEntry(char* title, BOOL selected)
{
	SCREEN_SET_X(menu_x);
	if(selected)
	{
		
		SET_COLOR_SELECTED();
		
		char spaces[61];
		memset(spaces, 0, 61);
		int i;
		for(i=0;i<60-strlen(title);i++)
		{
			spaces[i] = ' ';
		}
		printf("%s%s", title, spaces);
		
		SET_COLOR_DESELECTED();
		printf("\n");
		
	}
	else
	{
		SET_COLOR_DESELECTED();
		printf("%s\n", title);
	}
}



static inline void menuCreateBorders()
{
#ifdef ENABLE_DEBUG_LOG
pspDebugScreenClear();
printf("\n\n\n");
#endif
	SET_COLOR_MESSAGE();
	printf("\n\n");
	top_mesg_y = pspDebugScreenGetY()-2;
	printf("    ____________________________________________________________\n");
	int i;
	for(i=0;i<MAX_MENUENTRIES;i++)
	{	
		if(i==0)
		{
			menu_x = pspDebugScreenGetX()+4;
			menu_y = pspDebugScreenGetY();
		}
		printf("   |                                                            |\n");
	}
	printf("   |____________________________________________________________|\n");
	mesg_x = menu_x-1;
	bottom_mesg_y =pspDebugScreenGetY()+1;
	
	
}
static inline void printMenuEntries(int selected_index, int entriescount, MENUENTRY** entries)
{
	SET_COLOR_DESELECTED();
	pspDebugScreenSetXY(menu_x, menu_y);
	int i;
	for(i=0;i<entriescount;i++)
	{
		printMenuEntry(entries[i]->paramName, i == selected_index);
	}
}
static inline void clearMenuArea(int entriescount)
{
	pspDebugScreenSetXY(menu_x, menu_y);
	pspDebugScreenEnableBackColor(TRUE);
	pspDebugScreenSetTextColor(BLACK);
	int i;
	for(i=0;i<entriescount;i++)
	{		
		pspDebugScreenEnableBackColor(TRUE);
		printf("                                                            ");
		pspDebugScreenEnableBackColor(FALSE);
		printf("\n");
		SCREEN_SET_X(menu_x);
	}
	
}
int menuCreate(int entriescount, MENUENTRY** entries, int timeout)
{
	menuCreateBorders();
	
	int selected_index = 0;
	
		pspDebugScreenSetXY(mesg_x, bottom_mesg_y);
	printf("Use the up and down keys to navigate.\n");
	SCREEN_SET_X(mesg_x);
	printf("Press X to boot the selected option. Press O to exit.\n");
	
	

	printMenuEntries(selected_index, entriescount, entries);
	
	
	BOOL done = FALSE;
	
	SceCtrlData pad, oldpad;	
	sceCtrlSetSamplingCycle(0);

	if(timeout != -1)
	{	
		BOOL autoboot = TRUE;
		pspDebugScreenEnableBackColor(TRUE);	
		pspDebugScreenSetTextColor(GREEN);	
		pspDebugScreenSetBackColor(BLACK);
		pspDebugScreenSetXY(mesg_x, top_mesg_y);
		printf("Booting selected item automatically in %d seconds\n", timeout);
		clock_t t = clock();
		int timeLeft = timeout;
		const float timeoutFloat = (float) timeout;
		float timeLeftFloat = (float) timeLeft;
		float passed = 0;

		while(passed < timeout)
		{
			passed = ceil(((float)(clock()-t))/CLOCKS_PER_SEC);
	
			sceCtrlReadBufferPositive(&pad, 1); 
			if(pad.Buttons)
			{
				autoboot = FALSE;
				pspDebugScreenSetXY(mesg_x, top_mesg_y);
				printf("                                                            ");
				break;
			}

			if((timeoutFloat - passed) < timeLeftFloat)
			{
				timeLeftFloat = (timeoutFloat - passed);
				timeLeft = (int) timeLeftFloat;
				pspDebugScreenSetXY(mesg_x, top_mesg_y);
				printf("Booting selected item automatically in %d seconds\n", timeLeft);
			}
		
		}
		if(autoboot)
		{
			pspDebugScreenSetXY(mesg_x, top_mesg_y);
			printf("                                                            ");
			pspDebugScreenSetXY(mesg_x, top_mesg_y);
			printf("Booting %s...", entries[selected_index]->paramKernel);
			return 0;
		}
	}
	while(!done)
	{
		sceCtrlReadBufferPositive(&pad, 1); 
		
		if((pad.Buttons & PSP_CTRL_UP) != (oldpad.Buttons & PSP_CTRL_UP) || (pad.Buttons & PSP_CTRL_DOWN) != (oldpad.Buttons & PSP_CTRL_DOWN) 
		|| (pad.Buttons & PSP_CTRL_CROSS) != (oldpad.Buttons & PSP_CTRL_CROSS) || (pad.Buttons & PSP_CTRL_CIRCLE) != (oldpad.Buttons & PSP_CTRL_CIRCLE))
		{
			oldpad = pad;
			
			if (pad.Buttons & PSP_CTRL_UP)
			{
				selected_index--;
				if(selected_index<0) selected_index = 0;
			}
			if (pad.Buttons & PSP_CTRL_DOWN)
			{
				selected_index++;
				if(selected_index>=entriescount) selected_index = entriescount - 1;
			}
			if (pad.Buttons & PSP_CTRL_CROSS){
				done = TRUE;
			} 
			if (pad.Buttons & PSP_CTRL_CIRCLE){
				pspDebugScreenSetXY(mesg_x, top_mesg_y);
				pspDebugScreenSetTextColor(GREEN);	
				printf("Exitting...");
				return -1;
			} 
			if(pad.Buttons != 0)
			{
				clearMenuArea(entriescount);
				printMenuEntries(selected_index, entriescount, entries);
			}
			
		}
	}
	pspDebugScreenSetXY(mesg_x, top_mesg_y);
	pspDebugScreenSetTextColor(GREEN);	
	printf("Booting %s...", entries[selected_index]->paramKernel);
	return selected_index;
}


