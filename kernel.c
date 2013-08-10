#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <zlib.h>
#include <bzlib.h>
#include "kernel.h"
#include "configloader.h"
#ifndef FW150
#include "kmodlib/kmodlib.h"
#endif
#ifdef ENABLE_UART3
#include "uart3.h"
#endif


static int compression_type;
static char* kernel;
static char* cmdline;
#ifdef CONFIG_UART3
static int baud;
#endif

static inline BOOL CheckKernelCompression()
{
	FILE* f = fopen(kernel, "rb");
	if(f == NULL)
	{
		pspDebugScreenClear();
		ERROR_LOG("Cannot boot %s: File does not exist\n", kernel);
		return FALSE;
	}
	unsigned char* magic = (unsigned char*) malloc(2);
	size_t count = fread(magic, 1, 2, f);
	if(count!=2)
	{
		ERROR_LOG( "Read error.\n" );
		return FALSE;	
	}
	if(magic[0]==0x1F && magic[1]==0x8B) 
	{
		DEBUG_LOG("Detected GZIP compression\n");
		compression_type = COMPRESSION_GZIP;
	}
	else if(magic[0]=='B' && magic[1]=='Z') 
	{
		DEBUG_LOG("Detected BZIP2 compression\n");
		compression_type = COMPRESSION_BZIP2;
	}
	else
	{
		DEBUG_LOG("Detected no compression\n");
		compression_type = COMPRESSION_NONE;
	}
	fclose(f);
	return TRUE;	
}
static inline BOOL loadKernel(void** buf_, int* size_)
{
	if(compression_type == COMPRESSION_NONE)
	{
		FILE* f;
		void *buf;
		size_t size;
		if(buf_ == NULL || size_ == NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: buf_ == NULL or size_ == NULL\n", __FUNCTION__);
			return FALSE;
		}
		*buf_ = NULL;
		*size_ = 0;
		f = fopen(kernel, "rb");
		if(f==NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: f==NULL but no error occurred in CheckKernelCompression()\n", __FUNCTION__);
			return FALSE;
		}

		buf = (void *)malloc( KERNEL_MAX_SIZE );
		if ( buf == NULL )
		{
			pspDebugScreenClear();
		    ERROR_LOG("%s: Failed to allocate buffer\n", __FUNCTION__);
		    fclose(f);
		    return FALSE;
		}
		size = fread(buf, 1, KERNEL_MAX_SIZE, f);
		if ( size < 0 )
		{
			pspDebugScreenClear();
			ERROR_LOG("%s: Failed to read file\n", __FUNCTION__ );
			free( buf );
			fclose(f);
			return FALSE;
		} 

		fclose(f);
		DEBUG_LOG("%s: %d bytes loaded\n", __FUNCTION__, size );
		  
		*buf_ = buf;
		*size_ = size;

		return TRUE;
	}
	else if(compression_type == COMPRESSION_GZIP)
	{
		gzFile* f;
		void *buf;
		size_t size;

		if(buf_ == NULL || size_ == NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: buf_ == NULL or size_ == NULL\n", __FUNCTION__);
			return FALSE;
		}
		*buf_ = NULL;
		*size_ = 0;
		f = gzopen(kernel, "rb");
		if(f==NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: f==NULL but no error occurred in CheckKernelCompression()\n", __FUNCTION__);
			return FALSE;
		}

		buf = (void *)malloc( KERNEL_MAX_SIZE );
		if ( buf == NULL )
		{
		    pspDebugScreenClear();
		    ERROR_LOG("%s: Failed to allocate buffer\n", __FUNCTION__);
		    gzclose(f);
		    return FALSE;
		}
		size = gzread(f, buf, KERNEL_MAX_SIZE);
		if ( size < 0 )
		{
			pspDebugScreenClear();
			ERROR_LOG("%s: Failed to read file\n", __FUNCTION__ );
			free( buf );
			gzclose(f);
			return FALSE;
		} 

		gzclose(f);
		DEBUG_LOG("%s: %d bytes loaded\n", __FUNCTION__, size );
		  
		*buf_ = buf;
		*size_ = size;

		return TRUE;
	}
	else if(compression_type == COMPRESSION_BZIP2)
	{
		FILE* f;
		BZFILE* bzf;

		void *buf;
		size_t size;
		if(buf_ == NULL || size_ == NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: buf_ == NULL or size_ == NULL\n", __FUNCTION__);
			return FALSE;
		}
		*buf_ = NULL;
		*size_ = 0;
		f = fopen(kernel, "rb");
		if(f==NULL)
		{
			pspDebugScreenClear();
			ERROR_LOG("BUG: %s: f==NULL but no error occurred in CheckKernelCompression()\n", __FUNCTION__);
			return FALSE;
		}

		int error;
		bzf = BZ2_bzReadOpen(&error, f, 0, FALSE, NULL, 0);
		if(error != BZ_OK)
		{
			pspDebugScreenClear();
			ERROR_LOG("%s: bzReadOpen Error: %d\n", __FUNCTION__, error);
			fclose(f);
			return FALSE;
		}

		buf = (void *)malloc( KERNEL_MAX_SIZE );
		if ( buf == NULL )
		{
			pspDebugScreenClear();
		    ERROR_LOG("%s: Failed to allocate buffer\n", __FUNCTION__);
		    BZ2_bzReadClose(&error, bzf);
			fclose(f);
		    return FALSE;
		}
		size = BZ2_bzRead(&error, bzf, buf, KERNEL_MAX_SIZE);
		if(error != BZ_STREAM_END)
		{
			pspDebugScreenClear();
			ERROR_LOG("%s: bzRead Error: %d\n", __FUNCTION__, error);
			BZ2_bzReadClose(&error, bzf);
			fclose(f);
			return FALSE;
		}
		if ( size < 0 )
		{
			pspDebugScreenClear();
			ERROR_LOG("%s: Failed to read file\n", __FUNCTION__ );
			free( buf );
			fclose(f);
			return FALSE;
		} 
		BZ2_bzReadClose(&error, bzf);
		fclose(f);
		DEBUG_LOG("%s: %d bytes loaded\n", __FUNCTION__, size );
		  
		*buf_ = buf;
		*size_ = size;

		return TRUE;
	}
	else
	{
		pspDebugScreenClear();
		ERROR_LOG("BUG: %s: Unknown compression type: %d\n",  __FUNCTION__, compression_type);
		return FALSE;
	}
}
void disableIrq()
{
  __asm__ __volatile__ (
    "mfc0   $8, $12\n"
    "li     $9, 0xfffe\n"
    "and    $8, $8, $9\n"
    "mtc0   $8, $12\n"
  );
}
void memCopy(void * dest_, const void * sour_, int size_)
{
  const char * from = (const char *)sour_;
  char * to = (char *)dest_;

  while ( size_-- > 0 )
  {
    *to++ = *from++;
  }
}
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
void transferControl(void* buf_, int size_)
{
	if ( buf_ != NULL && size_ > 0 )
	{
	    KernelEntryFunc kernelEntry = (KernelEntryFunc)( KERNEL_ENTRY );
	    void * kernelParam = NULL;

	    DEBUG_LOG( "Transfering control to 0x%08x...\n", (int)kernelEntry );

	    /* disable IRQ */
	    disableIrq();

	    /* prepare kernel image */
	    memCopy( (void *)( KERNEL_ENTRY ), buf_, size_ );

	    /* prepare boot command line */
	    if ( cmdline != 0 )
	    {
		 kernelParam = (void *)( KERNEL_ENTRY + KERNEL_PARAM_OFFSET );
		 memCopy( kernelParam, cmdline, CONFIG_MAX_PARAM );
	    }

#ifdef CONFIG_UART3
	    if ( baud > 1 )
	    {
		 uart3_setbaud(baud);
		 uart3_puts( "Booting kernel...\n" );
	    }
#endif
	    
	    /* flush all caches */
	    pspClearDcache();
	    pspClearIcache();

	    kernelEntry( 0, 0, kernelParam );
	    /* never returns */
  }
}
#ifndef FW150
int transferControlWrapper(TransferControlParams * params_)
{
	transferControl( params_->buf, params_->size );
	return 0;
}
#endif
BOOL bootKernel(MENUENTRY* entry)
{
#ifdef ENABLE_DEBUG_LOG
	pspDebugScreenClear();
	SET_COLOR_MESSAGE();
#endif
	TransferControlParams params;
	kernel = entry->paramKernel;
	cmdline = entry->paramCmd;
	DEBUG_LOG("Booting %s...\n", kernel);
	if(!CheckKernelCompression()) return FALSE;
	if(!loadKernel(&params.buf, &params.size)) return FALSE;
	sleep(DELAY_BEFORE_BOOT);
#ifndef FW150
	  /* Switch to kernel mode to initiate the transfer */
	  kmodExec( (KmodexecFunc)transferControlWrapper,
					 &params );  /* should never return */
#else
	  /* For 150 firmware, this loader directly runs in kernel mode */
	  transferControl( params.buf, params.size );
#endif

	return TRUE;
}
