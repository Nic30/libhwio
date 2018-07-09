#pragma once

#include "hwio_version.h"

namespace hwio {

/**
 * Component specification structure.
 */
class hwio_comp_spec {
public:
	std::string name;
	std::string vendor;
	std::string type;
	hwio_version version;

	hwio_comp_spec();
	hwio_comp_spec(const hwio_comp_spec & spec);
	hwio_comp_spec(const std::string & vendor, const std::string & type,
			hwio_version version);
	hwio_comp_spec(const std::string & compatibility_str);

	bool operator==(const hwio_comp_spec & other) const;
	bool operator!=(const hwio_comp_spec & other) const;

	/**
	 * set name and duplicate it if is not null
	 * */
	void name_set(const std::string & name);
	const std::string & name_get();

	std::string to_str() const;
};

}
