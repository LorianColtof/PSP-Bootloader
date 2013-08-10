#ifndef UART3_H
#define UART3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

BOOL uart3_setbaud(int baud_);
int uart3_write(const char * buf_, int size_);
int uart3_puts(const char * str_);

#ifdef __cplusplus
{
#endif
#endif

