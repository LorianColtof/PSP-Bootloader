/*	
 * Config file for PSP Bootloader
 * Created by Lorian Coltof
*/

#ifndef CONFIG_H
#define CONFIG_H

#define bufLen 1024

#define BOOL                int
#define TRUE                1
#define FALSE               0
#define MAX_MENUENTRIES     19


#define KERNEL_ENTRY        0x88000000
#define KERNEL_PARAM_OFFSET 0x00000008
#define KERNEL_MAX_SIZE     (size_t)( 4 * 1024 * 1024 )   /* 4M */

#define COMPRESSION_NONE	   0
#define COMPRESSION_GZIP    1
#define COMPRESSION_BZIP2   2

#ifdef ENABLE_UART3
#define UART3_RXBUF         ( *(volatile char *)0xbe500000 )
#define UART3_TXBUF         ( *(volatile char *)0xbe500000 )
#define UART3_STATUS        ( *(volatile unsigned int *)0xbe500018 )
#define UART3_DIV1          ( *(volatile unsigned int *)0xbe500024 )
#define UART3_DIV2          ( *(volatile unsigned int *)0xbe500028 )
#define UART3_CTRL          ( *(volatile unsigned int *)0xbe50002c )
#define UART3_MASK_RXEMPTY  ( (unsigned int)0x00000010 )
#define UART3_MASK_TXFULL   ( (unsigned int)0x00000020 )
#define UART3_MASK_SETBAUD  ( (unsigned int)0x00000060 )
#endif

#define BANNER              "=====================================\n" \
                            "|       PSP Bootloader v%d.%d         |\n"  \
                            "=====================================\n\n", MAJOR_VERSION, MINOR_VERSION

#define WHITE			   0x00ffffff
#define BLACK			   0x00000000
#define BLUE                0x00ff0000
#define GREEN               0x0000ff00
#define RED                 0x000000ff
#define DELAY_BEFORE_EXIT   5 * 1000 * 1000   /* 5 seconds */
#define DELAY_BEFORE_BOOT   1 * 1000 * 1000   /* 1 second */
#define CONFIG_FILE         "pspboot.conf"
#define CONFIG_MAX_PARAM    256
#ifndef FW150
#define KMODLIB_PATH        "kmodlib.prx"
#endif

#define printf              pspDebugScreenPrintf
#define sleep(t)            sceKernelDelayThread( (t) )
#define SET_COLOR_MESSAGE(); pspDebugScreenEnableBackColor(FALSE); pspDebugScreenSetBackColor(BLACK); pspDebugScreenSetTextColor(GREEN);
#define SET_COLOR_ERROR_MESSAGE(); pspDebugScreenEnableBackColor(FALSE); pspDebugScreenSetBackColor(BLACK); pspDebugScreenSetTextColor(RED);
#define SET_COLOR_SELECTED(); pspDebugScreenEnableBackColor(TRUE); pspDebugScreenSetBackColor(WHITE); pspDebugScreenSetTextColor(BLACK);
#define SET_COLOR_DESELECTED(); pspDebugScreenEnableBackColor(FALSE); pspDebugScreenSetBackColor(BLACK); pspDebugScreenSetTextColor(WHITE);
#define SCREEN_SET_X(x) pspDebugScreenSetXY(x, pspDebugScreenGetY());

#ifdef ENABLE_DEBUG_LOG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void) 0)
#endif

#ifdef ENABLE_ERROR_LOG
#define ERROR_LOG(...) printf( __VA_ARGS__)
#else
#define ERROR_LOG(...) ((void) 0)
#endif



#endif
