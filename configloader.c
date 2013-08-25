#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "configloader.h"
#include "strings.h"


MENUENTRY* menuentries[MAX_MENUENTRIES];
int entryindex = 0;
BOOL inMenuEntry = FALSE;
int paramTimeout;


static inline BOOL parseParam(const char * name_, const char * value_)
{
	if(inMenuEntry)
	{
					
		if(strcmp(name_, "kernel") == 0)
		{
			menuentries[entryindex]->paramKernel = (char*) value_;
			DEBUG_LOG("\tKernel: %s\n", menuentries[entryindex]->paramKernel);
		}
		else if(strcmp(name_, "cmdline") == 0)
		{

			menuentries[entryindex]->paramCmd = (char*) value_;
			DEBUG_LOG("\tCommand line: %s\n", menuentries[entryindex]->paramCmd);
		}
	
		else if(strcmp(name_, "baud") == 0)
		{
			int tmp;
			sscanf( value_, "%d", &tmp);
			menuentries[entryindex]->paramBaud = tmp;
			DEBUG_LOG("\tBaud: %d\n", menuentries[entryindex]->paramBaud);
		}
		else goto UNKNOWN_VAR;
		
	}
	else
	{
		if(strcmp(name_, "timeout") == 0)
		{
			sscanf( value_, "%d", &paramTimeout);
		
			DEBUG_LOG("Timeout: %d\n", paramTimeout);
		}
		else
			goto UNKNOWN_VAR;
	}
	
	return TRUE;
UNKNOWN_VAR:
	DEBUG_LOG("\tUnknown parameter: %s\n", name_);
	return FALSE;
}

static inline BOOL parseParamLine(const char *line)
{
	int i=0;
	while(line[i] != '=') i++;
	if(!parseParam(substr(line, 0, i), substr(line, i+1, strlen(line) - i + 1))) return FALSE;		
	return TRUE;	
}
BOOL loadConfig(ConfigWrapper* cfg, char* conf_file)
{
	FILE* fconf = fopen(conf_file, "r");
	if(fconf==NULL) {
		ERROR_LOG("%s does not exist!\n", conf_file);
		goto ERROR;
	}	

	int line;
	char* buf = (char*) malloc(bufLen);

	BOOL waitForAccolade = FALSE;
	for ( line = 1; fgets( buf, bufLen, fconf ) != NULL; line++ )
	{
		buf[strlen(buf)-1] = (char) 0;
		buf = strtrim(buf);

		if(buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n' || buf[0] == '\r'|| strlen(buf) == 0) 
			continue;

		if(!inMenuEntry && !waitForAccolade)
		{
			if(strcmp(substr(buf, 0, 10), "menuentry ") == 0)
			{
				inMenuEntry = TRUE;
				menuentries[entryindex] = (MENUENTRY*) malloc(sizeof(MENUENTRY));
	
				char *rest = strtrim(substr(buf, 10, strlen(buf)-9));
				char *namestuff;
				if(rest[strlen(rest)-1] != '{') 
				{	
					namestuff = rest;
					waitForAccolade = TRUE;
				}
				else
					namestuff = substr(rest, 0, strlen(rest)-1);
				
				namestuff = strtrim(namestuff);
				if(namestuff[0] != '\"' || namestuff[strlen(namestuff)-1] != '\"')
				{
					DEBUG_LOG("Syntax error at line %d\n", line);
					goto ERROR;
				}
				menuentries[entryindex]->paramName = substr(namestuff, 1, strlen(namestuff)-2);
				DEBUG_LOG("Found menu entry: %s\n", menuentries[entryindex]->paramName);
				
			}
			else if(!parseParamLine(buf))  goto ERROR;
		}
		else if(inMenuEntry)
		{
			if(waitForAccolade)
			{
				if(buf[0] != '{')
				{
					DEBUG_LOG("Syntax error at line %d\n", line);
					goto ERROR;
				}
				waitForAccolade = FALSE;
				if(strlen(buf)>1)
				{
					char* rest = substr(buf, 1, strlen(buf)-1);
					if(!parseParamLine(rest)) goto ERROR;		
				}
			}
			else if(buf[0] == '}')
			{
				inMenuEntry = FALSE;
				entryindex++;
				if(strlen(buf)>1)
				{
					char* rest = substr(buf, 1, strlen(buf)-1);
					if(!parseParamLine(rest))  goto ERROR;
				}
				DEBUG_LOG("End of menu entry\n");
			}
			else if(buf[strlen(buf)-1] == '}')
			{
				if(!parseParamLine(substr(buf, 0, strlen(buf)-1))) goto ERROR;
				inMenuEntry = FALSE;
				entryindex++;
				DEBUG_LOG("End of menu entry\n");
			}
			else if(!parseParamLine(buf)) goto ERROR;
		}
	}

	cfg->timeout = paramTimeout;
	cfg->menuentries = menuentries;
	cfg->menuentriescount = entryindex;
	return TRUE;
ERROR:
	ERROR_LOG("An error has occurred\n");
	fclose(fconf);
	return FALSE;
}

