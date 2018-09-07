#define BOOST_TEST_MODULE "Tests of hwio_bus_json"
#include <boost/test/unit_test.hpp>

#include "bus/hwio_bus_json.h"

namespace hwio {


BOOST_AUTO_TEST_CASE(test_json_device_load) {
	hwio_bus_json bus("test_samples/device_descriptions/multiple.json");
	BOOST_CHECK_EQUAL(bus._all_devices.size(), 8);
}

}
