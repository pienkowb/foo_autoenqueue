#include "dialogs.h"
#include "watching.h"
#include "resources.h"
#include "foo_autoenqueue.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

//------------------------------------------------------------------------------

void loadObservedList(HWND listview) {
	ListView_DeleteAllItems(listview);

	for(unsigned int i = 0; i < cfg_watched.get_count(); i++) {
		const Watched& w = cfg_watched[i];
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
	switch(msg) {
		case WM_INITDIALOG: {
			HWND listview = GetDlgItem(hwnd, IDC_OBSERVEDLIST);			

			ListView_SetExtendedListViewStyle(listview, LVS_EX_FULLROWSELECT 
				| LVS_EX_GRIDLINES);
			
			listview_helper::insert_column(listview, 0, "Folder", 175);
			listview_helper::insert_column(listview, 1, "Playlist", 80);
			
			loadObservedList(listview);

			HWND trackbar = GetDlgItem(hwnd, IDC_BUFFSIZE);
			SendMessage(trackbar, TBM_SETRANGE, FALSE, MAKELONG(1, 64));
			SendMessage(trackbar, TBM_SETPOS, TRUE, cfg_buffsize);
			SendMessage(hwnd, WM_HSCROLL, 0, LPARAM(trackbar));

			uSetDlgItemText(hwnd, IDC_RESTRICT, cfg_restrict);
			uSetDlgItemText(hwnd, IDC_EXCLUDE, cfg_exclude);
			break;
		}

		case WM_NOTIFY: {
			NMHDR& nm = *reinterpret_cast<NMHDR*>(lp);
			if(nm.idFrom == IDC_OBSERVEDLIST && nm.code == LVN_ITEMCHANGED) {
				BOOL state = ListView_GetSingleSelection(nm.hwndFrom) != -1;

				EnableWindow(GetDlgItem(hwnd, IDC_EDIT), state);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), state);
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
						cfg_watched.add_item(*w);
						delete w;
					}
					else break;
				}
				else if(id == IDC_EDIT && selected != -1) {
					Watched* w = (Watched*) uDialogBox(IDD_ADDEDIT, hwnd, 
						addEditProc, LPARAM(&cfg_watched[selected]));

					if(w != NULL) {
						cfg_watched.replace_item(selected, *w);
						delete w;
					}
					else break;
				}
				else if(id == IDC_REMOVE && selected != -1)
					cfg_watched.remove_by_idx(selected);
				
				loadObservedList(listview);
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
			}
			break;
		}

		case WM_HSCROLL: {
			if(HWND(lp) != GetDlgItem(hwnd, IDC_BUFFSIZE)) break;

			pfc::string8 label;
			label << SendMessage(HWND(lp), TBM_GETPOS, 0, 0) << " kB";

			uSetDlgItemText(hwnd, IDC_SIZELABEL, label);
			break;
		}

		case WM_DESTROY: {
			HWND handle = GetDlgItem(hwnd, IDC_BUFFSIZE);
			cfg_buffsize = SendMessage(handle, TBM_GETPOS, 0, 0);
			
			uGetWindowText(GetDlgItem(hwnd, IDC_RESTRICT), cfg_restrict);
			uGetWindowText(GetDlgItem(hwnd, IDC_EXCLUDE), cfg_exclude);

			onDestroy();
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
		uSetWindowText(hwnd, "Add new observed folder");
		SendDlgItemMessage(hwnd, IDC_TYPE, CB_SETCURSEL, 0, 0);				
	}
	else {
		uSetWindowText(hwnd, "Edit observed folder");
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
	LPCWSTR title = L"Select folder to observe";
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

			delete buffer;
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
		msg << (!msg.is_empty() ? "\n" : "") << "Directory doesn't exist.";

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
			uSendMessageText(combobox, CB_ADDSTRING, 0, "custom");
			uSendMessageText(combobox, CB_ADDSTRING, 0, "active");
			uSendMessageText(combobox, CB_ADDSTRING, 0, "playing");

			combobox = GetDlgItem(hwnd, IDC_PLAYLIST);
			static_api_ptr_t<playlist_manager> pm;

			for(t_size i = 0; i < pm->get_playlist_count(); i++) {
				pfc::string8 playlist;
				pm->playlist_get_name(i, playlist);
				uSendMessageText(combobox, CB_ADDSTRING, 0, playlist);
			}

			if(pm->get_playlist_count())
				SendDlgItemMessage(hwnd, IDC_PLAYLIST, CB_SETCURSEL, 0, 0);			

			SendMessage(hwnd, DM_SETDEFID, IDC_OK, 0);

			initControls(hwnd, (Watched*) lp);
			break;
		}

		case WM_COMMAND:
			if(HIWORD(wp) == BN_CLICKED) {
				WORD id = LOWORD(wp);
				
				if(id == IDC_OPEN)
					openDialog(hwnd);
				else if(id == IDC_OK) {
					Watched* w = generateResult(hwnd);
					if(w != NULL)
						EndDialog(hwnd, INT_PTR(w));
				}
				else if(id == IDC_CANCEL)
					EndDialog(hwnd, NULL);
			}
			else if(HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == IDC_TYPE) {
				BOOL state = SendMessage(HWND(lp), CB_GETCURSEL, 0, 0) == 0;
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
