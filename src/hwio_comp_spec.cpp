#include "hwio_comp_spec.h"

#include <string.h>
#include <assert.h>
#include <stdexcept>
#include <sstream>

namespace hwio {

hwio_comp_spec::hwio_comp_spec(const hwio_comp_spec & spec) :
		hwio_comp_spec(spec.vendor, spec.type, spec.version) {
	name_set(spec.name);
}

hwio_comp_spec::hwio_comp_spec() :
		name(nullptr), vendor(nullptr), type(nullptr), version() {
}

hwio_comp_spec::hwio_comp_spec(const char * _vendor, const char * _type,
		hwio_version _version) :
		name(nullptr), vendor(nullptr), type(nullptr), version(_version) {
	if (_vendor != nullptr) {
		this->vendor = strdup(_vendor);
		assert(this->vendor != nullptr);
	}
	if (_type != nullptr) {
		this->type = strdup(_type);
		assert(this->type != nullptr);
	}
}

hwio_comp_spec::hwio_comp_spec(const char * compatibility_str) :
		name(nullptr), vendor(nullptr), type(nullptr), version() {
	/**
	 * Parses the given compat string into the spec instance.
	 */
	const size_t len = strlen(compatibility_str);

	// should look like this:
	//  <vendor>,<type>-<version>
	const char *comma = (const char *) memchr(compatibility_str, ',', len);
	const char *hyphen = (const char *) memrchr(compatibility_str, '-', len);
	const char * type_str = ((char *) comma) + 1;
	const char * version_str = ((char *) hyphen) + 1;
	size_t type_len = hyphen - compatibility_str;
	size_t vendor_len = comma - compatibility_str;

	if (comma == nullptr && hyphen == nullptr) {
		// <vendor>
		type = strdup(compatibility_str);
	} else if (comma != nullptr && hyphen == nullptr) {
		// <vendor>,<type>
		type = strdup(type_str);
		vendor = strndup(compatibility_str, vendor_len);

	} else if (comma == nullptr && hyphen != nullptr) {
		//  <type>-<version> or <type> only, with - in type
		try {
			version = hwio_version(version_str);
		} catch (const std::runtime_error & e) {
			version = hwio_version();
			type_len = strlen(compatibility_str);
		}
		type = strndup(compatibility_str, type_len);

	} else {
		//  <vendor>,<type>-<version>
		// or <vendor>,<type> only, with - in type
		type_len = hyphen - type_str;
		try {
			version = hwio_version(version_str);
		} catch (const std::runtime_error & e) {
			version = hwio_version();
			type_len = strlen(comma + 1);
		}
		type = strndup(type_str, type_len);
		vendor = strndup(compatibility_str, vendor_len);
	}
}
void hwio_comp_spec::name_set(const char * name) {
	if (this->name != nullptr)
		free(this->name);
	if (name == nullptr)
		this->name = nullptr;
	else
		this->name = strdup(name);
}

const char * hwio_comp_spec::name_get() {
	if (name == nullptr)
		return "";
	else
		return name;
}

std::string hwio_comp_spec::to_str() const {
	std::stringstream ss;
	if (name == nullptr)
		ss << "<NO_NAME>";
	else
		ss << name;
	ss << ":";
	if (vendor == nullptr)
		ss << "<NO_VENDOR>";
	else
		ss << vendor;
	ss << ",";

	if (type == nullptr)
		ss << "<NO_TYPE>";
	else
		ss << type;

	ss << "-" << version.to_str();

	return ss.str();
}

bool hwio_comp_spec::operator==(const hwio_comp_spec & other) const {
	if (name != nullptr && other.name != nullptr)
		return strcmp(name, other.name) == 0;

	if (vendor != nullptr && other.vendor != nullptr
			&& strcmp(vendor, other.vendor))
		return 0;

	if (type != nullptr && other.type != nullptr && strcmp(type, other.type))
		return 0;

	return version == other.version;
}

bool hwio_comp_spec::operator!=(const hwio_comp_spec & other) const {
	return !(*this == other);
}

/**
 * Override default copy constructor to take care of strings
 **/
hwio_comp_spec & hwio_comp_spec::operator=(const hwio_comp_spec & other) {
	name = other.name;
	vendor = other.vendor;
	type = other.type;
	version = other.version;

	if (name != nullptr) {
		name = strdup(name);
		assert(name != nullptr);
	}

	if (vendor != nullptr) {
		vendor = strdup(vendor);
		assert(vendor != nullptr);
	}

	if (type != nullptr) {
		type = strdup(type);
		assert(type != nullptr);
	}

	return *this;
}

hwio_comp_spec::~hwio_comp_spec() {
	if (name != nullptr)
		free(name);
	if (vendor != nullptr)
		free(vendor);
	if (type != nullptr)
		free(type);
}

}
