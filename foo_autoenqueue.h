#ifndef FOO_AUTOENQUEUE_H
#define FOO_AUTOENQUEUE_H

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

extern cfg_watchedlist cfg_watched;
extern cfg_string cfg_restrict;
extern cfg_string cfg_exclude;
extern advconfig_integer_factory cfg_buffsize;

extern BOOL watching;
extern HANDLE mutex;

#endif // FOO_AUTOENQUEUE_H
