#pragma once

#include <sys/stat.h>
#include <dirent.h>

#include "ihwio_bus.h"
#include "hwio_device_mmap.h"

namespace hwio {

/**
 * Bus with devices generated from device-tree
 * */
class hwio_bus_devicetree: public ihwio_bus {
	const char * mem_path;
	std::vector<char *> actual_path;
	DIR *actual_dir = nullptr;

	hwio_device_mmap * device_next();
	hwio_device_mmap *dev_from_dir(DIR *curr, std::vector<char*> & path);
	DIR * go_next_dir(DIR *curr, std::vector<char *> & path);
	DIR * go_up_next_dir(std::vector<char *> & path);

public:
	std::vector<hwio_device_mmap *> _all_devices;

	static const char * DEFAULT_DEVICE_TREE_PATH;

	hwio_bus_devicetree(
			const char * device_tree_path = DEFAULT_DEVICE_TREE_PATH,
			const char * mem_path = hwio_device_mmap::DEFAULT_MEM_PATH);

	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;

	virtual ~hwio_bus_devicetree() {
		for (auto dev : _all_devices)
			delete dev;
	}
};

}
