#pragma once

#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "ihwio_bus.h"

namespace hwio {

class wrong_format: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

/**
 * Spot devices from description in json file
 **/
class hwio_bus_json: public ihwio_bus {
public:
	using ptree = boost::property_tree::ptree;
	using value_type = boost::property_tree::ptree::value_type;
private:
	void load_devices(ptree& doc);
public:
	std::vector<ihwio_dev *> _all_devices;

	hwio_bus_json(const hwio_bus_json & other) = delete;
	hwio_bus_json(ptree& doc);
	hwio_bus_json(const std::string& file_name);
	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;
	virtual ~hwio_bus_json() override;
};

}
