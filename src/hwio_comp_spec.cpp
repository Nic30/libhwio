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
		name(""), vendor(""), type(""), version() {
}

hwio_comp_spec::hwio_comp_spec(const std::string & _vendor, const std::string & _type,
		hwio_version _version) :
		name(""), vendor(_vendor), type(_type), version(_version) {
}

hwio_comp_spec::hwio_comp_spec(const std::string & compatibility_str) :
		name(""), vendor(""), type(""), version() {
	/**
	 * Parses the given compat string into the spec instance.
	 */
	const size_t len = compatibility_str.size();

	// should look like this:
	//  <vendor>,<type>-<version>
	size_t comma = compatibility_str.find_last_of(',');
	size_t hyphen = compatibility_str.find_last_of('-');
	size_t type_begin = comma + 1;
	size_t version_begin = hyphen + 1;
	size_t type_end = hyphen;
	size_t vendor_end = comma;

	if (comma == std::string::npos && hyphen == std::string::npos) {
		// <vendor>
		type = compatibility_str;
	} else if (comma != std::string::npos && hyphen == std::string::npos) {
		// <vendor>,<type>
		type = type_begin;
		vendor = compatibility_str.substr(0, vendor_end);

	} else if (comma == std::string::npos && hyphen != std::string::npos) {
		//  <type>-<version> or <type> only, with - in type
		try {
			version = hwio_version(compatibility_str.substr(version_begin, compatibility_str.size()));
		} catch (const std::runtime_error & e) {
			type_end = len;
		}
		type = compatibility_str.substr(0, type_end);

	} else {
		//  <vendor>,<type>-<version>
		// or <vendor>,<type> only, with - in type
		type_end = hyphen - type_begin;
		try {
			version = hwio_version(compatibility_str.substr(version_begin, compatibility_str.size()));
		} catch (const std::runtime_error & e) {
			type_end = len;
		}
		type = compatibility_str.substr(type_begin, type_end);
		vendor = compatibility_str.substr(0, vendor_end);
	}
}
void hwio_comp_spec::name_set(const std::string & name) {
	this->name = name;
}

const std::string & hwio_comp_spec::name_get() {
	return name;
}

std::string hwio_comp_spec::to_str() const {
	std::stringstream ss;
	ss << name << ":" << vendor << "," << type << "-" << version.to_str();
	return ss.str();
}

bool hwio_comp_spec::operator==(const hwio_comp_spec & other) const {
	if (name != "" && other.name != "")
		return name == other.name;

	if (vendor != "" && other.vendor != ""
			&& vendor != other.vendor)
		return 0;

	if (type != "" && other.type != "" && type != other.type)
		return 0;

	return version == other.version;
}

bool hwio_comp_spec::operator!=(const hwio_comp_spec & other) const {
	return !(*this == other);
}

}
