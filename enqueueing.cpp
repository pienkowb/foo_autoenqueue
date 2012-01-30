#include "enqueueing.h"
#include "watching.h"
#include "foo_autoenqueue.h"

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <vector>
#include <queue>
#include <string>

using namespace std;

//------------------------------------------------------------------------------

queue< pair<UINT, wstring> > files;
static const int waitTime = 10;

//------------------------------------------------------------------------------

class enqueuer : public main_thread_callback {
	static_api_ptr_t<playlist_manager> pm;
	pfc::list_t<metadb_handle_ptr> items;
	t_uint32 index;
public:
	enqueuer(pfc::list_t<metadb_handle_ptr> it, UINT i) : items(it), index(i) {}
	t_size getPlaylist(pfc::string8 name) {
		if(name.is_empty()) 
			return infinite;

		if(name[0] == '0') {
			name.remove_chars(0, 1);
			return pm->find_or_create_playlist(name);
		}
		else if(name[0] == '1')
			return pm->get_active_playlist();
		else if(name[0] == '2')
			return pm->get_playing_playlist();
		else 
			return infinite;
	}
	void callback_run() {
		if(!items.get_count()) return;

		if(watched[index].requireConfirm) {
			pfc::string8 msg;
			filesystem::g_get_display_path(items[0]->get_path(), msg);
			msg << "\n" << "Do you confirm adding this item?";

			int result = uMessageBox(core_api::get_main_window(), msg, 
				"Autoenqueuer", MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);

			if(result == IDNO) return;
		}

		static_api_ptr_t<metadb_io_v2> miv;
		miv->load_info_async(items, metadb_io::load_info_default, NULL,
			metadb_io_v2::op_flag_no_errors, NULL);

		t_size playlist = getPlaylist(watched[index].playlist);
		if(playlist == infinite) return;
		
		pm->set_active_playlist(playlist);
		pm->playlist_add_items(playlist, items, bit_array_true());

		t_size count = pm->playlist_get_item_count(playlist);
		pm->playlist_ensure_visible(playlist, count - 1);

		if(watched[index].autoPlay)
			pm->playlist_execute_default_action(playlist, count - 
				items.get_count());
	}
};

//------------------------------------------------------------------------------

DWORD WINAPI EnqueueingProc(LPVOID) {
	while(watching) {
		WaitForSingleObject(mutex, INFINITE);
		vector< pair<UINT, wstring> > waiting;

		while(!files.empty()) {			
			pair<UINT, wstring> item = files.front();

			pfc::string8 path = watched[item.first].directory;
			path.replace_char('/', '\\');
			path.fix_dir_separator('\\');

			using namespace pfc::stringcvt;
			path.add_string(string_utf8_from_wide(item.second.c_str()));

			try {
				abort_callback_impl abort;

				if(filesystem::g_exists(path, abort)) {
					filesystem::g_get_canonical_path(path, path);
				
					if(!input_entry::g_is_supported_path(path))
						throw exception_aborted();

					if(!wildcard_helper::test_path(path, cfg_restrict, true))
						throw exception_aborted();
					if(wildcard_helper::test_path(path, cfg_exclude, true))
						throw exception_aborted();

					service_ptr_t<input_info_reader> iir;
					input_entry::g_open_for_info_read(iir, 0, path, abort);

					if(iir->get_file_stats(abort).m_size == 0)
						throw exception_io_data();

					pfc::list_t<metadb_handle_ptr> items;
					
					const t_uint32 count = iir->get_subsong_count();
					for(t_uint32 i = 0; i < count; i++) {
						abort.check();
						
						static_api_ptr_t<metadb> m;
						metadb_handle_ptr handle;
						m->handle_create(handle, make_playable_location(path, 
							iir->get_subsong(i)));

						items.add_item(handle);
					}

					static_api_ptr_t<main_thread_callback_manager> mtcm;
					mtcm->add_callback(new service_impl_t<enqueuer>(items, 
						item.first));
				}
			} catch(exception_io_data&) {
				waiting.push_back(item);
			} catch(exception_io_sharing_violation&) {
				waiting.push_back(item);
			} catch(exception_aborted&) {
				// ignoring
			} catch(pfc::exception& e) {
				pfc::string8 dpath;
				filesystem::g_get_display_path(path, dpath);

				console::formatter() << e.what() << ": \"" << dpath << "\"";
			}

			files.pop();
		}
		
		vector< pair<UINT, wstring> >::iterator it = waiting.begin();
		for(; it != waiting.end(); it++)
			files.push(*it);

		ReleaseMutex(mutex);
		Sleep(waitTime);
	}

	return 0;
}
