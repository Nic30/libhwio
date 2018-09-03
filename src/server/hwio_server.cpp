#include "hwio_server.h"
#include "hwio_remote_utils.h"

#include <algorithm>

using namespace std;
using namespace hwio;

HwioServer::HwioServer(struct addrinfo * addr, std::vector<ihwio_bus *> buses) :
		addr(addr), master_socket(-1), log_level(logWARNING), buses(buses) {
}

void HwioServer::prepare_server_socket() {
	struct sockaddr_in * addr = ((struct sockaddr_in *) this->addr->ai_addr);
	int opt = true;
	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		std::stringstream errss;
		errss << "hwio_server socket failed: " << gai_strerror(master_socket);
		throw std::runtime_error(std::string("[HWIO, server]") + errss.str());
	}

	// set master socket to allow multiple connections ,
	// also reuse socket after previous application crashed
	int err = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt));
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server setsockopt failed: "
				<< gai_strerror(master_socket);
		throw std::runtime_error(std::string("[HWIO, server]") + errss.str());
	}
	//bind the socket
	err = bind(master_socket, (sockaddr *) addr, sizeof(*addr));
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server bind failed: " << gai_strerror(err);
		throw std::runtime_error(std::string("[HWIO, server]") + errss.str());
	}

	err = listen(master_socket, MAX_PENDING_CONNECTIONS);
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server listen failed: " << gai_strerror(err);
		throw std::runtime_error(std::string("[HWIO, server]") + errss.str());
	}

	struct pollfd pfd;
	pfd.fd = master_socket;
	pfd.events = POLLIN;
	poll_fds.push_back(pfd);

	// now can accept the incoming connection
	if (log_level >= logDEBUG)
		std::cout << "[DEBUG] Hwio server waiting for connections ..." << endl;

}

HwioServer::PProcRes HwioServer::handle_msg(ClientInfo * client,
		Hwio_packet_header header) {
	ErrMsg * errM;
	DevQuery * q;
	Hwio_packet_header * txHeader;
	int cnt;
	stringstream ss;

	HWIO_CMD cmd = static_cast<HWIO_CMD>(header.command);

	switch (cmd) {
	case HWIO_CMD_READ:
		return handle_read(client, header);

	case HWIO_CMD_WRITE:
		return handle_write(client, header);

	case HWIO_CMD_REMOTE_CALL:
		return handle_remote_call(client, header);

	case HWIO_CMD_PING_REQUEST:
		if (header.body_len != 0)
			return send_err(MALFORMED_PACKET, "ECHO_REQUEST: size has to be 0");
		txHeader = reinterpret_cast<Hwio_packet_header*>(tx_buffer);
		txHeader->command = HWIO_CMD_PING_REPLY;
		txHeader->body_len = 0;
		return PProcRes(false, sizeof(Hwio_packet_header));

	case HWIO_CMD_QUERY:
		q = reinterpret_cast<DevQuery*>(rx_buffer);
		cnt = header.body_len / sizeof(Dev_query_item);
		if (header.body_len != cnt * sizeof(Dev_query_item)) {
			ss << "QUERY: wrong size of packet " << header.body_len
					<< "(not divisible by " << sizeof(Dev_query_item) << ")";
			return send_err(MALFORMED_PACKET, ss.str());
		}

		if (cnt == 0 || cnt > MAX_ITEMS_PER_QUERY) {
			ss << "Unsupported number of queries (" << cnt << ")" << endl;
			return send_err(UNKNOWN_COMMAND, ss.str());
		}
		return device_lookup_resp(client, q, cnt, tx_buffer);

	case HWIO_CMD_BYE:
		return PProcRes(true, 0);

	case HWIO_CMD_MSG:
		errM = reinterpret_cast<ErrMsg*>(rx_buffer);
		errM->msg[MAX_NAME_LEN - 1] = 0;
		if (log_level >= logERROR)
			LOG_ERR << "code:" << errM->err_code << ": " << errM->msg << endl;
		return PProcRes(false, 0);

	default:
		if (log_level >= logERROR)
			LOG_ERR << "Unknown command " << cmd;
		return send_err(UNKNOWN_COMMAND, ss.str());
	}
}

ClientInfo * HwioServer::add_new_client(int socket) {
	unsigned id = 0;
	for (auto & client : clients) {
		//if position is empty
		if (client == nullptr) {
			break;
		}
		id++;
	}
	ClientInfo * client = new ClientInfo(id, socket);
	if (id < clients.size())
		clients[id] = client;
	else
		clients.push_back(client);

	assert(fd_to_client.find(socket) == fd_to_client.end());
	fd_to_client[socket] = client;
	struct pollfd pfd;
	pfd.events = POLLIN;
	pfd.fd = socket;
	poll_fds.push_back(pfd);
	return client;
}

void HwioServer::handle_client_msgs(bool * run_server) {
	while (*run_server) {
		// wait for an activity on one of the sockets , timeout is NULL ,
		// so wait indefinitely
		int err = poll(&poll_fds[0], poll_fds.size(), POLL_TIMEOUT_MS);
		if (err == 0) {
			continue;
		} else if (err < 0) {
			if (log_level >= logERROR)
				LOG_ERR << "poll error" << endl;
			continue;
		}

		// If something happened on the master socket,
		// then its an incoming connection and we need to spot new client info instance
		for (auto & fd : poll_fds) {
			if (fd.revents == 0)
				continue;

			if (fd.fd == master_socket) {
				struct sockaddr_in address;
				int addrlen = sizeof(address);
				int new_socket;
				if ((new_socket = accept(master_socket,
						(struct sockaddr *) &address, (socklen_t*) &addrlen))
						< 0)
					throw runtime_error("error in accept for client socket");

				if (log_level >= logINFO) {
					//inform user of socket number - used in send and receive commands
					std::cout << "[INFO] New connection, ip:"
							<< inet_ntoa(address.sin_addr) << ", port:"
							<< ntohs(address.sin_port) << endl;
				}
				add_new_client(new_socket);
			} else {
				handle_client_requests(fd.fd);
			}
		}
	}
}
void HwioServer::handle_client_requests(int sd) {
	// else its some IO operation on some other socket
	//Check if it was for closing , and also read the
	//incoming message
	std::map<int, ClientInfo*>::iterator _client = fd_to_client.find(sd);
	ClientInfo * client = nullptr;
	if (_client == fd_to_client.end()) {
		client = add_new_client(sd);
	} else {
		client = _client->second;
	}

		//throw std::runtime_error(
		//		std::string(
		//				"[HWIO, server] fd_to_client does not know about socket for client on socket:")
		//				+ to_string(sd));

	assert(client->fd == sd);

	size_t rx_data_size;
	PProcRes respMeta(true, 0);
	Hwio_packet_header header;
	if ((rx_data_size = read(sd, &header, sizeof(Hwio_packet_header))) == 0) {
		respMeta = send_err(MALFORMED_PACKET, "No command received");
	} else {
		bool err = false;
		if (header.body_len) {
			if ((rx_data_size = read(sd, rx_buffer, header.body_len)) == 0) {
				respMeta = send_err(MALFORMED_PACKET,
						std::string("Wrong frame body (expectedSize=" + std::to_string(int(header.body_len))
									+ " got="+ std::to_string(rx_data_size)));
				err = true;
			}
		}
		if (!err) {
			respMeta = handle_msg(client, header);
			if (respMeta.tx_size) {
				if (send(sd, tx_buffer, respMeta.tx_size, 0) < 0) {
					throw std::runtime_error("[HWIO, server] Can not send response on client msg");
				}
			}
		}
	}

	if (respMeta.disconnect) {
		// Somebody disconnected, get his details and print
		// packet had wrong format or connection was disconnected.
		if (log_level >= logINFO) {
			std::cout << "[INFO] " << "Client " << client->id << " disconnected" << endl;

			for (auto d : client->devices) {
				std::cout << "    owned device:" << endl;
				for (auto & s : d->get_spec())
					std::cout << "        " << s.to_str() << endl;
			}
		}
		clients[client->id] = nullptr;
		fd_to_client.erase(sd);
		poll_fds.erase(
				std::remove_if(poll_fds.begin(), poll_fds.end(),
						[sd](struct pollfd item) {
							return item.fd == sd;
						}));
		//close(sd); in desctructor
		delete client;
	}
}

HwioServer::~HwioServer() {
	if (log_level >= logINFO)
		std::cout << "[INFO] Hwio server shutting down" << std::endl;

	for (auto & c : clients) {
		delete c;
	}
	if (master_socket >= 0)
		close(master_socket);
}
