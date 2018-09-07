#define BOOST_TEST_MODULE "Tests of hwio_bus_devicetree"
#include <boost/test/unit_test.hpp>

#include "hwio_bus_devicetree.h"
#include <iostream>

namespace hwio {

typedef std::vector<hwio_comp_spec> dev_spec_t;

BOOST_AUTO_TEST_CASE(Test_devicetree_device_load) {
	hwio_bus_devicetree bus("test_samples/device-tree0_32b");

	BOOST_CHECK_EQUAL(bus._all_devices.size(), 8);

	hwio_comp_spec serial0("xlnx,xps-uartlite-1.1.97");
	auto s0 = bus.find_devices((dev_spec_t ) { serial0 });
	BOOST_CHECK_EQUAL(s0.size(), 2);
	auto s0_base = dynamic_cast<hwio_device_mmap*>(s0.at(0))->on_bus_base_addr;
	auto s1_base = dynamic_cast<hwio_device_mmap*>(s0.at(1))->on_bus_base_addr;
	BOOST_CHECK_EQUAL(s0_base, 0x84000000);
	BOOST_CHECK_EQUAL(s1_base, 0x88000000);

	hwio_comp_spec serial1("xlnx,xps-uartlite-1.0.97");
	BOOST_CHECK_EQUAL(bus.find_devices((dev_spec_t ) { serial1 }).size(), 2);

	hwio_comp_spec simplebus("simple-bus");
	BOOST_CHECK_EQUAL(bus.find_devices((dev_spec_t ) { simplebus }).size(), 1);

	hwio_comp_spec serial_name;
	serial_name.name_set("serial@84000000");
	BOOST_CHECK_EQUAL(bus.find_devices((dev_spec_t ) { serial_name }).size(), 1);

}

BOOST_AUTO_TEST_CASE(test_devicetree_device_load2) {
	hwio_bus_devicetree bus("test_samples/device-tree1_32b/device-tree/");
	hwio_comp_spec dp("uprobe,dispather-1.0.a");
	BOOST_CHECK_EQUAL(bus.find_devices((dev_spec_t ) { dp }).size(), 2);
}

}
