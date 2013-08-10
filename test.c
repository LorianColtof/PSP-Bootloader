#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


#define bufLen 1024

#define BOOL                int
#define TRUE                1
#define FALSE               0
#define MAX_MENUENTRIES     100

#define DEBUG_LOG(...) printf(__VA_ARGS__)
#define ERROR_LOG(...) fprintf(stderr, __VA_ARGS__)

#define CONFIG_FILE "pspboot.conf"

char* strtrim(char* str);
char *substr(const char* str, int start, int stop);


typedef struct 
{
	char *paramName;
	char *paramKernel;
	char *paramCmd;
	BOOL paramKernelCompressed;
	int paramBaud;		
} MENUENTRY;


// Global variables

MENUENTRY* menuentries[MAX_MENUENTRIES];
int entryindex = 0;
BOOL inMenuEntry = FALSE;
int paramTimeout;

BOOL loadConfig()
{
	FILE* fconf = fopen(CONFIG_FILE, "r");
	if(fconf==NULL) {
		DEBUG_LOG("%s does not exist!\n", CONFIG_FILE);
		goto ERROR;
	}	

	int line;
	char* buf = (char*) malloc(bufLen);

	BOOL waitForAccolade = FALSE;
	for ( line = 1; fgets( buf, bufLen, fconf ) != NULL; line++ )
	{
		buf[strlen(buf)-1] = (char) 0;
		strtrim(buf);

		if(buf[0] == '#' || buf[0] == ' ' || buf[0] == '\n' || buf[0] == '\r'|| strlen(buf) == 0) 
			continue;

		if(!inMenuEntry && !waitForAccolade)
		{
			if(strcmp(substr(buf, 0, 10), "menuentry ") == 0)
			{
				inMenuEntry = TRUE;
				menuentries[entryindex] = (MENUENTRY*) malloc(sizeof(MENUENTRY));
	
				char *rest = substr(buf, 10, strlen(buf)-9);;
				strtrim(rest);
				char *namestuff;
				if(rest[strlen(rest)-1] != '{') 
				{	
					namestuff = rest;
					waitForAccolade = TRUE;
				}
				else
					namestuff = substr(rest, 0, strlen(rest)-1);
				
				strtrim(namestuff);
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
	printf("Found %d entries in total\n", entryindex);
	return TRUE;
ERROR:
	ERROR_LOG("An error has occurred\n");
	fclose(fconf);
	return FALSE;
}

BOOL parseParamLine(const char *line)
{
	int i=0;
	while(line[i] != '=') i++;
	if(!parseParam(substr(line, 0, i), substr(line, i+1, strlen(line) - i + 1))) return FALSE;		
	return TRUE;	
}
BOOL parseParam(const char * name_, const char * value_)
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

char *substr(const char* str, int start, int stop)
{
	char* result = (char*) malloc(strlen(str)+1);
	memset(result, 0, strlen(str)+1);
	strncpy(result, (char*) (str+start), stop);
	return result;
}

#define TIMEOUT 10
int main(int argc, char** argv)
{
	printf("Booting selected item automatically in %d seconds\n", TIMEOUT);
	clock_t t = clock();
	int timeLeft = TIMEOUT;
	const float timeoutFloat = (float) TIMEOUT;
	float timeLeftFloat = (float) timeLeft;
	float passed;
	while(passed < TIMEOUT)
	{
		passed = ceil(((float)(clock()-t))/CLOCKS_PER_SEC);
	
		
		if((timeoutFloat - passed) < timeLeftFloat)
		{
			timeLeftFloat = (timeoutFloat - passed);
			timeLeft = (int) timeLeftFloat;
			printf("Booting selected item automatically in %d seconds\n", timeLeft);
		}
		
	}
	printf("passed=%f\n", passed);
	
}


