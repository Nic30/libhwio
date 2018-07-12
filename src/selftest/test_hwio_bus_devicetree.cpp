#include "hwio_test.h"
#include "hwio_bus_devicetree.h"
#include <iostream>

namespace hwio {

typedef std::vector<hwio_comp_spec> dev_spec_t;

void test_devicetree_device_load() {
	test_start();
	hwio_bus_devicetree bus("test_samples/device-tree0_32b");

	test_assert(bus._all_devices.size() == 8, "All devices loaded");

	hwio_comp_spec serial0("xlnx,xps-uartlite-1.1.97");
	auto s0 = bus.find_devices((dev_spec_t ) { serial0 });
	test_assert(s0.size() == 2, "find by xlnx,xps-uartlite-1.1.97");
	auto s0_base = dynamic_cast<hwio_device_mmap*>(s0.at(0))->on_bus_base_addr;
	auto s1_base = dynamic_cast<hwio_device_mmap*>(s0.at(1))->on_bus_base_addr;
	test_assert(s1_base == 0x84000000, "correct address of serial 0x84000000");
	test_assert(s0_base == 0x88000000, "correct address of serial 0x88000000");

	hwio_comp_spec serial1("xlnx,xps-uartlite-1.0.97");
	test_assert(bus.find_devices((dev_spec_t ) { serial1 }).size() == 2, "find by xlnx,xps-uartlite-1.0.97");

	hwio_comp_spec simplebus("simple-bus");
	test_assert(bus.find_devices((dev_spec_t ) { simplebus }).size() == 1, "find by simple-bus");

	hwio_comp_spec serial_name;
	serial_name.name_set("serial@84000000");
	test_assert(bus.find_devices((dev_spec_t ) { serial_name }).size() == 1, "find by name serial@84000000");

	test_end();
}

void test_devicetree_device_load2() {
	test_start();
	hwio_bus_devicetree bus("test_samples/device-tree1_32b/device-tree/");
	hwio_comp_spec dp("uprobe,dispather-1.0.a");
	test_assert(bus.find_devices((dev_spec_t ) { dp }).size() == 2, "find by uprobe,dispather-1.0.a");
	test_end();
}

void test_hwio_bus_devicetree_all() {
	test_devicetree_device_load();
	test_devicetree_device_load2();
}

}
