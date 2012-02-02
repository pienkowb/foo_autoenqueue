#include "watching.h"
#include "enqueueing.h"
#include "dialogs.h"
#include "resources.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <list>
#include <iterator>

using namespace std;
using namespace stdext;

//------------------------------------------------------------------------------

DECLARE_COMPONENT_VERSION(
	"Autoenqueuer", 
	"1.0", 
	"Copyright (C) 2008 Bartosz Pie\305\204kowski"
);

//------------------------------------------------------------------------------

static const GUID cfg_watched_guid = { 0x464beab7, 0x1c84, 0x48f8, { 0xb6, 0xd8,
	0x9e, 0x18, 0xfe, 0xc0, 0x3a, 0x6e } };

static const GUID cfg_restrict_guid = { 0x423952f7, 0xa361, 0x4d6e, { 0x82,
	0x84, 0x4, 0x95, 0x47, 0xbb, 0x99, 0xdf } };

static const GUID cfg_exclude_guid = { 0x5d68e300, 0xad31, 0x4667, { 0xa9, 0x41,
	0x69, 0xaf, 0x13, 0xb8, 0x2e, 0x92 } };

static const GUID cfg_branch_guid = { 0xf8be81d, 0xc722, 0x4ff9, { 0x97, 0xde,
	0x1d, 0xa1, 0x10, 0x55, 0xde, 0x60 } };

static const GUID cfg_buffsize_guid = { 0xef84c02a, 0xa5e, 0x4d71, { 0xb7,
	0x67, 0x8f, 0xb4, 0xab, 0x58, 0x20, 0x5b } };

//------------------------------------------------------------------------------

cfg_watchedlist cfg_watched(cfg_watched_guid);
cfg_string cfg_restrict(cfg_restrict_guid, "*");
cfg_string cfg_exclude(cfg_exclude_guid, "*.CUE");

advconfig_branch_factory cfg_branch("Autoenqueuer", cfg_branch_guid,
	advconfig_entry::guid_branch_tools, 0);

advconfig_integer_factory cfg_buffsize("Buffer size (kB)", cfg_buffsize_guid,
	cfg_branch_guid, 0, 10, 1, 64);

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

void onDestroy() {
	foo_initquit.get_static_instance().on_quit();
	foo_initquit.get_static_instance().on_init();
}

//------------------------------------------------------------------------------

class preferences_page_autoenqueue : public preferences_page {
	HWND create(HWND p_parent) {
		return uCreateDialog(IDD_PREFPAGE, p_parent, prefPageProc);
	}
	const char* get_name() { return "Autoenqueuer"; }
	GUID get_guid() {
		static const GUID guid = { 0x760866b8, 0x971, 0x4aed, { 0xb8, 0x5e,
			0x41, 0xa5, 0xda, 0xba, 0xc3, 0x75 } };
		return guid;
	}
	GUID get_parent_guid() { return guid_tools; }
	bool reset_query() { return true; }
	void reset() {
		cfg_watched.remove_all();
		cfg_restrict = "*";
		cfg_exclude = "*.CUE";
	}
};

static preferences_page_factory_t<preferences_page_autoenqueue> 
	foo_preferences_page;
