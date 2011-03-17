#include "jack_client.h"

namespace ladtl {
	std::vector<jack_client_base*> jack_client_base::m_client_bases;
	const int jack_client_base::m_shutdown_signal = 2;
} // namespace