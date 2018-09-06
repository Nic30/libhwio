#include "hwio_client_to_server_con.h"

#include <unistd.h>
#include <sstream>

#include "ihwio_dev.h"
#include "hwio_remote_utils.h"

namespace hwio {

#ifdef HWIO_DEFAULT_SERVER_ADDRESS
const char * hwio_client_to_server_con::DEFAULT_SERVER_ADDRESS = HWIO_DEFAULT_SERVER_ADDRESS;
#else
const char * hwio_client_to_server_con::DEFAULT_SERVER_ADDRESS =
		"127.0.0.1:8896";
#endif

const size_t hwio_client_to_server_con::DEV_TIMEOUT = 500000;

hwio_client_to_server_con::hwio_client_to_server_con(std::string host) :
		sockfd(-1), orig_addr(host) {
	addr = parse_ip_and_port(host);
}

void hwio_client_to_server_con::connect_to_server() {
	sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sockfd < 0) {
		throw std::runtime_error("[HWIO] Can not create a socket");
	}

	// set timeout on socket
	struct timeval tv;
	tv.tv_sec = DEV_TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv,
			sizeof(struct timeval));
	int ret;
	ret = connect(sockfd, addr->ai_addr, addr->ai_addrlen);
	if (ret < 0) {
		throw std::runtime_error(
				std::string(
						"[HWIO] Can not connect to server: (error: ") + strerror(ret)
								+ ", address: " + orig_addr + " )");
	}

	ret = ping();
	if (ret < 0) {
		throw std::runtime_error("[HWIO] Initial ping to server has failed");
	}
}

int hwio_client_to_server_con::rx_bytes(size_t size) {
	size_t bytesRead = 0;
	int result;
	while (bytesRead < size) {
		result = read(sockfd, rx_buffer + bytesRead, size - bytesRead);
		if (result <= 0) {
			if (result >= -1 && (errno == EINTR))
				continue;
			perror("rx_bytes");
			return result;
		}
		bytesRead += result;
	}
	return 0;
}

void hwio_client_to_server_con::rx_pckt(Hwio_packet_header * header) {
	if (rx_bytes(sizeof(Hwio_packet_header)))
		throw hwio_error_rw("Only partial data received from server");

	*header = *reinterpret_cast<Hwio_packet_header*>(rx_buffer);
	if (header->body_len)
		if (rx_bytes(header->body_len))
			throw hwio_error_rw("Malformed packet received from server");
}

void hwio_client_to_server_con::tx_pckt() {
	Hwio_packet_header * f = reinterpret_cast<Hwio_packet_header*>(tx_buffer);
	int err = send(sockfd, tx_buffer, f->body_len + sizeof(Hwio_packet_header),
			0);

	if (err < 0) {
		std::stringstream ss;
		ss
				<< "hwio_client_to_server_con::tx_pckt, Unable to send message to server,"
				<< strerror(errno);
		throw hwio_error_rw(ss.str());
	}
}

int hwio_client_to_server_con::ping() {
	Hwio_packet_header * f = reinterpret_cast<Hwio_packet_header*>(tx_buffer);
	f->body_len = 0;
	f->command = HWIO_CMD_PING_REQUEST;
	try {
		tx_pckt();

		Hwio_packet_header resp;
		rx_pckt(&resp);
		if (resp.body_len != 0 || resp.command != HWIO_CMD_PING_REPLY) {
			return -ENAVAIL;
		}
	} catch (const std::runtime_error & err) {
		return -EBUSY;
	}

	return 0;
}

void hwio_client_to_server_con::bye() {
	Hwio_packet_header * f = reinterpret_cast<Hwio_packet_header*>(tx_buffer);
	f->body_len = 0;
	f->command = HWIO_CMD_BYE;
	tx_pckt();
}

hwio_client_to_server_con::~hwio_client_to_server_con() {
	if (sockfd >= 0) {
		bye();
		close(sockfd);
	}
	if (addr != nullptr)
		freeaddrinfo(addr);
}

}
