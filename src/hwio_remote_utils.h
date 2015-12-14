#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */

namespace hwio {

struct addrinfo * parse_ip_and_port(const std::string & host);
std::string addrinfo_to_str(const struct addrinfo * addr);

}
