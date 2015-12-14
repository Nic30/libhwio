#pragma once

#include <vector>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "ihwio_bus.h"


namespace hwio {

class xml_wrong_format: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

/**
 * Spot devices from description in file xml
 **/
class hwio_bus_xml: public ihwio_bus {
	void load_devices(xmlDoc * doc);
public:

	std::vector<ihwio_dev *> _all_devices;

	hwio_bus_xml(const hwio_bus_xml & other) = delete;
	hwio_bus_xml(xmlDoc * doc);
	hwio_bus_xml(const char * xml_file_name);
	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;
	virtual ~hwio_bus_xml() override;
};

}
