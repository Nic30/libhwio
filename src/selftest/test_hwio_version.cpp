#include "hwio_test.h"

namespace hwio {

void test_version_match(void) {
	test_start();

	struct hwio_version v1;
	struct hwio_version v2;
	v1 = {4, 5, 6};
	v2 = {4, 5, 0};
	test_assert(v1 != v2, "Version matches");

	v2 = {4, 5, 6};
	test_assert(v1 == v2, "Version does not match");

	v2 = {4, 5};
	test_assert(v1 == v2, "Version does not match");

	v2 = {4};
	test_assert(v1 == v2, "Version does not match");

	v2 = {};
	test_assert(v1 == v2, "Version does not match");

	v2 = {HWIO_VERSION_NA, 5, HWIO_VERSION_NA};
	test_assert(v1 == v2, "Version does not match");

	v2 = {HWIO_VERSION_NA, 3, HWIO_VERSION_NA};
	test_assert(v1 != v2, "Version does not match");

	test_end();
}

void test_parse_version() {
	test_start();
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

	test_end();
}

void test_hwio_version_all() {
	test_version_match();
	test_parse_version();
}

}
