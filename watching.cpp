#include "watching.h"

#include "enqueueing.h"
#include "foo_autoenqueue.h"
#include "preferences.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <queue>
#include <string>
#include <vector>

using namespace std;

//------------------------------------------------------------------------------

vector<Watched> watched;
static const int waitTime = 10;

//------------------------------------------------------------------------------

static void errorMessage(DWORD code, const char* prefix = "") {
	pfc::string8 error, msg;
	uFormatSystemErrorMessage(error, code);
	msg << prefix << "\n" << error;

	uMessageBox(NULL, msg, "Autoenqueuer", MB_ICONEXCLAMATION | MB_TASKMODAL);
}

//------------------------------------------------------------------------------

DWORD WINAPI WatchingProc(LPVOID lpParameter) {
	UINT i = (UINT) lpParameter, bufferSize = (UINT) cfg_buffsize*1024;

	HANDLE directory = uCreateFile(watched[i].directory, FILE_LIST_DIRECTORY, 
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	if(directory == INVALID_HANDLE_VALUE) {
		DWORD code = GetLastError();
		if(code == ERROR_FILE_NOT_FOUND)
			code = ERROR_PATH_NOT_FOUND;

		errorMessage(code, watched[i].directory);
		return 0;
	}

	BYTE* buffer = new BYTE[bufferSize];

	OVERLAPPED overlapped = {0};
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while(watching) {
		HRESULT result = ReadDirectoryChangesW(directory, buffer, bufferSize,
			watched[i].watchSubtree, FILE_NOTIFY_CHANGE_FILE_NAME, NULL,
			&overlapped, NULL);

		if(!result) {
			errorMessage(GetLastError(), watched[i].directory);
			break;
		}

		do {
			result = WaitForSingleObject(overlapped.hEvent, waitTime);
			if(!watching) break;
		} while(result == WAIT_TIMEOUT);

		if(!watching || result == WAIT_FAILED) break;

		if(!overlapped.InternalHigh) {
			uMessageBox(NULL, "A fatal error occurred, try increasing buffer size.",
				"Autoenqueuer", MB_ICONERROR | MB_TASKMODAL);
			break;
		}

		UINT offset = 0;
		FILE_NOTIFY_INFORMATION* info;

		do {
			info = (FILE_NOTIFY_INFORMATION*) &buffer[offset];

			if(info->Action == FILE_ACTION_ADDED) {
				wstring f(info->FileName, info->FileNameLength / sizeof(WCHAR));

				WaitForSingleObject(mutex, INFINITE);
				files.push_back(make_pair(i, f));
				ReleaseMutex(mutex);
			}
			offset += info->NextEntryOffset;
		} 
		while(info->NextEntryOffset);
	}

	CloseHandle(overlapped.hEvent);
	delete[] buffer;
	CloseHandle(directory);

	return 0;
}
