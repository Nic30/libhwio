#define BOOST_TEST_MODULE "Tests of hwio_device_mmap"
#include <boost/test/unit_test.hpp>

#include "hwio_bus_primitive.h"
#include "hwio_device_mmap.h"

namespace hwio {

BOOST_AUTO_TEST_CASE(test_byname) {
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

	BOOST_CHECK_EQUAL(bus0.find_devices(compat).size(), 0);
	for (auto & d : devs) {
		bus0._all_devices.push_back(d);
	}
	auto found = bus0.find_devices(compat);
	BOOST_CHECK_EQUAL(found.size(), 2);
	for (auto & d : devs) {
		delete d;
	}
}


}
