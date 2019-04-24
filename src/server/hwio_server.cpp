#include "hwio_server.h"
#include "hwio_remote_utils.h"

#include <algorithm>
#include <stdexcept>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;
using namespace hwio;

const char * HwioServer::DEFAULT_ADDR = "0.0.0.0:8896";

HwioServer::HwioServer(struct addrinfo * addr, std::vector<ihwio_bus *> buses) :
		addr(addr), master_socket(-1), log_level(logWARNING), buses(buses) {
	client_timeout.tv_nsec = 1000 * POLL_TIMEOUT_MS;
	client_timeout.tv_sec = POLL_TIMEOUT_MS / 1000;
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
		errss << "hwio_server setsockopt SO_REUSEADDR failed: "
				<< gai_strerror(master_socket);
		throw std::runtime_error(std::string("[HWIO, server]") + errss.str());
	}
	int val = 1;
	err = setsockopt(master_socket, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof val);
	if (err < 0) {
		std::stringstream errss;
		errss << "hwio_server setsockopt SO_KEEPALIVE failed: "
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
	case HWIO_CMD_REMOTE_CALL_FAST:
		return handle_fast_remote_call(client, header);
            
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

	case HWIO_CMD_GET_REMOTE_CALL_ID:
		return handle_get_rpc_fn_id(client, header);
                
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
	pfd.revents = 0;
	poll_fds.push_back(pfd);
	return client;
}

void HwioServer::remove_client(int socket) {
	std::map<int, ClientInfo*>::iterator _client = fd_to_client.find(socket);
	ClientInfo * client = nullptr;
	if (_client == fd_to_client.end()) {
		if (log_level >= logWARNING) {
			std::cout << "[WARNING] " << "Socket: " << socket << " is not in client database." << endl;
		}
		return;
	} else {
		client = _client->second;
	}
	assert(client->fd == socket);
	clients[client->id] = nullptr;
	fd_to_client.erase(socket);
	delete client;
}

size_t HwioServer::get_client_cnt() {
	size_t i = 0;
	for (auto c : clients) {
		if (c != nullptr)
			i++;
	}
	assert(i == fd_to_client.size());
	return i;
}

void HwioServer::pool_client_msgs() {
	//sigset_t origmask;
	//sigprocmask(0, nullptr, &origmask);

	// wait for an activity on one of the sockets , timeout is NULL ,
	// so wait indefinitely
	int err = ppoll(&poll_fds[0], poll_fds.size(), &client_timeout, nullptr);
	if (err == 0) {
		// timeout
		return;
	} else if (err < 0) {
		if (errno == EINTR) {
			if (log_level >= logINFO)
				LOG_ERR << "ppoll interrupted by signal" << endl;
			return;
		}
		if (log_level >= logERROR)
			LOG_ERR << "ppoll error" << endl;
		throw std::runtime_error("Error condition upon execution of ppoll()! errno = " + std::to_string(errno));
	}

	// If something happened on the master socket,
	// then its an incoming connection and we need to spot new client info instance
	for (auto & fd : poll_fds) {
		auto e = fd.revents;
		if (fd.fd < 0 || fd.revents == 0)
			continue;

		if (fd.revents != POLLIN)
			std::cerr << "fd:" << fd.fd << " revents:" << fd.revents
					<< std::endl;

		if ((e & POLLERR) | (e & POLLHUP) | (e & POLLNVAL)) {
			if (fd.fd == master_socket) {
				LOG_ERR << "Error on master socket" << std::endl;
				throw std::runtime_error("Error condition on master socket! fd.revents = " + std::to_string(e));
			} else {
				LOG_ERR << "Error on socket" << fd.fd << std::endl;
				remove_client(fd.fd);
				removed_poll_fds.push_back(fd.fd);
			}
		} else if (fd.fd == master_socket) {
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
						<< ntohs(address.sin_port) << " socket:"
						<< new_socket << endl;
			}
			add_new_client(new_socket);
		} else {
			handle_multiple_client_requests(fd.fd);
		}
	}
	if (removed_poll_fds.size() > 0) {
		auto new_poll_fds = std::vector<struct pollfd>();
		// Clean up after fd error condition
		for (auto & rfd : removed_poll_fds) {
			for (auto fd : poll_fds) {
				if (fd.fd == rfd) {
					continue;
				}
				new_poll_fds.push_back(fd);
			}
		}
		poll_fds.swap(new_poll_fds);
		removed_poll_fds.clear();
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
	//std::cout << "handle_client_requests:" << sd << " " << client->id
	//		<< std::endl;
	assert(client->fd == sd);

	PProcRes respMeta(true, 0);
	Hwio_packet_header header;

	int rx_data_size = 0;
	uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&header);
	while (true) {
		errno = 0;
		int s = recv(sd, data_ptr+rx_data_size, sizeof(Hwio_packet_header) - rx_data_size, MSG_DONTWAIT);
		if (s < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
				continue;
			} else {
				// process as error
				rx_data_size = 0;
				break;
			}
		} else if (s == 0) {
			// process as error
			rx_data_size = 0;
			break;
		} else {
			rx_data_size += s;
			if (rx_data_size == sizeof(Hwio_packet_header))
				break;
		}
	}
	if (rx_data_size != 0) {
		bool err = false;
		data_ptr = reinterpret_cast<uint8_t*>(&rx_buffer);
		rx_data_size = 0;
		if (header.body_len) {
			while (true) {
				int s = recv(sd, data_ptr + rx_data_size, header.body_len - rx_data_size, MSG_DONTWAIT);
				if (s < 0) {
					if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
						continue;
					} else {
						// process as error
						rx_data_size = 0;
						break;
					}
				} else if (s == 0) {
					// process as error
					rx_data_size = 0;
					break;
				} else {
					rx_data_size += s;
					if (rx_data_size == header.body_len)
						break;
				}
			}
			if (rx_data_size == 0) {
				respMeta = send_err(MALFORMED_PACKET,
						std::string(
								"Wrong frame body (expectedSize="
										+ std::to_string(int(header.body_len))
										+ " got="
										+ std::to_string(rx_data_size)));
				err = true;
			}
		}
		if (!err) {
			respMeta = handle_msg(client, header);
			if (respMeta.tx_size) {
				size_t bytesWr = 0;
				int result;
				while (bytesWr < respMeta.tx_size) {
					result = send(sd, tx_buffer + bytesWr, respMeta.tx_size - bytesWr, 0);
					if (result < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
							continue;
						}
						respMeta = PProcRes(true, 0);
						if (log_level >= logERROR) {
							std::cerr
									<< "[HWIO, server] Can not send response to client "
									<< (client->id) << " (socket=" << (client->fd)
									<< ")" << std::endl;
							break; 
						}
					}
					bytesWr += result;
                                }
			}
		}
	}

	if (respMeta.disconnect) {
		// Somebody disconnected, get his details and print
		// packet had wrong format or connection was disconnected.
		if (log_level >= logINFO) {
			std::cout << "[INFO] " << "Client " << client->id << " socket:"
					<< client->fd << " disconnected" << endl;

			for (auto d : client->devices) {
				std::cout << "    owned device:" << endl;
				for (auto & s : d->get_spec())
					std::cout << "        " << s.to_str() << endl;
			}
		}
		remove_client(sd);
		removed_poll_fds.push_back(sd);
	}
}

bool HwioServer::read_from_socket(ClientInfo * client) {
    char * rd_ptr = client->rx_buffer.buffer;
    size_t rd_len = RxBuffer::RX_BUFFER_SIZE;
    
    if (client->rx_buffer.curr_len > 0) {
        if (client->rx_buffer.curr_ptr != client->rx_buffer.buffer) {
           memcpy(client->rx_buffer.buffer, client->rx_buffer.curr_ptr, client->rx_buffer.curr_len);
        }
        rd_ptr += client->rx_buffer.curr_len;
        rd_len -= client->rx_buffer.curr_len;
    }
    client->rx_buffer.curr_ptr = client->rx_buffer.buffer;
//     std::cout << "read_from_socket:" << (void*)client->rx_buffer.buffer << " " << (void*)rd_ptr << " " << rd_len << std::endl;
    while (true) {
        errno = 0;
        int s = recv(client->fd, rd_ptr, rd_len, MSG_DONTWAIT);
        if (s > 0) {
            client->rx_buffer.curr_len += s;
//             std::cout << "read_from_socket:" << s << " " << client->rx_buffer.curr_len << std::endl;
            return true;
        } else {
            if (s < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    continue;
                } else {
                    // process as error
                    return false;
                }
            } else {
                // process as error
                return false;
            }
        }
    }
    // Should never happen
    return false;
}

void HwioServer::parse_msgs(ClientInfo * client) {
    while (client->rx_buffer.curr_len >= sizeof(Hwio_packet_header)) {
        Hwio_packet_header *header = reinterpret_cast<Hwio_packet_header*>(client->rx_buffer.curr_ptr);
//         std::cout << "parse_msgs:" << (void*)header << " " << (void*)client->rx_buffer.curr_ptr << " " << client->rx_buffer.curr_len << std::endl;
        PProcRes respMeta(true, 0);
        size_t msg_len = sizeof(Hwio_packet_header) + header->body_len;
        if (msg_len <= client->rx_buffer.curr_len) {
            rx_buffer = client->rx_buffer.curr_ptr + sizeof(Hwio_packet_header);
//             std::cout << "parse_msgs:" << msg_len << " " <<  (int)header->command << " " << header->body_len << " " << (void*)rx_buffer << std::endl;
            respMeta = handle_msg(client, *header);
            if (respMeta.tx_size) {
                size_t bytesWr = 0;
                int result;
                while (bytesWr < respMeta.tx_size) {
                    result = send(client->fd, tx_buffer + bytesWr, respMeta.tx_size - bytesWr, 0);
                    if (result < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                            continue;
                        }
                        respMeta = PProcRes(true, 0);
                        if (log_level >= logERROR) {
                            std::cerr
                                    << "[HWIO, server] Can not send response to client "
                                    << (client->id) << " (socket=" << (client->fd)
                                    << ")" << std::endl;
                            break; 
                        }
                    }
                    bytesWr += result;
                }
            }
            client->rx_buffer.curr_ptr += msg_len;
            client->rx_buffer.curr_len -= msg_len;
//             std::cout << "parse_msgs:" << (void*)client->rx_buffer.curr_ptr << " " << client->rx_buffer.curr_len << std::endl;
            if (!respMeta.disconnect) {
                continue;
            } else {
                // Somebody disconnected, get his details and print
                // packet had wrong format or connection was disconnected.
                if (log_level >= logINFO) {
                    std::cout << "[INFO] " << "Client " << client->id << " socket:"
                            << client->fd << " disconnected" << endl;

                    for (auto d : client->devices) {
                        std::cout << "    owned device:" << endl;
                        for (auto & s : d->get_spec())
                            std::cout << "        " << s.to_str() << endl;
                    }
                }
                remove_client(client->fd);
                removed_poll_fds.push_back(client->fd);
            }
        }
        // Message is incomplete break the cycle
        break;
    }
}

void HwioServer::handle_multiple_client_requests(int sd) {
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
// 	std::cout << "handle_client_requests:" << sd << " " << client->id
// 			<< std::endl;
	assert(client->fd == sd);
	if(read_from_socket(client)) {
		parse_msgs(client);
	} else {
		// Somebody disconnected, get his details and print
		// packet had wrong format or connection was disconnected.
		if (log_level >= logINFO) {
			std::cout << "[INFO] " << "Client " << client->id << " socket:"
					<< client->fd << " disconnected" << endl;

			for (auto d : client->devices) {
				std::cout << "    owned device:" << endl;
				for (auto & s : d->get_spec())
					std::cout << "        " << s.to_str() << endl;
			}
		}
		remove_client(sd);
		removed_poll_fds.push_back(sd);
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
