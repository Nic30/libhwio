#include "hwio_test.h"

using namespace hwio;

int main() {
	test_hwio_version_all();
	test_hwio_comp_spec_all();
	run_unit_tests_component_search();
	test_hwio_bus_xml_all();
	test_hwio_bus_devicetree_all();
	test_hwio_bus_remote_all();
	return 0;
}
