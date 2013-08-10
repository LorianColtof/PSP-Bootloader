#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

typedef struct 
{
	char *paramName;
	char *paramKernel;
	char *paramCmd;
	BOOL paramKernelCompressed;
	int paramBaud;		
} MENUENTRY;

typedef struct
{
	int timeout;
	MENUENTRY** menuentries;
	int menuentriescount;
} ConfigWrapper;

BOOL loadConfig(ConfigWrapper* cfg, char* conf_file);


#ifdef __cplusplus
{
#endif
#endif

