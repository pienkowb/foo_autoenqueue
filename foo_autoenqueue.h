#ifndef FOO_AUTOENQUEUE_H
#define FOO_AUTOENQUEUE_H

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

extern cfg_watchedlist cfg_watched;
extern cfg_uint cfg_buffsize;
extern cfg_string cfg_restrict;
extern cfg_string cfg_exclude;

extern BOOL watching;
extern HANDLE mutex;

void onDestroy();

#endif // FOO_AUTOENQUEUE_H