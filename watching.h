#ifndef WATCHING_H
#define WATCHING_H

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include <vector>

//------------------------------------------------------------------------------

struct Watched {
	pfc::string8 directory;
	pfc::string8 playlist;
	bool watchSubtree;
	bool requireConfirm;
	bool autoPlay;

	bool operator==(const Watched& other) const {
		return (directory == other.directory
			&& playlist == other.playlist
			&& watchSubtree == other.watchSubtree
			&& requireConfirm == other.requireConfirm
			&& autoPlay == other.autoPlay);
	}

	bool operator!=(const Watched& other) const {
		return !(*this == other);
	}
};

//------------------------------------------------------------------------------

class cfg_watchedlist : public cfg_var, public pfc::list_t<Watched> {
public:
	void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
		t_uint32 n, m = get_count();
		p_stream->write_lendian_t(m, p_abort);

		for(n = 0; n < m; n++) {
			p_stream->write_string(m_buffer[n].directory, p_abort);
			p_stream->write_string(m_buffer[n].playlist, p_abort);
			p_stream->write_object_t<bool>(m_buffer[n].watchSubtree, p_abort);
			p_stream->write_object_t<bool>(m_buffer[n].requireConfirm, p_abort);
			p_stream->write_object_t<bool>(m_buffer[n].autoPlay, p_abort);
		}
	}

	void set_data_raw(stream_reader* p_stream, t_size, abort_callback& p_abort) {
		t_uint32 n, count;
		p_stream->read_lendian_t(count, p_abort);
		m_buffer.set_size(count);

		for(n = 0; n < count; n++) {
			try {
				p_stream->read_string(m_buffer[n].directory, p_abort);
				p_stream->read_string(m_buffer[n].playlist, p_abort);
				p_stream->read_object_t<bool>(m_buffer[n].watchSubtree, p_abort);
				p_stream->read_object_t<bool>(m_buffer[n].requireConfirm, p_abort);
				p_stream->read_object_t<bool>(m_buffer[n].autoPlay, p_abort);
			}
			catch(...) { m_buffer.set_size(0); throw; }
		}
	}
public:
	cfg_watchedlist(const GUID & p_guid) : cfg_var(p_guid) {}
};

//------------------------------------------------------------------------------

extern std::vector<Watched> watched;
DWORD WINAPI WatchingProc(LPVOID lpParameter);

#endif // WATCHING_H
