#include "foo_autoenqueue.h"

#include "enqueueing.h"
#include "preferences.h"
#include "watching.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <iterator>
#include <list>

using namespace std;
using namespace stdext;

//------------------------------------------------------------------------------

DECLARE_COMPONENT_VERSION(
	"Autoenqueuer", 
	"1.0", 
	"Copyright (C) 2008 Bartosz Pie\305\204kowski"
);

//------------------------------------------------------------------------------

BOOL watching = FALSE;
HANDLE mutex = NULL;

//------------------------------------------------------------------------------

class initquit_autoenqueue : public initquit {
	list<HANDLE> threads;
public:
	void on_init() {
		for(unsigned int i = 0; i < cfg_watched.get_count(); i++)
			watched.push_back(cfg_watched[i]);		

		watching = TRUE;
		mutex = CreateMutex(NULL, FALSE, NULL);

		for(unsigned int i = 0; i < watched.size(); i++) {
			HANDLE h = CreateThread(NULL, 0, WatchingProc, LPVOID(i), 0, NULL);
			threads.push_back(h);
		}

		threads.push_back(CreateThread(NULL, 0, EnqueueingProc, NULL, 0, NULL));
	}

	void on_quit() {
		HANDLE* handles = new HANDLE[threads.size()];
		checked_array_iterator<HANDLE*> it(handles, threads.size());
		copy(threads.begin(), threads.end(), it);

		watching = FALSE;
		WaitForMultipleObjects(threads.size(), handles, TRUE, INFINITE);
		CloseHandle(mutex);

		watched.clear();

		delete[] handles;
	}
};

static initquit_factory_t<initquit_autoenqueue> foo_initquit;

void restartThreads() {
	foo_initquit.get_static_instance().on_quit();
	foo_initquit.get_static_instance().on_init();
}

//------------------------------------------------------------------------------

class preferences_page_autoenqueue : public preferences_page_v3 {
	preferences_page_instance::ptr instantiate(HWND parent,
		preferences_page_callback::ptr callback)
	{
		return new service_impl_t<PreferencesPage>(parent, callback);
	}

	const char* get_name() { return "Autoenqueuer";	}

	GUID get_parent_guid() { return guid_tools; }

	GUID get_guid() {
		static const GUID guid = { 0x760866b8, 0x971, 0x4aed, { 0xb8, 0x5e,
			0x41, 0xa5, 0xda, 0xba, 0xc3, 0x75 } };
		return guid;
	}
};

static preferences_page_factory_t<preferences_page_autoenqueue>
	foo_preferences_page;
