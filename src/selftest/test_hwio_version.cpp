#define BOOST_TEST_MODULE "Tests of hwio_version"
#include <boost/test/unit_test.hpp>

#include "test_utils.h"
#include "../hwio_version.h"

namespace hwio {

BOOST_AUTO_TEST_CASE(test_version_match) {
	struct hwio_version v1;
	struct hwio_version v2;
	v1 = {4, 5, 6};
	v2 = {4, 5, 0};
	BOOST_CHECK_NE(v1, v2);

	v2 = {4, 5, 6};
	BOOST_CHECK_EQUAL(v1, v2);

	v2 = {4, 5};
	BOOST_CHECK_EQUAL(v1, v2);

	v2 = {4};
	BOOST_CHECK_EQUAL(v1, v2);

	v2 = {};
	BOOST_CHECK_EQUAL(v1, v2);

	v2 = {HWIO_VERSION_NA, 5, HWIO_VERSION_NA};
	BOOST_CHECK_EQUAL(v1, v2);

	v2 = {HWIO_VERSION_NA, 3, HWIO_VERSION_NA};
	BOOST_CHECK_NE(v1, v2);
}

BOOST_AUTO_TEST_CASE(test_parse_version) {
	hwio_version ver;

	ver = {"1.00.a"};
	test_version(ver, 1, 0, 'a');

	ver = {"2.00.c"};
	test_version(ver, 2, 0, 'c');

	ver = {"3.00.0"};
	test_version(ver, 3, 0, 0);

	ver = {"4.00"};
	test_version(ver, 4, 0, HWIO_VERSION_NA);

	ver = {"5.0"};
	test_version(ver, 5, 0, HWIO_VERSION_NA);

	ver = {"6.0.1"};
	test_version(ver, 6, 0, 1);

	ver = {"7.1.1"};
	test_version(ver, 7, 1, 1);

	ver = {"8.2.a"};
	test_version(ver, 8, 2, 'a');

	ver = {"12.12.c"};
	test_version(ver, 12, 12, 'c');
}

}
