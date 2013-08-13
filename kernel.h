#ifndef KERNEL_H
#define KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "configloader.h"

typedef void ( *KernelEntryFunc )(int zero_, int arch_, void * params_);
typedef struct
{
  void * buf;
  int size;
} TransferControlParams;


char* bootKernel(MENUENTRY* entry);
#ifdef __cplusplus
{
#endif
#endif

