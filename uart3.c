#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"

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
