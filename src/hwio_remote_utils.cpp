#include "hwio_remote_utils.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace hwio {

void parse_ip_and_port(const std::string & _addrString, std::string & ip,
		std::string & port) {
	std::string addrString = _addrString;
	if (addrString[0] == '[') {
		// IPv6 in format [ipv6]:port
		std::string::iterator splitEnd = std::find(addrString.begin() + 1,
				addrString.end(), ']');
		ip.assign(addrString.begin(), splitEnd);
		if (splitEnd != addrString.end())
			splitEnd++;
		if (splitEnd != addrString.end() && *splitEnd == ':')
			port.assign(splitEnd, addrString.end());
	} else {
		// IPv4 in format ipv4:port
		std::string::iterator splitPoint = std::find(addrString.rbegin(),
				addrString.rend(), ':').base();
		if (splitPoint == addrString.begin())
			ip = addrString;
		else {
			ip.assign(addrString.begin(), splitPoint - 1);
			port.assign(splitPoint, addrString.end());
		}
	}
}

struct addrinfo * parse_ip_and_port(const std::string & host) {
	std::string ip, port;
	parse_ip_and_port(host, ip, port);

	int error;
	struct addrinfo hints;
	struct addrinfo *res = nullptr;

	memset(&hints, 0, sizeof(hints));

	/* set-up hints structure */
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = AI_PASSIVE;
	hints.ai_flags = AI_ALL;
	//hints.ai_flags = AI_CANONNAME;

	const char * _ip = ip.c_str();
	const char * _port = port.c_str();
	error = getaddrinfo(_ip, _port, &hints, &res);

	if (error || res == nullptr)
		throw std::runtime_error(std::string("[HWIO] ") + gai_strerror(error));
	else {
		return res;
	}
}

std::string addrinfo_to_str(const struct addrinfo * addr) {
	if (addr == nullptr)
		return "<NULL>";

	std::stringstream ss;
	char ipstr[INET_ADDRSTRLEN];
	memset(ipstr, 0, sizeof ipstr);

	//inet_ntop(res->ai_family, res, ipstr, sizeof ipstr);
	if (nullptr
			== inet_ntop( AF_INET,
					&((struct sockaddr_in *) addr->ai_addr)->sin_addr, ipstr,
					sizeof(ipstr))) {
		perror("inet_ntop");
	}
	ss << ipstr << ":" << __builtin_bswap16(((struct sockaddr_in *) addr->ai_addr)->sin_port);
	return ss.str();
}

}
