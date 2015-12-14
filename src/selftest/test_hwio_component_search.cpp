#include "hwio_test.h"
#include "hwio_bus_primitive.h"
#include "hwio_device_mmap.h"

namespace hwio {

void test_byname() {
	test_start();
	std::vector<hwio_device_mmap*> devs = { //
			new hwio_device_mmap({"dsjfskdjf", "sdkfjsdlkfj", {9, 9}}, 0, 4),       //
			new hwio_device_mmap({"test-vendor-2", "test-comp-2", {1, 0, 5}}, 0, 4),//
		    new hwio_device_mmap({"332904832", "34293483940", {1, 2, 3}}, 0, 4),    //
            new hwio_device_mmap({"test-vendor-1", "test-comp-1", {2, 3}}, 0, 4),   //
	        new hwio_device_mmap({"j4jk3j5h5", "3j6n4b0d1e4", {4, 5, 6}}, 0, 4),    //
	};
	std::vector<hwio_comp_spec> compat = { //
			{ "test-vendor-1", "test-comp-1", {2, 3} }, //
			{ "test-vendor-2", "test-comp-2", {1, 0} }, //
			};

	hwio_bus_primitive bus0;

	test_assert(bus0.find_devices(compat).size() == 0,
			"Can not find when there are not devices");
	for (auto & d : devs) {
		bus0._all_devices.push_back(d);
	}
	auto found = bus0.find_devices(compat);
	test_assert(found.size() == 2,
			"Found when devices present");
	for (auto & d : devs) {
		delete d;
	}
	test_end();
}

void run_unit_tests_component_search() {
	test_byname();
}

}
