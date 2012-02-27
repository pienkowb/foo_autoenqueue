#ifndef FOO_AUTOENQUEUE_H
#define FOO_AUTOENQUEUE_H

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

extern BOOL watching;
extern HANDLE mutex;

void restartThreads();

#endif // FOO_AUTOENQUEUE_H
