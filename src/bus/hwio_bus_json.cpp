#include "hwio_bus_json.h"

#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <assert.h>

#include "hwio_device_mmap.h"

namespace hwio {

/**
 * Extract data from an json component node and store them
 * in an `hwio_comp' structure.
 * @param src json component node.
 *   json attribute (one of `version', `name', `index', `base' or `size').
 */
hwio_device_mmap * parse_device(boost::property_tree::ptree& src,
		hwio_phys_addr_t offset, const std::string & mem_file_path) {
	auto name = src.get<std::string>("name", "");
	auto base = src.get<std::string>("base", "");
	auto size = src.get<std::string>("size", "");

	const char * err_msg = nullptr;

	if (base == "")
		err_msg = "Missing base attribute";
	else if (size == "")
		err_msg = "Missing size attribute";

	hwio_phys_addr_t _size;
	hwio_phys_addr_t _base;
	std::vector<hwio_comp_spec> specs;
	if (err_msg == nullptr) {
		for (boost::property_tree::ptree::value_type& item : src.get_child("compatible")) {
			assert(item.first.empty());// array elements have no names
			auto compatible_str = item.second.get_value<std::string>();
			hwio_comp_spec spec = {compatible_str.c_str() };
			spec.name_set(name);
			specs.push_back(spec);
		}
		_size = std::stoul(base, nullptr, 16);
		_base = std::stoul(size, nullptr, 16);
	}

	if (err_msg != nullptr)
		throw wrong_format(err_msg);

	return new hwio_device_mmap(specs, offset + _base, _size, mem_file_path);
}

void hwio_bus_json::load_devices(boost::property_tree::ptree& root) {
		std::string mem_file = root.get<std::string>("memfile", hwio_device_mmap::DEFAULT_MEM_PATH);
		std::string offset_str = root.get<std::string>("offset", "0");
		hwio_phys_addr_t offset = std::stoul(offset_str.c_str(), nullptr, 16);
		for (value_type &d : root.get_child("devices")) {
			auto dev = parse_device(d.second, offset, mem_file);
			_all_devices.push_back(dev);
		}
}

hwio_bus_json::hwio_bus_json(boost::property_tree::ptree& doc) {
	load_devices(doc);
}

hwio_bus_json::hwio_bus_json(const std::string& file_name) {
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	if (access(file_name.c_str(), R_OK) != 0) {
		throw std::runtime_error(
				std::string("Specified config file for hwio_bus_json does not exists ")
						+ file_name);
	}
	ptree doc;
	boost::property_tree::read_json(file_name, doc);
	load_devices(doc);
}

std::vector<ihwio_dev *> hwio_bus_json::find_devices(
		const std::vector<hwio_comp_spec> & spec) {
	return filter_device_by_spec(_all_devices, spec);
}

hwio_bus_json::~hwio_bus_json() {
	for (auto dev : _all_devices) {
		delete dev;
	}
}

}
