#include "dialogs.h"

#include "preferences.h"
#include "resources.h"
#include "watching.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <uxtheme.h>
#include <windowsx.h>

//------------------------------------------------------------------------------

HFONT createSeparatorFont(HWND hwnd) {
	HFONT font = GetWindowFont(hwnd);

	LOGFONT lf;
	GetObject(font, sizeof(LOGFONT), &lf);

	lf.lfHeight = -14;
	lf.lfWeight = FW_BOLD;

	return CreateFontIndirect(&lf);
}

void createSeparator(HWND parent, const char* name, int y) {
	static HFONT font = createSeparatorFont(parent);

	HWND hwnd = uCreateWindowEx(WS_EX_NOPARENTNOTIFY, "foobar2000:separator",
		name, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, y, 498, 20, parent,
		NULL, NULL, NULL);

	SetWindowFont(hwnd, font, TRUE);
}

//------------------------------------------------------------------------------

void loadObservedList(HWND listview, pfc::list_t<Watched> watched) {
	ListView_DeleteAllItems(listview);

	for(unsigned int i = 0; i < watched.get_count(); i++) {
		const Watched& w = watched[i];
		listview_helper::insert_item(listview, i, w.directory, NULL);

		if(w.playlist.is_empty()) continue;
		pfc::string8 playlist = w.playlist;

		if(w.playlist[0] == '0') 
			playlist.remove_chars(0, 1);
		else if(w.playlist[0] == '1')
			playlist = "[active]";
		else if(w.playlist[0] == '2')
			playlist = "[playing]";
		else continue;

		listview_helper::set_item_text(listview, i, 1, playlist);
	}
}

//------------------------------------------------------------------------------

INT_PTR CALLBACK prefPageProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	PreferencesPage* pp = (PreferencesPage*) GetWindowLong(hwnd, GWL_USERDATA);

	switch(msg) {
		case WM_INITDIALOG: {
			SetWindowLong(hwnd, GWL_USERDATA, lp);
			pp = (PreferencesPage*) lp;

			createSeparator(hwnd, "Observed folders", 0);
			createSeparator(hwnd, "File types", 192);

			HWND listview = GetDlgItem(hwnd, IDC_OBSERVEDLIST);

			SetWindowTheme(listview, L"Explorer", NULL);

			ListView_SetExtendedListViewStyle(listview, LVS_EX_FULLROWSELECT
				| LVS_EX_DOUBLEBUFFER);

			listview_helper::insert_column(listview, 0, "Folder", 178);
			listview_helper::insert_column(listview, 1, "Playlist", 80);

			loadObservedList(listview, pp->watched);

			uSetDlgItemText(hwnd, IDC_RESTRICT, cfg_restrict);
			uSetDlgItemText(hwnd, IDC_EXCLUDE, cfg_exclude);
			break;
		}

		case WM_NOTIFY: {
			NMHDR& nm = *reinterpret_cast<NMHDR*>(lp);

			if(nm.idFrom != IDC_OBSERVEDLIST) break;

			BOOL state = (ListView_GetSingleSelection(nm.hwndFrom) != -1);

			if(nm.code == LVN_ITEMCHANGED) {
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT), state);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), state);
			}
			else if(nm.code == NM_DBLCLK) {
				WORD id = state ? IDC_EDIT : IDC_ADDNEW;
				SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), 0);
			}
			break;
		}

		case WM_COMMAND: {
			if(HIWORD(wp) == BN_CLICKED) {
				WORD id = LOWORD(wp);

				HWND listview = GetDlgItem(hwnd, IDC_OBSERVEDLIST);
				int selected = ListView_GetSingleSelection(listview);

				if(id == IDC_ADDNEW) {
					Watched* w = (Watched*) uDialogBox(IDD_ADDEDIT, hwnd, 
						addEditProc, NULL);

					if(w != NULL) {
						if(selected != -1)
							pp->watched.insert_item(*w, selected);
						else
							selected = pp->watched.add_item(*w);
						delete w;
					}
					else break;
				}
				else if(id == IDC_EDIT && selected != -1) {
					Watched* w = (Watched*) uDialogBox(IDD_ADDEDIT, hwnd, 
						addEditProc, (LPARAM) &pp->watched[selected]);

					if(w != NULL) {
						pp->watched.replace_item(selected, *w);
						delete w;
					}
					else break;
				}
				else if(id == IDC_REMOVE && selected != -1) {
					pp->watched.remove_by_idx(selected);

					if(selected == pp->watched.get_count())
						selected = -1;
				}

				loadObservedList(listview, pp->watched);
				listview_helper::select_single_item(listview, selected);

				EnableWindow(GetDlgItem(hwnd, IDC_EDIT), selected != -1);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), selected != -1);
			}

			if(pp && pp->callback.is_valid())
				pp->callback->on_state_changed();
			break;
		}

		default:
			return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------------

void initControls(HWND hwnd, Watched* w) {
	if(w == NULL) {
		uSetWindowText(hwnd, "Add an observed folder");
		SendDlgItemMessage(hwnd, IDC_TYPE, CB_SETCURSEL, 0, 0);
	}
	else {
		uSetWindowText(hwnd, "Edit an observed folder");
		uSetDlgItemText(hwnd, IDC_FOLDER, w->directory);

		CheckDlgButton(hwnd, IDC_OBSERVESUB, w->watchSubtree ? 1 : 0);
		CheckDlgButton(hwnd, IDC_REQCONFIRM, w->requireConfirm ? 1 : 0);
		CheckDlgButton(hwnd, IDC_AUTOPLAY, w->autoPlay ? 1 : 0);

		SendDlgItemMessage(hwnd, IDC_TYPE, CB_SETCURSEL, 0, 0);
		if(w->playlist.is_empty()) return;

		switch(w->playlist[0]) {
			case '0': {
				pfc::string8 playlist = w->playlist;
				playlist.remove_chars(0, 1);

				uSetDlgItemText(hwnd, IDC_PLAYLIST, playlist);
				break;
			}
			case '1':
				SendDlgItemMessage(hwnd, IDC_TYPE, CB_SETCURSEL, 1, 0);
				EnableWindow(GetDlgItem(hwnd, IDC_PLAYLIST), FALSE);
				break;
			case '2':
				SendDlgItemMessage(hwnd, IDC_TYPE, CB_SETCURSEL, 2, 0);
				EnableWindow(GetDlgItem(hwnd, IDC_PLAYLIST), FALSE);
		}
	}
}

//------------------------------------------------------------------------------

void openDialog(HWND hwnd) {
	LPCWSTR title = L"Select a folder to observe";
	CoInitialize(NULL);

	OSVERSIONINFO ovi = {0};
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ovi);

	if(ovi.dwMajorVersion >= 6) {
		IFileDialog* fd;
		HRESULT hr;
		
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
			IID_IFileDialog, (LPVOID*) &fd);

		if(SUCCEEDED(hr)) {
			DWORD opts;
			fd->GetOptions(&opts);
			fd->SetOptions(opts | FOS_PICKFOLDERS);

			fd->SetTitle(title);
			hr = fd->Show(hwnd);

			if(SUCCEEDED(hr)) {
				IShellItem* result;
				hr = fd->GetResult(&result);

				if(SUCCEEDED(hr)) {
					LPOLESTR pwsz = NULL;
					hr = result->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

					if(SUCCEEDED(hr)) 
						SetDlgItemText(hwnd, IDC_FOLDER, pwsz);

			        result->Release();
				}
			}
			fd->Release();
		}
	}
	else {
		BROWSEINFO bi = {0};
		bi.hwndOwner = hwnd;
		bi.lpszTitle = title;
		bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;

		PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
		if(pidl != NULL) {
			WCHAR* buffer = new WCHAR[MAX_PATH];

			if(SHGetPathFromIDList(pidl, buffer))
				SetDlgItemText(hwnd, IDC_FOLDER, buffer);

			delete[] buffer;
			CoTaskMemFree(pidl);
		}
	}
}

//------------------------------------------------------------------------------

static void errorMessage(HWND hwnd, const char* msg) {
	uMessageBox(hwnd, msg, "Autoenqueuer", MB_ICONEXCLAMATION);
}

Watched* generateResult(HWND hwnd) {
	Watched* w = new Watched;

	uGetDlgItemText(hwnd, IDC_FOLDER, w->directory);
	if(uGetFileAttributes(w->directory) == INVALID_FILE_ATTRIBUTES) {
		pfc::string8 msg = w->directory;
		msg << (!msg.is_empty() ? "\n" : "")
			<< "Selected directory doesn't exist.";

		errorMessage(hwnd, msg);
		return NULL;
	}

	if(!w->directory.is_empty() && w->directory[0] != '\\') {
		filesystem::g_get_canonical_path(w->directory, w->directory);
		filesystem::g_get_display_path(w->directory, w->directory);
	}

	switch(SendDlgItemMessage(hwnd, IDC_TYPE, CB_GETCURSEL, 0, 0)) {
		case 0:
			uGetDlgItemText(hwnd, IDC_PLAYLIST, w->playlist);
			if(w->playlist.is_empty()) {
				errorMessage(hwnd, "Invalid playlist name.");
				return NULL;
			}
			w->playlist.insert_chars(0, "0");
			break;
		case 1:
			w->playlist = "1"; 
			break;
		case 2:
			w->playlist = "2"; 
			break;
		default:
			return NULL;
	}

	w->watchSubtree = IsDlgButtonChecked(hwnd, IDC_OBSERVESUB) == BST_CHECKED;
	w->requireConfirm = IsDlgButtonChecked(hwnd, IDC_REQCONFIRM) == BST_CHECKED;
	w->autoPlay = IsDlgButtonChecked(hwnd, IDC_AUTOPLAY) == BST_CHECKED;

	return w;
}

//------------------------------------------------------------------------------

INT_PTR CALLBACK addEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch(msg) {
		case WM_INITDIALOG: {
			HWND combobox = GetDlgItem(hwnd, IDC_TYPE);

			ComboBox_AddString(combobox, L"custom");
			ComboBox_AddString(combobox, L"active");
			ComboBox_AddString(combobox, L"playing");

			combobox = GetDlgItem(hwnd, IDC_PLAYLIST);
			static_api_ptr_t<playlist_manager> pm;

			for(t_size i = 0; i < pm->get_playlist_count(); i++) {
				if(pm->playlist_lock_is_present(i))
					continue;

				pfc::string8 playlist;
				pm->playlist_get_name(i, playlist);
				uSendMessageText(combobox, CB_ADDSTRING, 0, playlist);
			}

			if(ComboBox_GetCount(combobox))
				SendDlgItemMessage(hwnd, IDC_PLAYLIST, CB_SETCURSEL, 0, 0);

			SendMessage(hwnd, DM_SETDEFID, IDC_OK, 0);

			initControls(hwnd, (Watched*) lp);
			break;
		}

		case WM_COMMAND:
			if(LOWORD(wp) == IDCANCEL) {
				EndDialog(hwnd, NULL);
			}
			else if(HIWORD(wp) == BN_CLICKED) {
				WORD id = LOWORD(wp);
				
				if(id == IDC_OPEN)
					openDialog(hwnd);
				else if(id == IDC_OK) {
					Watched* w = generateResult(hwnd);
					if(w != NULL)
						EndDialog(hwnd, (INT_PTR) w);
				}
				else if(id == IDC_CANCEL)
					EndDialog(hwnd, NULL);
			}
			else if(HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == IDC_TYPE) {
				BOOL state = SendMessage((HWND) lp, CB_GETCURSEL, 0, 0) == 0;
				EnableWindow(GetDlgItem(hwnd, IDC_PLAYLIST), state);
			}
			break;

		case WM_CLOSE:
			EndDialog(hwnd, NULL);
			break;

		default:
			return FALSE;
	}
	return TRUE;
}
