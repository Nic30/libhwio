#define BOOST_TEST_MODULE "Tests of hwio_bus_remote"
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <thread>

#include "hwio_bus_remote.h"
#include "hwio_server.h"
#include "hwio_bus_devicetree.h"
#include "hwio_remote_utils.h"
#include "hwio_device_remote.h"
#include "bus/hwio_bus_json.h"
namespace utf = boost::unit_test;

using namespace std;
namespace hwio {

struct server_thread_args_t {
	HwioServer * server;
	bool * run_flag;
};

const typedef vector<hwio_comp_spec> dev_spec_t;

const char * server_addr = "127.0.0.1:8896";
bool run_server_flag = true;

inline void server_start_delay() {
	usleep(200000);
}

inline void server_stop_delay() {
	usleep(200000);
}

void spot_dev_mem_file() {
	ofstream mem("test_samples/mem0.dat", std::ios::binary);
	size_t s = 2 * 0x1000;
	auto buff = new uint8_t[s];
	std::fill(buff, &buff[s], 0);
	mem.write((const char *) buff, s);
	delete[] buff;
}

void run_server() {
	spot_dev_mem_file();
	hwio_bus_devicetree bus_on_server("test_samples/device-tree0_32b",
			"test_samples/mem0.dat");
	hwio_bus_json bus_on_server_json(
			"test_samples/device_descriptions/simple.json");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	HwioServer server(addr, { &bus_on_server, &bus_on_server_json });

	server.prepare_server_socket();
	while(run_server_flag) {
		server.pool_client_msgs();
    }
	freeaddrinfo(addr);
}
/**
 * Open tcp connection on specified server
 * */
int tcp_open(const std::string& server_addr) {
	auto addr = parse_ip_and_port(server_addr);
	int sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (sockfd < 0) {
		throw std::runtime_error("[HWIO] Can not create a socket");
	}
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv,
			sizeof(struct timeval));
	int ret = connect(sockfd, addr->ai_addr, addr->ai_addrlen);
	if (ret < 0) {
		throw std::runtime_error(
				std::string("[HWIO, test] Can not connect to server: (error: ")
						+ strerror(ret) + ", address: " + server_addr + " )");
	}
	freeaddrinfo(addr);
	return sockfd;
}

struct plugin_add_int_args {
	uint32_t a;
	uint32_t b;
};

// function with real functionality
void _add_int(ihwio_dev * dev, plugin_add_int_args * args, uint32_t * ret) {
	*ret = args->a + args->b;
}

// wrapper for local call or call of plugin on server
uint32_t add_int(ihwio_dev * dev, uint32_t a, uint32_t b) {
	hwio_device_remote * d = dynamic_cast<hwio_device_remote *>(dev);
	plugin_add_int_args args { a, b };
	if (d == nullptr) {
		// device is local, there is no server or we are the server
		uint32_t ret;
		_add_int(dev, &args, &ret);
		return ret;
	} else {
		// we are on client and we can call this function on server
		return d->remote_call<plugin_add_int_args, uint32_t>(__func__, &args);
	}
}

void run_server_with_plugins0() {
	spot_dev_mem_file();
	hwio_bus_devicetree bus_on_server("test_samples/device-tree0_32b",
			"test_samples/mem0.dat");
	hwio_bus_json bus_on_server_json(
			"test_samples/device_descriptions/simple.json");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	HwioServer server(addr, { &bus_on_server, &bus_on_server_json });

	server.install_plugin_fn<plugin_add_int_args, uint32_t>("add_int",
			_add_int);

	server.prepare_server_socket();
	while(run_server_flag) {
		server.pool_client_msgs();
	}
	freeaddrinfo(addr);
}

void _test_device_rw(ihwio_dev * dev) {
	dev->memset(0, 0, 32 * sizeof(uint32_t));
	for (int i = 0; i < 32 * 4; i++) {
		BOOST_CHECK_EQUAL(dev->read8(i * sizeof(uint8_t)), 0);
	}

	//for (int i = 0; i < 32*2; i++) {
	//	BOOST_CHECK_EQUAL(s0->read16(i * sizeof(uint16_t)) == 0, "read16 0");
	//}

	for (int i = 0; i < 32; i++) {
		BOOST_CHECK_EQUAL(dev->read32(i * sizeof(uint32_t)), 0);
	}
	for (int i = 0; i < 32 / 2; i++) {
		BOOST_CHECK_EQUAL(dev->read64(i * sizeof(uint64_t)), 0);
	}

	unsigned char buff[32 * sizeof(uint32_t)];
	unsigned char buff_ref[32 * sizeof(uint32_t)];
	memset(buff_ref, 0, 32 * sizeof(uint32_t));

	dev->read(0, buff, 32 * sizeof(uint32_t));
	BOOST_CHECK_EQUAL(memcmp(buff, buff_ref, 32 * sizeof(uint32_t)), 0);

	for (unsigned i = 0; i < 32 * sizeof(uint32_t); i++)
		buff_ref[i] = i + 1;

	dev->write(0, buff_ref, 32 * sizeof(uint32_t));
	for (size_t i = 0; i < 32 * sizeof(uint32_t); i++) {
		BOOST_CHECK_EQUAL(dev->read8(i * sizeof(uint8_t)), buff_ref[i]);
	}

	for (int i = 0; i < 32; i++) {
		uint32_t tmp = reinterpret_cast<uint32_t*>(buff_ref)[i];
		BOOST_CHECK_EQUAL(dev->read32(i * sizeof(uint32_t)), tmp);
	}

	for (int i = 0; i < 32/2; i++) {
		uint64_t tmp = reinterpret_cast<uint64_t*>(buff_ref)[i];
		BOOST_CHECK_EQUAL(dev->read64(i * sizeof(uint64_t)), tmp);
	}

	dev->read(0, buff, 32 * sizeof(uint32_t));
	BOOST_CHECK_EQUAL(memcmp(buff, buff_ref, 32 * sizeof(uint32_t)), 0);

}

BOOST_AUTO_TEST_SUITE(hwio_bus_remoteTC)

BOOST_AUTO_TEST_CASE(test_remote_rw, * utf::timeout(15)) {
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	auto bus = make_unique<hwio_bus_remote>(server_addr);

	hwio_comp_spec dev0("dev0,v-1.0.a");
	auto devices = bus->find_devices((dev_spec_t ) { dev0 });

	BOOST_CHECK_EQUAL(devices.size(), 1);

	auto s0 = devices[0];
	s0->attach();
	_test_device_rw(s0);

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}

BOOST_AUTO_TEST_CASE(test_remote_device_load, * utf::timeout(15)) {
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	for (int i = 0; i < 10; i++) {
		auto bus = make_unique<hwio_bus_remote>(server_addr);

		hwio_comp_spec serial0("xlnx,xps-uartlite-1.1.97");
		auto serialDevs = bus->find_devices((dev_spec_t ) { serial0 });
		BOOST_CHECK_EQUAL(serialDevs.size(), 2);
		hwio_comp_spec serial1("xlnx,xps-uartlite-1.0.97");
		dev_spec_t spec = { serial1 };
		BOOST_CHECK_EQUAL(bus->find_devices(spec).size(), 2);

		hwio_comp_spec simplebus("simple-bus");
		BOOST_CHECK_EQUAL(bus->find_devices((dev_spec_t ) { simplebus }).size(),
				1);

		hwio_comp_spec serial_name;
		serial_name.name_set("serial@84000000");
		BOOST_CHECK_EQUAL(
				bus->find_devices((dev_spec_t ) { serial_name }).size(), 1);
	}
	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}

BOOST_AUTO_TEST_CASE(test_remote_ping, * utf::timeout(15)) {
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	for (int i0 = 0; i0 < MAX_CLIENTS; i0++) {
		auto con = new hwio_client_to_server_con(server_addr);
		con->connect_to_server();
		for (int i = 0; i < 10; i++) {
			cout.flush();
			cerr.flush();
			//cout << i << "--------------------" << endl;
			BOOST_CHECK_EQUAL(con->ping(), 0);
		}
		delete con;
	}

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}

BOOST_AUTO_TEST_CASE(test_remote_call, * utf::timeout(15)) {
	run_server_flag = true;
	thread server_thread(run_server_with_plugins0);
	server_start_delay();

	auto bus = make_unique<hwio_bus_remote>(server_addr);

	hwio_comp_spec dev0("dev0,v-1.0.a");
	auto devices = bus->find_devices((dev_spec_t ) { dev0 });

	BOOST_CHECK_EQUAL(devices.size(), 1);
	auto d = devices.at(0);
	d->attach();

	BOOST_CHECK_EQUAL(add_int(d, 1, 2), 3);
	BOOST_CHECK_EQUAL(add_int(d, 1 << 16, 2), ((1 << 16) + 2));

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}


BOOST_AUTO_TEST_CASE(test_remote_server_stability, * utf::timeout(15)) {
	run_server_flag = true;
	thread server_thread(run_server_with_plugins0);
	server_start_delay();
	try {
		std::vector<hwio_bus_remote*> buses;
		std::vector<int> opened_socketes;
		for (int try_i = 0; try_i < 30; try_i++) {
			for (int i = 0; i < 5; i++) {
				auto bus = new hwio_bus_remote(server_addr);
				buses.push_back(bus);

				for (int i2 = 0; i2 < 5; i2++) {
					int s = tcp_open(server_addr);
					opened_socketes.push_back(s);
				}

				hwio_comp_spec dev0("dev0,v-1.0.a");
				std::vector<ihwio_dev*> devices = bus->find_devices((dev_spec_t) { dev0 });

				BOOST_CHECK_EQUAL(devices.size(), 1);

				auto d = devices.at(0);
				d->attach();

				BOOST_CHECK_EQUAL(add_int(d, 1, 2), 3);
				BOOST_CHECK_EQUAL(add_int(d, 1 << 16, 2), ((1 << 16) + 2));

			}

			for (auto bus : buses) {
				delete bus;
			}
			buses.clear();

			for (int s : opened_socketes) {
				close(s);
			}
			opened_socketes.clear();
		}
	} catch (const std::runtime_error& e) {
		std::cerr << e.what();
		run_server_flag = false;
		server_thread.join();
		server_stop_delay();
		throw e;
	}
	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}


void serve_clients(server_thread_args_t * args) {
	while(*args->run_flag) {
		args->server->pool_client_msgs();
	}
}

BOOST_AUTO_TEST_CASE(clients_are_disconnecting_correctly, * utf::timeout(5)) {
	spot_dev_mem_file();
	run_server_flag = true;

	hwio_bus_json bus_on_server_json(
			"test_samples/device_descriptions/simple.json");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	HwioServer server(addr, { &bus_on_server_json });
	server.prepare_server_socket();
	server_thread_args_t args =  {&server, &run_server_flag};
	thread server_thread(serve_clients, &args);
	server_start_delay();
	for (int i = 0; i < 10; i++) {
		std::vector<ihwio_bus*> buses;
		for (int i2 = 0; i2 < (i + 1); i2++) {
			auto bus = new hwio_bus_remote(server_addr);
			buses.push_back(bus);
		}
		usleep(100000);
		BOOST_CHECK_EQUAL(server.get_client_cnt(), buses.size());
		for (auto bus: buses)
			delete bus;
		usleep(100000);
		BOOST_CHECK_EQUAL(server.get_client_cnt(), 0);
	}

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
	freeaddrinfo(addr);
}


BOOST_AUTO_TEST_SUITE_END()

}
