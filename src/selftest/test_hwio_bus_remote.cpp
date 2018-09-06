#define BOOST_TEST_MODULE "Tests of hwio_bus_remote"
#include <boost/test/unit_test.hpp>

#include "hwio_bus_remote.h"
#include "hwio_server.h"
#include "hwio_bus_devicetree.h"
#include "hwio_remote_utils.h"
#include "hwio_device_remote.h"

#include <iostream>
#include <fstream>
#include <thread>
#include "../bus/hwio_bus_json.h"

using namespace std;
namespace hwio {

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
	size_t s = 2*0x1000;
	auto buff = new uint8_t[s];
	std::fill(buff, &buff[s], 0);
	mem.write((const char *)buff, s);
	delete [] buff;
}

void run_server() {
	spot_dev_mem_file();
	hwio_bus_devicetree bus_on_server("test_samples/device-tree0_32b",
			"test_samples/mem0.dat");
	hwio_bus_json bus_on_server_json("test_samples/device_descriptions/simple.json");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	HwioServer server(addr, { &bus_on_server, &bus_on_server_json });

	server.prepare_server_socket();
	server.handle_client_msgs(&run_server_flag);
	freeaddrinfo(addr);
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
	plugin_add_int_args args {a, b};
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
	hwio_bus_json bus_on_server_json("test_samples/device_descriptions/simple.json");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	HwioServer server(addr, { &bus_on_server, &bus_on_server_json });

	server.install_plugin_fn<plugin_add_int_args, uint32_t>("add_int", _add_int);

	server.prepare_server_socket();
	server.handle_client_msgs(&run_server_flag);
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
	for (int i = 0; i < 32 * 4; i++) {
		BOOST_CHECK_EQUAL(dev->read8(i * sizeof(uint8_t)), i + 1);
	}

	dev->read(0, buff, 32 * sizeof(uint32_t));
	BOOST_CHECK_EQUAL(memcmp(buff, buff_ref, 32 * sizeof(uint32_t)), 0);

}
BOOST_AUTO_TEST_CASE(test_remote_rw) {
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

BOOST_AUTO_TEST_CASE(test_remote_device_load) {
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
		BOOST_CHECK_EQUAL(bus->find_devices((dev_spec_t ) { simplebus }).size(),1);

		hwio_comp_spec serial_name;
		serial_name.name_set("serial@84000000");
		BOOST_CHECK_EQUAL(
				bus->find_devices((dev_spec_t ) { serial_name }).size(), 1);
	}
	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}

BOOST_AUTO_TEST_CASE(test_remote_ping) {
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

BOOST_AUTO_TEST_CASE(test_remote_call) {
	run_server_flag = true;
	thread server_thread(run_server_with_plugins0);
	server_start_delay();

	auto bus = make_unique<hwio_bus_remote>(server_addr);

	hwio_comp_spec dev0("dev0,v-1.0.a");
	auto devices = bus->find_devices((dev_spec_t ) { dev0 });

	BOOST_CHECK_EQUAL(devices.size(), 1);
	BOOST_CHECK_EQUAL(add_int(devices.at(0), 1, 2), 3);
	BOOST_CHECK_EQUAL(add_int(devices.at(0), 1<<16, 2), ((1<<16) + 2));


	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
}

}
