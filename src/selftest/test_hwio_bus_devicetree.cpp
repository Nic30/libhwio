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
	test_assert(bus.find_devices((dev_spec_t ) { serial0 }).size() == 2, "find by xlnx,xps-uartlite-1.1.97");

	hwio_comp_spec serial1("xlnx,xps-uartlite-1.0.97");
	test_assert(bus.find_devices((dev_spec_t ) { serial1 }).size() == 2, "find by xlnx,xps-uartlite-1.0.97");

	hwio_comp_spec simplebus("simple-bus");
	test_assert(bus.find_devices((dev_spec_t ) { simplebus }).size() == 1, "find by simple-bus");

	hwio_comp_spec serial_name;
	serial_name.name_set("serial@84000000");
	test_assert(bus.find_devices((dev_spec_t ) { serial_name }).size() == 1, "find by name serial@84000000");

	test_end();
}

void test_hwio_bus_devicetree_all() {
	test_devicetree_device_load();
}

}
