#pragma once
/*
 * HWIO server
 *
 * hwio is library for simple communication with hardware components
 * This server uses simple protocol over tcp connection to query devices
 * and perform read/write operation on them
 *
 * hwio is not thread safe, this server allows multiple client,
 * uses linux select and runs in single thread
 *
 * */
#include <string.h>   //strlen, strdup
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sstream>     // stringstream
#include <assert.h>
#include <netdb.h>
#include <map>
#include <type_traits>
#include <functional>

#include "hwio_remote.h"
#include "ihwio_dev.h"
#include "ihwio_bus.h"

namespace hwio {

class ClientInfo {
public:
	int id;
	int fd;
	std::vector<ihwio_dev *> devices;
	ClientInfo(int id, int _socket) :
			id(id), fd(_socket), devices() {
	}
	~ClientInfo() {
		if (fd >= 0)
			close(fd);
	}
};

class HwioServer {
private:
	/**
	 * Packet processing result
	 * */
	class PProcRes {
	public:
		bool disconnect;
		size_t tx_size;
		PProcRes(bool _disconnect, size_t _tx_size) :
				disconnect(_disconnect), tx_size(_tx_size) {
		}
	};
	struct addrinfo * addr;
	// socket for accepting clients
	int master_socket;

	// buffers for rx/tx
	char rx_buffer[BUFFER_SIZE];
	char tx_buffer[BUFFER_SIZE];

	std::vector<struct pollfd> poll_fds;
	std::map<int, ClientInfo*> fd_to_client;

	// meta-informations about clients in server
	// some items may be nullptr if client has disconnected
	std::vector<ClientInfo *> clients;

	/*
	 * Parse hwio device spec from device query message
	 * */
	std::vector<hwio_comp_spec> extractSpecFromClientQuery(DevQuery * q,
			int cnt);

	/*
	 * Perform hwio device query for client by its spec from device query message
	 * */
	PProcRes device_lookup_resp(ClientInfo * client, DevQuery * q, int cnt,
			char tx_buffer[BUFFER_SIZE]);
	/*
	 * @return first unused server device record or NULL if all device records are used
	 * */
	int firstEmptyDevInfo();

	/*
	 * Send error message to client
	 * @return PProcRes with size of tx data in tx_buffer and disconnect flag
	 * */
	PProcRes send_err(int errCode, const std::string & msg);

	/*
	 * HWIO hw read operation by read message
	 * @return PProcRes with size of tx data in tx_buffer and disconnect flag
	 * */
	PProcRes handle_read(ClientInfo * client, Hwio_packet_header header);

	/*
	 * HWIO hw write operation by write message
	 * @return PProcRes with size of tx data in tx_buffer and disconnect flag
	 * */
	PProcRes handle_write(ClientInfo * client, Hwio_packet_header header);

	/**
	 * HWIO remote call of plugin function
	 */
	PProcRes handle_remote_call(ClientInfo * client, Hwio_packet_header header);

	/*
	 * Process message from client
	 * @return PProcRes with size of tx data in tx_buffer and disconnect flag
	 * */
	PProcRes handle_msg(ClientInfo * client, Hwio_packet_header header);

	/**
	 * Spot new ClientInfo instance in this->clients
	 **/
	ClientInfo * add_new_client(int socket);

	void handle_client_requests(int client_fd);

	static constexpr unsigned MAX_PENDING_CONNECTIONS = 32;
	static constexpr unsigned POLL_TIMEOUT_MS = 100;

	static ihwio_dev * client_get_dev(ClientInfo * client, dev_id_t devId);

public:
	static const char * DEFAULT_ADDR;
	struct timespec client_timeout;

	enum loglevel_e {logERROR=0, logWARNING=1, logINFO=2, logDEBUG=3};

	enum loglevel_e log_level;
	std::vector<ihwio_bus *> buses;

	using plugin_fn_t = std::function<void (ihwio_dev*, void *, void *)> ;
	struct plugin_info_s {
		plugin_fn_t fn;
		size_t args_size;
		size_t ret_size;
	};
	std::map<const std::string, plugin_info_s> plugins;
	HwioServer(struct addrinfo * addr, std::vector<ihwio_bus *> buses);
	void prepare_server_socket();

	/**
	 * pool once over all sockets and handle messages
	 *
	 * @attention has to be called in cycle in order to have server running
	 */
	void pool_client_msgs();
	size_t get_client_cnt();

	// [TODO] plugin function should be restricted to device class by spec
	template <typename ARGS_T, typename RET_T>
	void install_plugin_fn(const std::string & name, void (*plugin_fn)(ihwio_dev* dev, ARGS_T * args, RET_T * ret)) {
		plugin_info_s p;
		p.fn =  [plugin_fn](ihwio_dev* a, void * b, void * c) {
			plugin_fn(a, reinterpret_cast<ARGS_T*>(b), reinterpret_cast<RET_T*>(c));
		};
		if (std::is_same<ARGS_T, void>::value)
			p.args_size = 0;
		else
			p.args_size = sizeof(ARGS_T);

		if (std::is_same<RET_T, void>::value)
			p.ret_size = 0;
		else
			p.ret_size = sizeof(RET_T);
		plugins[name] = p;
	}
	~HwioServer();
};

}
