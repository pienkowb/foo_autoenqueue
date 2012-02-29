#include "preferences.h"

#include "dialogs.h"
#include "foo_autoenqueue.h"
#include "resources.h"
#include "watching.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

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

PreferencesPage::PreferencesPage(HWND parent, preferences_page_callback::ptr
	callback) : watched(cfg_watched), callback(callback)
{
	hwnd = uCreateDialog(IDD_PREFPAGE, parent, prefPageProc, (LPARAM) this);
}

t_uint32 PreferencesPage::get_state() {
	t_uint32 state = preferences_state::resettable;

	pfc::string8 restrict, exclude;
	uGetWindowText(GetDlgItem(hwnd, IDC_RESTRICT), restrict);
	uGetWindowText(GetDlgItem(hwnd, IDC_EXCLUDE), exclude);

	if(watched != cfg_watched || restrict != cfg_restrict || exclude != cfg_exclude)
		state |= preferences_state::changed;

	return state;
}

void PreferencesPage::apply() {
	(pfc::list_t<Watched>&) cfg_watched = watched;

	uGetWindowText(GetDlgItem(hwnd, IDC_RESTRICT), cfg_restrict);
	uGetWindowText(GetDlgItem(hwnd, IDC_EXCLUDE), cfg_exclude);

	restartThreads();
}

void PreferencesPage::reset() {
	watched.remove_all();
	ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_OBSERVEDLIST));

	uSetDlgItemText(hwnd, IDC_RESTRICT, "*");
	uSetDlgItemText(hwnd, IDC_EXCLUDE, "*.CUE");
}
