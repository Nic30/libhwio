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
#include <netinet/in.h>
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <sstream>     // stringstream
#include <assert.h>
#include <netdb.h>

#include "hwio_remote.h"
#include "ihwio_dev.h"
#include "ihwio_bus.h"

namespace hwio {

class ClientInfo {
public:
	int id;
	int socket;
	std::vector<ihwio_dev *> devices;
	ClientInfo(int id, int _socket) :
			id(id), socket(_socket), devices() {
	}
	~ClientInfo() {
		if (socket >= 0)
			close(socket);
	}
};

class Hwio_server {
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

	/*
	 * Process message from client
	 * @return PProcRes with size of tx data in tx_buffer and disconnect flag
	 * */
	PProcRes handle_msg(ClientInfo * client, Hwio_packet_header header);

	/**
	 * Spot new ClientInfo instance in this->clients
	 **/
	ClientInfo * add_new_client(int socket);

	void handle_client_requests(fd_set * readfds);

	static const int MAX_PENDING_CONNECTIONS;
	static const int SELECT_TIMEOUT_MS;

public:
	std::vector<ihwio_bus *> buses;
	Hwio_server(struct addrinfo * addr, std::vector<ihwio_bus *> buses);
	void prepare_server_socket();
	void handle_client_msgs(bool * run_server);
	~Hwio_server();
};

}
