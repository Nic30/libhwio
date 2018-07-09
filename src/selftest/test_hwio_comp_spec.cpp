#include "hwio_test.h"


#include <string.h>


namespace hwio {


void test_comp_spec_match(void) {
	test_start();

	hwio_comp_spec s1;
	hwio_comp_spec s2;

	s1 = {"test-vendor", "test-type-1", {1, 2, 0}};
	s2 = {"test-vendor", "test-type-2", {1, 2, 0}};
	test_assert(s1 != s2, "Specification matches");

	s2 = {"test-vendor", "test-type-1", {1, 2, 0}};
	test_assert(s1 == s2, "Specification does not match");

	s2 = {"test-vendor", "", {1, 2, 0}};
	test_assert(s1 == s2, "Specification does not match");

	s2 = {"", "test-type-1", {1, 2, 0}};
	test_assert(s1 == s2, "Specification does not match");

	s2 = {"", "", {1, 2, 0}};
	test_assert(s1 == s2, "Specification does not match");

	s2 = {"test-vendor", "test-type-1", {1, 2, HWIO_VERSION_NA}};
	test_assert(s1 == s2, "Specification does not match");

	test_end();
}

void test_comp_name_and_spec_match() {
	test_start();

	hwio_comp_spec s1;
	hwio_comp_spec s2;

	s1 = {"test-vendor", "test-type", {1, 2, 0}};
	s2 = {"test-vendor", "test-type", {1, 2, 0}};
	s1.name_set("comp-name-1");
	s2.name_set("comp-name-2");
	test_assert(s1 != s2, "Specification and name match");

	s2.name_set("comp-name-1");
	test_assert(s1 == s2, "Specification and name do not match");

	s2.name_set("");
	test_assert(s1 == s2, "Specification and name do not match");

	s2 = {"test-vendor", "test-type", {1, 2, 3}};
	test_assert(s1 != s2, "Specification and name match");

	test_end();
}

void test_treat_it_as_type() {
	test_start();
	hwio_comp_spec spec("testing-type");

	test_assert(spec.vendor == "", "Vendor doesn't match");
	test_assert(spec.type == "testing-type", "Type doesn't match");

	test_end();
}

void test_use_vendor_and_type() {
	test_start();
	hwio_comp_spec spec("test-vendor,testing-type");

	test_assert(spec.vendor == "test-vendor", "Vendor doesn't match");
	test_assert(spec.type == "testing-type", "Type doesn't match");

	test_end();
}

void test_use_type_and_version() {
	test_start();
	hwio_comp_spec spec("testing-type-1.0.a");

	test_assert(spec.vendor == "", "Vendor doesn't match");
	test_assert(spec.type == "testing-type", "Type doesn't match");
	test_version(spec.version, 1, 0, 'a');

	test_end();
}

void test_use_vendor_type_version() {
	test_start();
	hwio_comp_spec spec("test-vendor,testing-type-1.0.a");

	test_assert(spec.vendor == "test-vendor", "Vendor doesn't match");
	test_assert(spec.type == "testing-type", "Type doesn't match");
	test_version(spec.version, 1, 0, 'a');

	test_end();
}

void test_hwio_comp_spec_all() {
	test_comp_spec_match();
	test_comp_name_and_spec_match();
	test_treat_it_as_type();
	test_use_vendor_and_type();
	test_use_type_and_version();
	test_use_vendor_type_version();
}

}
