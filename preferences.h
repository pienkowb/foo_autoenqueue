#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "watching.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

//------------------------------------------------------------------------------

extern cfg_watchedlist cfg_watched;
extern cfg_string cfg_restrict;
extern cfg_string cfg_exclude;
extern advconfig_integer_factory cfg_buffsize;

//------------------------------------------------------------------------------

class PreferencesPage : public preferences_page_instance {
	HWND hwnd;
	pfc::list_t<Watched> watched;
	preferences_page_callback::ptr callback;
public:
	PreferencesPage(HWND parent, preferences_page_callback::ptr callback);

	t_uint32 get_state();
	HWND get_wnd() { return hwnd; }
	void apply();
	void reset();

	friend INT_PTR CALLBACK prefPageProc(HWND, UINT, WPARAM, LPARAM);
};

#endif // PREFERENCES_H
