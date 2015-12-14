#pragma once

#include <stdexcept>

namespace hwio {

/**
 * Version component not available.
 */
#define HWIO_VERSION_NA -1

/**
 * Container of version numbers.
 */
struct __attribute__((packed)) hwio_version {
	int major;
	int minor;
	int subminor;

	/**
	 * Parses a version string in the form a.b.c, where a and b are decimal numbers
	 * and c is either a number or an alpha character.
	 *
	 * @throw std::runtime_error
	 */
	hwio_version(const char * version_str);
	hwio_version(int major = HWIO_VERSION_NA, int minor = HWIO_VERSION_NA,
			int subminor = HWIO_VERSION_NA);

	/**
	 * Check the version against the version pattern.
	 * @param other Version with possible wildcards (HWIO_VERSION_NA).
	 *   If some field is HWIO_VERSION_NA, it matches.
	 * @return Returns true if the versions match, false otherwise.
	 */
	bool operator==(const hwio_version& other) const;
	bool operator!=(const hwio_version& other) const;
	std::string to_str() const;
};

}
