#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "hwio_remote.h"

namespace hwio {

class interupt_recieved: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class hwio_client_to_server_con {
	int sockfd;
	struct addrinfo * addr;
	int rx_bytes(size_t size);
	void bye();

public:
	std::string orig_addr;
	static const char * DEFAULT_SERVER_ADDRESS;
	static const size_t DEV_TIMEOUT;

	uint8_t rx_buffer[BUFFER_SIZE];
	uint8_t tx_buffer[BUFFER_SIZE];

	hwio_client_to_server_con(std::string host);

	/**
	 * @throw runtime_error
	 * */
	void connect_to_server();

	int ping();
	void tx_pckt();
	void rx_pckt(Hwio_packet_header * header);

	~hwio_client_to_server_con();
};

}
