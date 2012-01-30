#ifndef ENQUEUEING_H
#define ENQUEUEING_H

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <string>
#include <deque>

extern std::deque< std::pair<UINT, std::wstring> > files;
DWORD WINAPI EnqueueingProc(LPVOID);

#endif // ENQUEUEING_H
