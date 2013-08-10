/*-----------------------------------------------------------------------------
 * PSPBoot 0.22 created by Jackson Mo
 * 
 * Revisions:
 *    v0.22 - Redesigned a way of entering the kernel mode eliminating the risk
 *            of overwriting of the running prx code during kernel RAM transfer,
 *            and fixed a couple of other bugs as well.
 *    v0.21 - Bug fixing primarily.
 *    v0.20(slim) - Ported to slim psp by danzel. Great thanks to danzel!
 *    v0.20 - Became configurable through a config file and was released along
 *            with kernel v0.2.
 *    v0.10 - Prototyping
 *---------------------------------------------------------------------------*/

#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
/* should be modified to your zlib location accordingly */
#include <zlib.h> 
#ifndef FW150
#include "kmodlib/kmodlib.h"
#endif


/*---------------------------------------------------------------------------*/
/* Version info                                                              */
/*---------------------------------------------------------------------------*/

#define MAJOR_VERSION 0
#define MINOR_VERSION 23


/*---------------------------------------------------------------------------*/
/* Module info                                                               */
/*---------------------------------------------------------------------------*/
#ifndef FW150
PSP_MODULE_INFO( "PSPBoot", PSP_MODULE_USER, MAJOR_VERSION, MINOR_VERSION );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER );
PSP_HEAP_SIZE_KB( 8 * 1024 );   /* 8M heap size */
#else
PSP_MODULE_INFO( "PSPBoot", 0x1000, MAJOR_VERSION, MINOR_VERSION );
PSP_MAIN_THREAD_ATTR( 0 );
#endif


/*---------------------------------------------------------------------------*/
/* Macros                                                                    */
/*---------------------------------------------------------------------------*/
#if 1
#define CONFIG_UART3
#endif

#define BOOL                int
#define TRUE                1
#define FALSE               0

#define KERNEL_ENTRY        0x88000000
#define KERNEL_PARAM_OFFSET 0x00000008
#define KERNEL_MAX_SIZE     (size_t)( 4 * 1024 * 1024 )   /* 4M */
#define MAX_KERNELS         100

#ifdef CONFIG_UART3
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
                            " PSPBoot v%d.%d Created by Jackson Mo\n"  \
							"           Modified by Lorian        \n" \
                            "=====================================\n\n", MAJOR_VERSION, MINOR_VERSION
#define BLUE                0x00ff0000
#define GREEN               0x0000ff00
#define RED                 0x000000ff
#define DELAY_BEFORE_EXIT   5 * 1000 * 1000   /* 5 seconds */
#define DELAY_BEFORE_BOOT       0 * 1000   /* 0 seconds */
#define CONFIG_FILE         "pspboot.conf"
#define CONFIG_MAX_PARAM    256
#define CONFIG_PARAM_CMD    "cmdline"
#define CONFIG_PARAM_KERNEL "kernel"
#define CONFIG_PARAM_BAUD   "baud"
#ifndef FW150
#define KMODLIB_PATH        "kmodlib.prx"
#endif

#define printf              pspDebugScreenPrintf
#define sleep(t)            sceKernelDelayThread( (t) )
#define exit()              transferControl( NULL, 0 )
#define END_OF_LINE(p)      ( *(p) == '\r' || *(p) == '\n' || *(p) == 0 )


/*---------------------------------------------------------------------------*/
/* Type definitions                                                          */
/*---------------------------------------------------------------------------*/
typedef void ( *KernelEntryFunc )(int zero_, int arch_, void * params_);
typedef struct
{
  void * buf;
  int size;
} TransferControlParams;


/*---------------------------------------------------------------------------*/
/* Static Data                                                               */
/*---------------------------------------------------------------------------*/
static char s_paramCmd[ CONFIG_MAX_PARAM ];
static char s_paramKernel[ CONFIG_MAX_PARAM ];
static BOOL s_kernelCompressed;
static SceUID s_kmodlibMod = -1;
#ifdef CONFIG_UART3
static int  s_paramBaud = 1;
#endif


/*---------------------------------------------------------------------------*/
/* Prototypes                                                                */
/*---------------------------------------------------------------------------*/
BOOL loadConfig();
char* strtrim(char* str);
BOOL CheckKernelCompression();
BOOL parseParam(const char * name_, const char * value_);
BOOL loadKernel(void ** buf_, int * size_);
BOOL loadKernelCompressed(void ** buf_, int * size_);
void transferControl(void * buf_, int size_);
#ifndef FW150
int transferControlWrapper(TransferControlParams * params_);
#endif
void disableIrq();
void memCopy(void * dest_, const void * sour_, int size_);
void pspClearIcache();
void pspClearDcache();
#ifndef FW150
BOOL loadKmodlib();
BOOL unloadKmodlib();
#endif

#ifdef CONFIG_UART3
BOOL uart3_setbaud(int baud_);
int uart3_write(const char * buf_, int size_);
int uart3_puts(const char * str_);
#endif


/*---------------------------------------------------------------------------*/
/* Implementations                                                           */
/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  TransferControlParams params;
  int rt;

  pspDebugScreenInit();
  
  /* Print the banner */
  pspDebugScreenSetTextColor( RED );
  printf( BANNER );
  pspDebugScreenSetTextColor( GREEN );

#ifndef FW150
  /* Load the kmodlib */
  if ( !loadKmodlib() )
  {
    sleep( DELAY_BEFORE_EXIT );
    exit();
  }
#endif

  /* Load the config file first */
  if ( !loadConfig() )
  {
#ifndef FW150
    (void)unloadKmodlib();
#endif
    sleep( DELAY_BEFORE_EXIT );
    exit();
  }

    /* Check if the kernel is compressed */
  if ( !CheckKernelCompression() )
  {
#ifndef FW150
    (void)unloadKmodlib();
#endif
    sleep( DELAY_BEFORE_EXIT );
    exit();
  }

  if(s_kernelCompressed)
  {

	  /* Load the compressed kernel image */
	  if ( !loadKernelCompressed( &params.buf, &params.size ) )
	  {
#ifndef FW150
	 (void)unloadKmodlib();
#endif
		sleep( DELAY_BEFORE_EXIT );
		exit();
	  }
  }
  else
  {
	    /* Load the uncompressed kernel image  */
	if ( !loadKernel( &params.buf, &params.size ) )
		{
#ifndef FW150
		(void)unloadKmodlib();
#endif
		sleep( DELAY_BEFORE_EXIT );
		exit();
		}
  }


  sleep(DELAY_BEFORE_BOOT);
#ifndef FW150
	  /* Switch to kernel mode to initiate the transfer */
	  rt = kmodExec( (KmodexecFunc)transferControlWrapper,
					 &params );  /* should never return */
#else
	  /* For 150 firmware, this loader directly runs in kernel mode */
	  transferControl( params.buf, params.size );
	  rt = -1;
#endif

  /* If it gets here, then something must be wrong */
  printf( "Failed to transfer control, err=%d\n", rt );
  sleep( DELAY_BEFORE_EXIT );
  exit();

  /* should never get here */
  return 0;
}
/*---------------------------------------------------------------------------*/
BOOL CheckKernelCompression()
{
	if ( *s_paramKernel == 0 )
    {
		printf( "Invalid kernel file\n" );
		return FALSE;
    }
	FILE* f = fopen(s_paramKernel, "rb");
	unsigned char* magic = (unsigned char*) malloc(2);
	if(f==NULL || magic==NULL)
	{
		printf( "Internal error\n" );
        return FALSE;
	}
	size_t count = fread(magic, 1, 2, f);
	if(count!=2)
	{
		printf( "Read error. (is the kernel invalid?)\n" );
        return FALSE;	
	}

	if(magic[0]==0x1F && magic[1]==0x8B) 
	{
		s_kernelCompressed=TRUE;
		printf("Kernel image is compressed.\n");
	}
	else
	{
		s_kernelCompressed=FALSE;
		//printf("kernel is not compressed.\n");
	}
	return TRUE;
}
BOOL loadConfig()
{
  FILE * fconf;
  const int bufLen = 1024;
  char buf[ bufLen ];
  char * p;
  char * end;
  char * paramName;
  char * paramValue;
  int line;

  printf( "Loading config file\n" );

  fconf = fopen( CONFIG_FILE, "r" );
  if ( fconf == NULL )
  {
    printf( "Failed to open config file %s\n", CONFIG_FILE );
    return FALSE;
  }

  for ( line = 1; fgets( buf, bufLen, fconf ) != NULL; line++ )
  {
    buf[ bufLen - 1 ] = 0;
    p = buf;
    end = buf + bufLen;

    /* skip all leading white space of tab */
    while ( ( *p == ' ' || *p == '\t' ) && *p != 0 && p < end )
    {
      p++;
    }

    /* skip empty line and comment */
    if ( END_OF_LINE( p ) || p >= end || *p == '#' )
    {
      continue;
    }

    paramName = p;    /* get the parameter name */

    while ( *p != '=' && !END_OF_LINE( p ) && p < end ) p++;
    if ( END_OF_LINE( p ) || p >= end )
    {
      printf( "Syntax error at line %d\n", line );
      fclose( fconf );
      return FALSE;
    }

    *p++ = 0;           /* set the end of the param name */
    paramValue = p;     /* get the parameter value */

    /* set the end of the param value */
    while ( !END_OF_LINE( p ) && p < end ) p++;
    *p = 0;

    /* Parse the parameter */
    if ( !parseParam( paramName, paramValue ) )
    {
      fclose( fconf );
      return FALSE;
    }
  
    line++;
  }

  fclose( fconf );
  return TRUE;
}
/*---------------------------------------------------------------------------*/
BOOL parseParam(const char * name_, const char * value_)
{
  /* cmdline=... */
  if ( stricmp( name_, CONFIG_PARAM_CMD ) == 0 )
  {
    strncpy( s_paramCmd, value_, CONFIG_MAX_PARAM );
    s_paramCmd[ CONFIG_MAX_PARAM - 1 ] = 0;
    printf( "  %s: %s\n", CONFIG_PARAM_CMD, s_paramCmd );
  }

  /* kernel=... */
  else if ( stricmp( name_, CONFIG_PARAM_KERNEL ) == 0 )
  {
    /*
    strncpy( s_paramKernel, value_, CONFIG_MAX_PARAM );
    s_paramKernel[ CONFIG_MAX_PARAM - 1 ] = 0;
    printf( "  %s: %s\n", CONFIG_PARAM_KERNEL, s_paramKernel );
	*/
	char** available_kernels = (char**) malloc(MAX_KERNELS * sizeof(char*));
	char* token;
	int i=0;
    while ((token = strsep(&value_, ",")) != NULL && i < MAX_KERNELS)
    {
	  /*
	  if(!fileExists(token))
		continue;
	  */
	  available_kernels[i++] = strtrim(token); 
       printf("  %d:%s\n", i, available_kernels[i-1]);
    }

	 strncpy( s_paramKernel, available_kernels[0], CONFIG_MAX_PARAM );
    s_paramKernel[ CONFIG_MAX_PARAM - 1 ] = 0;
    
  }

#ifdef CONFIG_UART3
  /* baud=9600|115200 */
  else if ( stricmp( name_, CONFIG_PARAM_BAUD ) == 0 )
  {
    sscanf( value_, "%d", &s_paramBaud );
    printf( "  %s: %d\n", CONFIG_PARAM_BAUD, s_paramBaud );
  }
#endif

  else
  {
    printf( "  Unsupported param %s\n", name_ );
  }

  return TRUE;
}

BOOL fileExists(char* filename)
{
	FILE* f = fopen(filename, "r");
	BOOL ret = (f != NULL);
	fclose(f);
	return ret;
}

char* strtrim(char* str)
{ 
	char *end;
  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;
  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  // Write new null terminator
  *(end+1) = 0;
  return str;
}

/*---------------------------------------------------------------------------*/
BOOL loadKernelCompressed(void ** buf_, int * size_)
{
  gzFile zf;
  void * buf;
  int size;

  printf( "Decompressing linux kernel...(%s)\n", s_paramKernel );

  if ( buf_ == NULL || size_ == NULL )
  {
    printf( "Internal error\n" );
    return FALSE;
  }

  *buf_ = NULL;
  *size_ = 0;

  if ( *s_paramKernel == 0 )
  {
    printf( "Invalid kernel file\n" );
    return FALSE;
  }

  zf = gzopen( s_paramKernel, "r" );
  if ( zf == NULL )
  {
    printf( "Failed to open file %s\n", s_paramKernel );
    return FALSE;
  }

  buf = (void *)malloc( KERNEL_MAX_SIZE );
  if ( buf == NULL )
  {
    printf( "Failed to allocate buffer\n" );
    gzclose( zf );
    return FALSE;
  }

  size = gzread( zf, buf, KERNEL_MAX_SIZE );
  if ( size < 0 )
  {
    printf( "Failed to read file\n" );
    free( buf );
    gzclose( zf );
    return FALSE;
  } 

  gzclose( zf );
  printf( "%d bytes loaded\n", size );
  
  *buf_ = buf;
  *size_ = size;

  return TRUE;
}
/*---------------------------------------------------------------------------*/
BOOL loadKernel(void ** buf_, int * size_)
{
  FILE* f;
  void * buf;
  int size;

  printf("Loading linux kernel... (%s)\n", s_paramKernel);

  if ( buf_ == NULL || size_ == NULL )
  {
    printf( "Internal error\n" );
    return FALSE;
  }

  *buf_ = NULL;
  *size_ = 0;

  if ( *s_paramKernel == 0 )
  {
    printf( "Invalid kernel file\n" );
    return FALSE;
  }

  f = fopen(s_paramKernel, "rb");
  if ( f == NULL )
  {
    printf( "Failed to open file %s\n", s_paramKernel );
    return FALSE;
  }

  buf = (void *)malloc( KERNEL_MAX_SIZE );
  if ( buf == NULL )
  {
    printf( "Failed to allocate buffer\n" );
	fclose(f);
    return FALSE;
  }

  size = fread(buf, 1, KERNEL_MAX_SIZE, f);
  if ( size < 0 )
  {
    printf( "Failed to read file\n" );
    free( buf );
	fclose(f);
    return FALSE;
  } 

  fclose(f);
  printf( "%d bytes loaded\n", size );
  
  *buf_ = buf;
  *size_ = size;

  return TRUE;
}
/*---------------------------------------------------------------------------*/
void transferControl(void * buf_, int size_)
{
  if ( buf_ != NULL && size_ > 0 )
  {
    KernelEntryFunc kernelEntry = (KernelEntryFunc)( KERNEL_ENTRY );
    void * kernelParam = NULL;

    printf( "Transfering control to 0x%08x...\n", (int)kernelEntry );

    /* disable IRQ */
    disableIrq();

    /* prepare kernel image */
    memCopy( (void *)( KERNEL_ENTRY ), buf_, size_ );

    /* prepare boot command line */
    if ( *s_paramCmd != 0 )
    {
      kernelParam = (void *)( KERNEL_ENTRY + KERNEL_PARAM_OFFSET );
      memCopy( kernelParam, s_paramCmd, CONFIG_MAX_PARAM );
    }

#ifdef CONFIG_UART3
    if ( s_paramBaud > 1 )
    {
      uart3_setbaud( s_paramBaud );
      uart3_puts( "Booting Linux kernel...\n" );
    }
#endif
    
    /* flush all caches */
    pspClearDcache();
    pspClearIcache();

    kernelEntry( 0, 0, kernelParam );
    /* never returns */
  }

  sceKernelExitGame();
}
/*---------------------------------------------------------------------------*/
int transferControlWrapper(TransferControlParams * params_)
{
  transferControl( params_->buf, params_->size );
  return 0;
}
/*---------------------------------------------------------------------------*/
void disableIrq()
{
  __asm__ __volatile__ (
    "mfc0   $8, $12\n"
    "li     $9, 0xfffe\n"
    "and    $8, $8, $9\n"
    "mtc0   $8, $12\n"
  );
}
/*---------------------------------------------------------------------------*/
void memCopy(void * dest_, const void * sour_, int size_)
{
  const char * from = (const char *)sour_;
  char * to = (char *)dest_;

  while ( size_-- > 0 )
  {
    *to++ = *from++;
  }
}
/*---------------------------------------------------------------------------*/
void pspClearIcache()
{
  /* Taken from the IPL SDK */
  __asm__ __volatile__ (
    ".word 0x40088000\n"
    ".word 0x24091000\n"
    ".word 0x7D081240\n"
    ".word 0x01094804\n"
    ".word 0x4080E000\n"
    ".word 0x4080E800\n"
    ".word 0x00004021\n"
    ".word 0xBD010000\n"
    ".word 0xBD030000\n"
    ".word 0x25080040\n"
    ".word 0x1509FFFC\n"
    ".word 0x00000000\n"
    : :
  );
}
/*---------------------------------------------------------------------------*/
void pspClearDcache()
{
  /* Taken from the IPL SDK */
  __asm__ __volatile__ (
    ".word 0x40088000\n"
    ".word 0x24090800\n"
    ".word 0x7D081180\n"
    ".word 0x01094804\n"
    ".word 0x00004021\n"
    ".word 0xBD140000\n"
    ".word 0xBD140000\n"
    ".word 0x25080040\n"
    ".word 0x1509FFFC\n"
    ".word 0x00000000\n"
    ".word 0x0000000F\n"
    ".word 0x00000000\n"
	  : :
  );
}
/*---------------------------------------------------------------------------*/
#ifndef FW150
BOOL loadKmodlib()
{
	s_kmodlibMod = pspSdkLoadStartModule( KMODLIB_PATH,
                                        PSP_MEMORY_PARTITION_KERNEL );
  if ( s_kmodlibMod < 0 )
  {
    printf( "Failed to load %s [%08x]\n", KMODLIB_PATH, (int)s_kmodlibMod );
    return FALSE;
  }

  return TRUE;
}
/*---------------------------------------------------------------------------*/
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
    printf( "Failed to stop %s [%08x,%d]\n", KMODLIB_PATH, rt, rtMod );
    return FALSE;
  } 

  rt = sceKernelUnloadModule( s_kmodlibMod );
  if ( rt < 0 )
  {
    printf( "Failed to unload %s [%08x]\n", KMODLIB_PATH, rt );
    return FALSE;
  }

  s_kmodlibMod = -1;
  return TRUE;
}
#endif
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_UART3
BOOL uart3_setbaud(int baud_)
{
  int div1, div2;

  div1 = 96000000 / baud_;
  div2 = div1 & 0x3F;
  div1 >>= 6;

  UART3_DIV1 = div1;
  UART3_DIV2 = div2;
  UART3_CTRL = UART3_MASK_SETBAUD;

  return TRUE;
}
/*---------------------------------------------------------------------------*/
int uart3_write(const char * buf_, int size_)
{
  int bytesWritten = 0;
  while ( bytesWritten < size_ )
  {
    if ( *buf_ == '\n' )
    {
      while ( UART3_STATUS & UART3_MASK_TXFULL );
      UART3_TXBUF = '\n';
      while ( UART3_STATUS & UART3_MASK_TXFULL );
      UART3_TXBUF = '\r';
    }
    else
    {
      while ( UART3_STATUS & UART3_MASK_TXFULL );
      UART3_TXBUF = *buf_;
    }

    ++buf_;
    ++bytesWritten;
  }
  
  return bytesWritten;
}
/*---------------------------------------------------------------------------*/
int uart3_puts(const char * str_)
{
  int len = 0;
  while ( str_[ len ] != 0 ) ++len;

  return uart3_write( str_, len );
}
#endif


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
