#pragma once

#define test_version(ver, maj, min, sub)\
	BOOST_CHECK_EQUAL(ver.major, maj);\
	BOOST_CHECK_EQUAL(ver.minor, min);\
	BOOST_CHECK_EQUAL(ver.subminor, sub);
