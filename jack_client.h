#ifndef LADTL_JACK_CLIENT_HH
#define LADTL_JACK_CLIENT_HH

#include <jack/jack.h>
#include <signal.h>
#include <string>
#include <vector>
#include <sstream>
#include <eigen2/Eigen/Core>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

namespace ladtl {

	inline void default_process(const Eigen::MatrixXd&in, Eigen::MatrixXd &out) {
		out = in;
	}

	template<class T> 
	static inline int jack_client_process_callback(jack_nframes_t nframes, void *arg) {
		return ((T*)arg)->process_callback(nframes);
	}
 
	struct jack_client_base {
	 jack_client_t *m_client;
	 bool m_shutdown;

	jack_client_base() : m_shutdown(false) {
		if (m_client_base == 0) m_client_base = this;
		else throw std::runtime_error("only one client may exist at a time..");
	}

	virtual void shutdown() {
		std::cout << "shutting down" << std::endl;
		jack_deactivate(m_client);
		m_shutdown = true;
	}
	static jack_client_base *m_client_base;

	};

	template<class T>
	void shutdown_handler(int) {
		std::cout << "shutdown" << std::endl;
		jack_client_base::m_client_base->shutdown();
	}

	template<class T>
	void default_signal_handler(int) { 
	}
 
	template 
	<
		class process = void(*)(const Eigen::MatrixXd&, Eigen::MatrixXd &),
		unsigned int in_channels = 2, 
		unsigned int out_channels = 2,
		class signal_handler = void(*)(int)
   > 
	struct jack_client : 
		public jack_client_base 
	{  
		jack_client(
			const std::string &name = "jack_client", 
			process p = default_process,
			signal_handler sh = default_signal_handler<jack_client_type>
		) :
			m_name(name),
			m_process(p)
		{
			m_client = jack_client_open(name.c_str(), JackNullOption, 0);
			if (m_client == 0) throw std::runtime_error("failed to register client " + name);
		
			m_in_ports.resize(in_channels);
			m_out_ports.resize(out_channels);
		
			for (unsigned int i = 0; i < in_channels; ++i) 
			{
				std::stringstream s; s << i;
				m_in_ports[i] = jack_port_register(m_client, ("input: " + s.str()).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
				if (m_in_ports[i] == 0) throw std::runtime_error(std::string("failed to register input port ") + s.str());
			}
		
			for (unsigned int i = 0; i < out_channels; ++i)
			{
				std::stringstream s; s << i;
				m_out_ports[i] = jack_port_register(m_client, ("output: " + s.str()).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
				if (m_out_ports[i] == 0) throw std::runtime_error(std::string("failed to register output port ") + s.str());
			}
		
			m_original_signal_handler = signal(9, sh);
	  
			int frames = jack_get_buffer_size(m_client);
			in = Eigen::MatrixXd((int)in_channels, (int)frames);
			out = Eigen::MatrixXd((int)out_channels, (int)frames);

			jack_set_process_callback(
				m_client, 
				jack_client_process_callback<jack_client_type>, 
				this
			);
		}
		  
		void operator()() {
			jack_activate(m_client);
			while (!m_shutdown) sleep(1);
		}

		~jack_client() {
			jack_client_close(m_client);
			signal(9, m_original_signal_handler);
		}

		int process_callback(jack_nframes_t nframes) {
			for (unsigned int channel = 0; channel < in_channels; ++channel) {
				double *buffer = (double*)jack_port_get_buffer(m_in_ports[channel], nframes);
				for (unsigned int frame = 0; frame < nframes; ++frame) {
					in(channel, frame) = buffer[frame];
				}
			}
		
			// do the deed
			m_process(in, out);

			for (unsigned int channel = 0; channel < out_channels; ++channel) {
				double *buffer = (double*)jack_port_get_buffer(m_out_ports[channel], nframes);
				for (unsigned int frame = 0; frame < nframes; ++frame) {
					buffer[frame] = out(channel, frame);
				}
			}
			return 0;
		} // process_callback

		typedef jack_client<process, in_channels, out_channels, signal_handler> jack_client_type;

		protected:
			std::string m_name;
			std::vector<jack_port_t*> m_in_ports;
			std::vector<jack_port_t*> m_out_ports;
			void(*m_original_signal_handler)(int);
			Eigen::MatrixXd in;
			Eigen::MatrixXd out;

			process m_process;
	}; // jack_client

} // namespace

#endif
