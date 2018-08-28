#include "hwio_version.h"

#include <stdlib.h>
#include <assert.h>
#include <regex>
#include <stdexcept>
#include <sstream>

namespace hwio {

bool hwio_version::operator==(const hwio_version& other) const {
	if (other.major != HWIO_VERSION_NA && other.major != major)
		return false;

	if (other.minor != HWIO_VERSION_NA && other.minor != minor)
		return false;

	if (other.subminor != HWIO_VERSION_NA && other.subminor != subminor)
		return false;

	return true;
}

bool hwio_version::operator!=(const hwio_version& other) const {
	return !(*this == other);
}

std::string hwio_version::to_str() const {
	std::stringstream ss;
	if (major == HWIO_VERSION_NA)
		ss << "<NA>";
	else
		ss << major;
	ss << ".";
	if (minor == HWIO_VERSION_NA)
		ss << "<NA>";
	else
		ss << minor;
	ss << ".";
	if (subminor == HWIO_VERSION_NA)
		ss << "<NA>";
	else
		ss << subminor;
	return ss.str();
}

hwio_version::hwio_version(int major, int minor, int subminor) {
	// initializers not used due name conflict with sysmacros.h
	this->major = major;
	this->minor = minor;
	this->subminor = subminor;
}

static std::regex hwio_version_regex("^(\\d+)\\.(\\d+)(\\.(\\w|\\d+))?");
hwio_version::hwio_version(const std::string & version_str) {
	std::smatch m;
	auto v = version_str;
	if (std::regex_match(v, m, hwio_version_regex)) {
		major = std::stoi(m[1].str());
		minor = std::stoi(m[2].str());
		auto sm = m[4];
		if (sm.length()) {
			auto sm_str = sm.str();
			if (std::isalpha(sm_str[0])) {
				subminor = int(sm_str[0]);
			} else {
				subminor = std::stoi(sm_str);
			}
		} else {
			subminor = HWIO_VERSION_NA;
		}
	} else
		throw std::runtime_error("[HWIO] Wrong hwio_version format");
}

}

