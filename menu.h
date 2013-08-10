#ifndef MENU_H
#define MENU_H



#ifdef __cplusplus
extern "C" {
#endif

#include "configloader.h"

int menuCreate(int entriescount, MENUENTRY** entries, int timeout);

#ifdef __cplusplus
}
#endif
#endif
