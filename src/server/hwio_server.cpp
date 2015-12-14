#include "hwio_server.h"
#include "hwio_remote_utils.h"

#include <sys/time.h>


using namespace std;
using namespace hwio;


Hwio_server::Hwio_server(struct addrinfo * addr, std::vector<ihwio_bus *> buses) :
		addr(addr), master_socket(-1), buses(buses) {
	//addr->ai_flags = AI_PASSIVE;
}

const int Hwio_server::MAX_PENDING_CONNECTIONS = 32;
const int Hwio_server::SELECT_TIMEOUT_MS = 100;

void Hwio_server::prepare_server_socket() {
	struct sockaddr_in * addr = ((struct sockaddr_in *) this->addr->ai_addr);
	int opt = true;
	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		std::stringstream errss;
		errss << "hwio_server socket failed: " << gai_strerror(master_socket);
		throw std::runtime_error(errss.str());
	}

	// set master socket to allow multiple connections ,
	// also reuse socket after previous application crashed
	int err = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt));
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server setsockopt failed: "
				<< gai_strerror(master_socket);
		throw std::runtime_error(errss.str());
	}
	//bind the socket
	err = bind(master_socket, (sockaddr *) addr, sizeof(*addr));
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server bind failed: " << gai_strerror(err);
		throw std::runtime_error(errss.str());
	}

	err = listen(master_socket, MAX_PENDING_CONNECTIONS);
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server listen failed: " << gai_strerror(err);
		throw std::runtime_error(errss.str());
	}

	// now can accept the incoming connection
#ifdef LOG_INFO
	LOG_INFO << "Hwio server waiting for connections ..." << endl;
#endif

}

Hwio_server::PProcRes Hwio_server::handle_msg(ClientInfo * client,
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
		std::cout << "HWIO_CMD_MSG" << std::endl;

		errM = reinterpret_cast<ErrMsg*>(rx_buffer);
		errM->msg[MAX_NAME_LEN - 1] = 0;
#ifdef LOG_INFO
		LOG_ERR << "code:" << errM->err_code << ": " << errM->msg << endl;
#endif
		return PProcRes(false, 0);

	default:
		ss << "Unknown command " << cmd;
		return send_err(UNKNOWN_COMMAND, ss.str());
	}

}

ClientInfo * Hwio_server::add_new_client(int socket) {
	int id = 0;
	for (auto & client : clients) {
		//if position is empty
		if (client == nullptr) {
			client = new ClientInfo(id, socket);
			client->id = id;
#ifdef LOG_INFO
			LOG_INFO << "Adding to list of sockets as " << id << endl;
#endif
			return client;
		}
		id++;
	}
	auto client = new ClientInfo(id, socket);
	clients.push_back(client);
	return client;
}

void Hwio_server::handle_client_msgs(bool * run_server) {
	//set of socket descriptors
	fd_set readfds;
	int max_sd;
	int sd;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int new_socket;
	struct timespec timeout;
	memset(&timeout, 0, sizeof(timeout));
	timeout.tv_sec = SELECT_TIMEOUT_MS / 1000;
	timeout.tv_nsec = (SELECT_TIMEOUT_MS - timeout.tv_sec * 1000) / 1000;

	while (*run_server) {
		// clear the socket set
		FD_ZERO(&readfds);

		// add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		// add child sockets to set
		for (ClientInfo * client : clients) {
			if (client == nullptr) {
				continue;
			}
			// socket descriptor
			sd = client->socket;

			// if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			// highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		// wait for an activity on one of the sockets , timeout is NULL ,
		// so wait indefinitely
		if ((pselect(max_sd + 1, &readfds, NULL, NULL, &timeout, NULL) < 0)
				&& (errno != EINTR)) {
			LOG_ERR << "pselect error" << endl;
		}

		// If something happened on the master socket,
		// then its an incoming connection and we need to spot new client info instance
		if (FD_ISSET(master_socket, &readfds)) {
			if ((new_socket = accept(master_socket,
					(struct sockaddr *) &address, (socklen_t*) &addrlen)) < 0)
				throw runtime_error("error in accept for client socket");

#ifdef LOG_INFO
			//inform user of socket number - used in send and receive commands
			LOG_INFO << "New connection, ip:" << inet_ntoa(address.sin_addr)
					<< ", port:" << ntohs(address.sin_port) << endl;
#endif
			add_new_client(new_socket);
		} else {
			handle_client_requests(&readfds);
		}
	}
}
void Hwio_server::handle_client_requests(fd_set * readfds) {
	// else its some IO operation on some other socket
	for (ClientInfo * client : clients) {
		if (client == nullptr)
			continue;
		int sd = client->socket;

		if (FD_ISSET(sd, readfds)) {
			//Check if it was for closing , and also read the
			//incoming message
			size_t rx_data_size;
			PProcRes respMeta(true, 0);
			Hwio_packet_header header;
			if ((rx_data_size = read(sd, &header, sizeof(Hwio_packet_header)))
					== 0) {
				respMeta = send_err(MALFORMED_PACKET, "No command received");
			} else {
				if (header.body_len) {
					if ((rx_data_size = read(sd, rx_buffer, header.body_len))
							== 0)
						respMeta = send_err(MALFORMED_PACKET,
								"Wrong frame body");
				}
				respMeta = handle_msg(client, header);
				if (respMeta.tx_size) {
					if (send(sd, tx_buffer, respMeta.tx_size, 0) < 0) {
						throw std::runtime_error(
								"Can not send response on client msg");
					}
				}
			}

			if (respMeta.disconnect) {
				//Somebody disconnected , get his details and print
#ifdef LOG_INFO
				LOG_INFO << "Client " << client->id << " disconnected" << endl;

				for (auto d : client->devices) {
					LOG_INFO << "    owned device:" << endl;
					for (auto & s : d->get_spec())
						LOG_INFO << "        " << s.to_str() << endl;
				}
#endif
				// Close the socket and mark as 0 in list for reuse
				// packet had wrong format or connection was disconnected.
				clients[client->id] = nullptr;
				delete client;
			}
		}
	}
}

Hwio_server::~Hwio_server() {
#ifdef LOG_INFO
	LOG_INFO << "Hwio server shutting down" << std::endl;
#endif
	for (auto & c : clients) {
		delete c;
	}
	if (master_socket >= 0)
		close(master_socket);
}
