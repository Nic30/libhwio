#include "hwio_test.h"
#include "hwio_bus_remote.h"
#include "hwio_server.h"
#include "hwio_bus_devicetree.h"
#include "hwio_bus_xml.h"
#include "hwio_remote_utils.h"

#include <iostream>
#include <thread>

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

void run_server() {
	hwio_bus_devicetree bus_on_server("test_samples/device-tree0_32b",
			"test_samples/mem0.dat");
	hwio_bus_xml bus_on_server_xml("test_samples/hwio_test_simple.xml");

	string server_addr_str(server_addr);
	struct addrinfo * addr = parse_ip_and_port(server_addr_str);
	Hwio_server server(addr, { &bus_on_server, &bus_on_server_xml });

	server.prepare_server_socket();
	server.handle_client_msgs(&run_server_flag);
}

void _test_device_rw(ihwio_dev * dev) {
	dev->memset(0, 0, 32 * sizeof(uint32_t));
	for (int i = 0; i < 32 * 4; i++) {
		test_assert(dev->read8(i * sizeof(uint8_t)) == 0, "read8 0");
	}

	//for (int i = 0; i < 32*2; i++) {
	//	test_assert(s0->read16(i * sizeof(uint16_t)) == 0, "read16 0");
	//}

	for (int i = 0; i < 32; i++) {
		test_assert(dev->read32(i * sizeof(uint32_t)) == 0, "read32 0");
	}
	for (int i = 0; i < 32 / 2; i++) {
		test_assert(dev->read64(i * sizeof(uint64_t)) == 0, "read64 0");
	}

	unsigned char buff[32 * sizeof(uint32_t)];
	unsigned char buff_ref[32 * sizeof(uint32_t)];
	memset(buff_ref, 0, 32 * sizeof(uint32_t));

	dev->read(0, buff, 32 * sizeof(uint32_t));
	test_assert(memcmp(buff, buff_ref, 32 * sizeof(uint32_t)) == 0,
			"read all 0");

	for (unsigned i = 0; i < 32 * sizeof(uint32_t); i++)
		buff[i] = i + 1;

	dev->write(0, buff, 32 * sizeof(uint32_t));
	for (int i = 0; i < 32 * 4; i++) {
		//cout << "dev->read8(i * sizeof(uint8_t)) =" << (int) dev->read8(i * sizeof(uint8_t)) << endl;
		test_assert(dev->read8(i * sizeof(uint8_t)) == i + 1, "read8 sequence");
	}

	dev->read(0, buff, 32 * sizeof(uint32_t));
	test_assert(memcmp(buff, buff_ref, 32 * sizeof(uint32_t)) == 0,
			"read all sequence");

}

void test_remote_rw() {
	test_start();
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	auto * bus = new hwio_bus_remote(server_addr);

	hwio_comp_spec dev0("dev0,v-1.0.a");
	auto devices = bus->find_devices((dev_spec_t ) { dev0 });

	test_assert(devices.size() == 1, "dev0,v-1.0.a");

	auto s0 = devices[0];
	s0->attach();
	_test_device_rw(s0);

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
	test_end();
}

void test_remote_device_load() {
	test_start();
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	for (int i = 0; i < 10; i++) {
		auto * bus = new hwio_bus_remote(server_addr);

		hwio_comp_spec serial0("xlnx,xps-uartlite-1.1.97");
		test_assert(bus->find_devices((dev_spec_t ) { serial0 }).size() == 2,
				"find by xlnx,xps-uartlite-1.1.97");
		hwio_comp_spec serial1("xlnx,xps-uartlite-1.0.97");
		dev_spec_t spec = { serial1 };
		test_assert(bus->find_devices(spec).size() == 2,
				"find by xlnx,xps-uartlite-1.0.97");

		hwio_comp_spec simplebus("simple-bus");
		test_assert(bus->find_devices((dev_spec_t ) { simplebus }).size() == 1,
				"find by simple-bus");

		hwio_comp_spec serial_name;
		serial_name.name_set("serial@84000000");
		test_assert(
				bus->find_devices((dev_spec_t ) { serial_name }).size() == 1,
				"find by name serial@84000000");

		delete bus;
	}
	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
	test_end();
}

void test_remote_ping() {
	test_start();
	run_server_flag = true;
	thread server_thread(run_server);
	server_start_delay();

	for (int i0 = 0; i0 < 10; i0++) {
		auto * con = new hwio_client_to_server_con(server_addr);
		con->connect_to_server();
		for (int i = 0; i < 10; i++) {
			cout.flush();
			cerr.flush();
			//cout << i << "--------------------" << endl;
			test_assert(con->ping() == 0, "ping works");
		}
		delete con;
	}

	run_server_flag = false;
	server_thread.join();
	server_stop_delay();
	test_end();
}

void test_hwio_bus_remote_all() {
	test_remote_ping();
	test_remote_device_load();
	test_remote_rw();
}

}
