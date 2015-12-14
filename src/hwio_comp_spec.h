#pragma once

#include "hwio_version.h"

namespace hwio {

/**
 * Component specification structure.
 */
class hwio_comp_spec {
public:
	char * name;
	char * vendor;
	char * type;
	hwio_version version;

	hwio_comp_spec();
	hwio_comp_spec(const hwio_comp_spec & spec);
	hwio_comp_spec(const char * vendor, const char * type,
			hwio_version version);
	hwio_comp_spec(const char * compatibility_str);

	bool operator==(const hwio_comp_spec & other) const;
	bool operator!=(const hwio_comp_spec & other) const;
	hwio_comp_spec & operator=(const hwio_comp_spec & other);

	/**
	 * set name and duplicate it if is not null
	 * */
	void name_set(const char * name);
	const char * name_get();

	std::string to_str() const;

	~hwio_comp_spec();
};

}
