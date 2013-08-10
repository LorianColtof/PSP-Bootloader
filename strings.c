#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

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
