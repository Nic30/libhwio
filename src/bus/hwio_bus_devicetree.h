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
	const std::string mem_path;
	std::vector<char *> path_stack;
	std::vector<DIR *> dir_stack;
	bool top_dir_checked_for_dev;

	hwio_device_mmap * device_next();
	hwio_device_mmap * dev_from_dir(DIR *curr);
	DIR * go_next_dir(DIR *curr, std::vector<char *> & path);

	/**
	 * @param fname name of file in actual path
	 * */
	bool path_is_file(const char *fname);
	/**
	 * @param fname name of file in actual path
	 * */
	bool path_is_dir(const char *fname);
	/**
	 * @param fname name of file in actual path
	 * */
	bool dir_has_file(DIR *curr, const char *fname);

	bool path_stat(const char *fname, struct stat *st);
	char * file_path_from_stack(const char *fname);
	FILE * path_fopen(const char *fname, const char *mode);
	void dev_parse_reg(const char *fname, hwio_phys_addr_t * base,
			hwio_phys_addr_t * size);
	std::vector<char *> dev_parse_compat(const char *fname);
	DIR * opendir_on_stack();
	DIR * go_next_dir(DIR *curr);

public:
	std::vector<hwio_device_mmap *> _all_devices;

	static const std::string DEFAULT_DEVICE_TREE_PATH;

	hwio_bus_devicetree(
			const std::string & device_tree_path = DEFAULT_DEVICE_TREE_PATH,
			const std::string & mem_path = hwio_device_mmap::DEFAULT_MEM_PATH);

	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;

	virtual ~hwio_bus_devicetree() {
		for (auto dev : _all_devices)
			delete dev;
	}
};

}
