
#define BOOST_TEST_MODULE "Tests of hwio_bus_devicetree"
#include <boost/test/unit_test.hpp>
#include <string.h>

#include "../tests/test_utils.h"
#include "hwio_comp_spec.h"


namespace hwio {

BOOST_AUTO_TEST_CASE(test_comp_spec_match) {
	hwio_comp_spec s1;
	hwio_comp_spec s2;

	s1 = {"test-vendor", "test-type-1", {1, 2, 0}};
	s2 = {"test-vendor", "test-type-2", {1, 2, 0}};
	BOOST_CHECK_NE(s1, s2);

	s2 = {"test-vendor", "test-type-1", {1, 2, 0}};
	BOOST_CHECK_EQUAL(s1, s2);

	s2 = {"test-vendor", "", {1, 2, 0}};
	BOOST_CHECK_EQUAL(s1, s2);

	s2 = {"", "test-type-1", {1, 2, 0}};
	BOOST_CHECK_EQUAL(s1, s2);

	s2 = {"", "", {1, 2, 0}};
	BOOST_CHECK_EQUAL(s1, s2);

	s2 = {"test-vendor", "test-type-1", {1, 2, HWIO_VERSION_NA}};
	BOOST_CHECK_EQUAL(s1, s2);
}

BOOST_AUTO_TEST_CASE(test_comp_name_and_spec_match) {
	hwio_comp_spec s1;
	hwio_comp_spec s2;

	s1 = {"test-vendor", "test-type", {1, 2, 0}};
	s2 = {"test-vendor", "test-type", {1, 2, 0}};
	s1.name_set("comp-name-1");
	s2.name_set("comp-name-2");
	BOOST_CHECK_NE(s1, s2);

	s2.name_set("comp-name-1");
	BOOST_CHECK_EQUAL(s1, s2);

	s2.name_set("");
	BOOST_CHECK_EQUAL(s1, s2);

	s2 = {"test-vendor", "test-type", {1, 2, 3}};
	BOOST_CHECK_NE(s1, s2);
}

BOOST_AUTO_TEST_CASE(test_treat_it_as_type) {
	hwio_comp_spec spec("testing-type");

	BOOST_CHECK_EQUAL(spec.vendor, "");
	BOOST_CHECK_EQUAL(spec.type, "testing-type");
}

BOOST_AUTO_TEST_CASE(test_use_vendor_and_type) {
	hwio_comp_spec spec("test-vendor,testing-type");

	BOOST_CHECK_EQUAL(spec.vendor, "test-vendor");
	BOOST_CHECK_EQUAL(spec.type, "testing-type");
}

BOOST_AUTO_TEST_CASE(test_use_type_and_version) {
	hwio_comp_spec spec("testing-type-1.0.a");

	BOOST_CHECK_EQUAL(spec.vendor, "");
	BOOST_CHECK_EQUAL(spec.type, "testing-type");
	test_version(spec.version, 1, 0, 'a');
}

BOOST_AUTO_TEST_CASE(test_use_vendor_type_version) {
	hwio_comp_spec spec("test-vendor,testing-type-1.0.a");

	BOOST_CHECK_EQUAL(spec.vendor, "test-vendor");
	BOOST_CHECK_EQUAL(spec.type, "testing-type");
	test_version(spec.version, 1, 0, 'a');
}

}
