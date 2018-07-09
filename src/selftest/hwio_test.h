#pragma once

#include <stdio.h>
#include "hwio_version.h"
#include "hwio_comp_spec.h"

namespace hwio {

#define test_start() fprintf(stderr, "  Started %s\n", __func__)
#define test_end()   fprintf(stderr, "  Finished %s\n", __func__)
#define subtest_start(name) fprintf(stderr, "  Started %s\n", (name))
#define subtest_end(name)  fprintf(stderr, "  Finished %s\n", (name))
#define test_fail(msg, r) fprintf(stderr, "! Failure at %s (%d): %s\n", __func__, (r), (msg))
#define test_assert(x, msg) if(!(x)) {fprintf(stderr, "! Assertion failed at %s (%s.%d): %s\n", __func__, __FILE__, __LINE__, (msg));}

static inline
void _test_version(struct hwio_version ver, int maj, int min, int sub, const char *func, const char *file, int line) {
	if(ver.major != maj)
		fprintf(stderr, "! Major doesn't match at %s (%s.%d): %d != %d\n", __func__, file, line, ver.major, maj);
	if(ver.minor != min)
		fprintf(stderr, "! Minor doesn't match at %s (%s.%d): %d != %d\n", __func__, file, line, ver.minor, min);
	if(ver.subminor != sub)
		fprintf(stderr, "! Subminor doesn't match at %s (%s.%d): %d != %d\n", __func__, file, line, ver.subminor, sub);
}
#define test_version(ver, maj, min, sub) _test_version((ver), (maj), (min), (sub), __func__, __FILE__, __LINE__)

void spot_dev_mem_file();

void test_hwio_comp_spec_all();
void test_hwio_version_all();
void run_unit_tests_component_search();
void test_hwio_bus_xml_all();
void test_hwio_bus_devicetree_all();
void test_hwio_bus_remote_all();

}
