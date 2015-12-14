#include "hwio_test.h"
#include "hwio_bus_xml.h"

namespace hwio {

void test_xml_device_load() {
	test_start();

	hwio_bus_xml bus("test_samples/hwio_test_uprobe1g.xml");
	test_assert(bus._all_devices.size() == 8, "All device loaded");

	test_end();
}

void test_hwio_bus_xml_all() {
	test_xml_device_load();
}

}
