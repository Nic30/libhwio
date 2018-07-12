#include "hwio_comp_spec.h"
#include "hwio_bus_devicetree.h"
#include "hwio_bus_primitive.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

using namespace std;
namespace hwio {

typedef vector<char*> & path_ref_t;

class device_tree_format_err: public runtime_error {
	using runtime_error::runtime_error;
};

const char * hwio_bus_devicetree::DEFAULT_DEVICE_TREE_PATH = "/proc/device-tree";

char * hwio_bus_devicetree::file_path_from_stack(const char *fname) {
	assert(fname != nullptr);

	size_t plen = 0;
	for (auto p : path_stack) {
		assert(p != nullptr);
		plen += strlen((const char *) p) + 1;
	}

	const size_t fnamelen = strlen(fname);
	char *full = (char *) malloc(plen + fnamelen + 1);

	char *p = full;
	for (auto frag : path_stack) {
		const size_t flen = strlen((const char *) frag);

		memcpy(p, frag, flen);
		p[flen] = '/';
		p += flen + 1;
	}

	assert((size_t ) (p - full) == plen);

	if (fnamelen > 0) {
		memcpy(p, fname, fnamelen);
		p[fnamelen] = '\0';
	} else {
		full[plen - 1] = '\0';
	}

	return full;
}

bool hwio_bus_devicetree::path_stat(const char *fname, struct stat *st) {
	char *fpath = file_path_from_stack(fname);
	if (fpath == nullptr)
		goto clean_and_exit;

	if (stat(fpath, st))
		goto clean_and_exit;

	free(fpath);
	return false;

	clean_and_exit: if (fpath != nullptr)
		free(fpath);
	return true;
}

bool hwio_bus_devicetree::path_is_file(const char *fname) {
	struct stat st;
	if (path_stat(fname, &st))
		return false;

	return S_ISREG(st.st_mode); // is file
}

bool hwio_bus_devicetree::path_is_dir(const char *fname) {
	struct stat st;
	if (path_stat(fname, &st))
		return false;

	return S_ISDIR(st.st_mode);
}

bool hwio_bus_devicetree::dir_has_file(DIR *curr, const char *fname) {
	rewinddir(curr);
	struct dirent *d;

	while ((d = readdir(curr)) != nullptr) {
		if (strcmp(d->d_name, fname))
			continue;

		if (path_is_file(fname)) {
			rewinddir(curr);
			return true;
		}
	}

	rewinddir(curr);
	return false;
}

FILE *hwio_bus_devicetree::path_fopen(const char *fname, const char *mode) {
	char *fpath = file_path_from_stack(fname);
	FILE *file = fopen(fpath, mode);
	free(fpath);
	if (file == nullptr)
		throw device_tree_format_err("Can not open file");

	return file;
}

/*
 * Read content of file and add '\0' to end of data
 * and close file
 * */
static
char *file_read(FILE *file, size_t *flen) {
	struct stat file_stat;
	if (fstat(fileno(file), &file_stat)) {
		fclose(file);
		return nullptr;
	}

	const size_t fsize = file_stat.st_size;

	char *m = new char[fsize + 1];
	if (m == nullptr) {
		fclose(file);
		return nullptr;
	}

	size_t rlen = fread(m, 1, fsize, file);
	if (rlen < fsize) {
		delete[] m;
		fclose(file);
		return nullptr;
	}

	((char *) m)[fsize] = '\0';

	*flen = fsize;
	return m;
}

template<typename T>
void reverse_endianity(T & val) {
	T tmp = 0;
	for (unsigned i = 0; i < sizeof(T); i++) {
		tmp <<= 8;
		tmp |= val & 0xff;
		val >>= 8;
	}
	val = tmp;
}

void hwio_bus_devicetree::dev_parse_reg(const char *fname,
		hwio_phys_addr_t * base, hwio_phys_addr_t * size) {
	FILE *regf = path_fopen(fname, "r");
	assert(regf != nullptr);

	size_t length = 0;
	const char *content = file_read(regf, &length);
	fclose(regf);

	// some devices can have reg file of different size because they have different size of address
	if (length != 2 * sizeof(hwio_phys_addr_t)) {
		delete [] content;
		auto fp = file_path_from_stack(fname);
		auto errmsg = std::string(
				"Base and size has different size than expected ("
						+ to_string(length) + "B), 32b/64b system problem: ")
				+ fp;
		free(fp);
		throw device_tree_format_err(errmsg);
	}

	*base = *((hwio_phys_addr_t*) content);
	*size = *((hwio_phys_addr_t*) (content + (length / 2)));

	reverse_endianity<hwio_phys_addr_t>(*base);
	reverse_endianity<hwio_phys_addr_t>(*size);

	delete[] content;
}

vector<char *> hwio_bus_devicetree::dev_parse_compat(const char *fname) {
	vector<char *> compatibility_strings;
	FILE *regf = path_fopen(fname, "r");

	size_t length = 0;
	const char *content = file_read(regf, &length);
	fclose(regf);
	const char *actual = content;
	size_t actual_length = 0;

	while (actual_length < length) {
		size_t l = strlen(actual);
		compatibility_strings.push_back(strdup(actual));
		actual_length += l + 1;
		actual += l + 1;
	}
	delete[] content;
	return compatibility_strings;
}

hwio_device_mmap *hwio_bus_devicetree::dev_from_dir(DIR *curr) {
	assert(path_stack.size() > 1); // the root is never a device

	char * name = path_stack.back();

	vector<hwio_comp_spec> spec;
	hwio_phys_addr_t base = 0, size = 0;
	struct dirent *d;
	rewinddir(curr);
	while ((d = readdir(curr)) != nullptr) {
		if (!path_is_file(d->d_name))
			continue;

		try {
			if (!strcmp(d->d_name, "reg"))
				dev_parse_reg("reg", &base, &size);
		} catch (const device_tree_format_err & err) {
			rewinddir(curr);
			return nullptr;
		}

		if (!strcmp(d->d_name, "compatible")) {
			for (auto cs : dev_parse_compat("compatible")) {
				spec.push_back(hwio_comp_spec(cs));
				free(cs);
			}
		}
	}
	rewinddir(curr);

	auto dev = new hwio_device_mmap(spec, base, size, mem_path);
	dev->name(name);
	return dev;
}

hwio_bus_devicetree::hwio_bus_devicetree(const char * device_tree_path,
		const char * mem_path) :
		mem_path(mem_path) {

	top_dir_checked_for_dev = false;
	auto * actual_dir = opendir(device_tree_path);
	if (actual_dir == nullptr)
		throw device_tree_format_err(
				std::string("Can not open root device-tree folder:")
						+ device_tree_path);
	dir_stack.push_back(actual_dir);
	path_stack.push_back(strdup(device_tree_path));

	auto dev = device_next();
	while (dev != nullptr) {
		_all_devices.push_back(dev);
		dev = device_next();
	}
	assert(dir_stack.size() == 0);

	for (auto p : path_stack)
		free(p);

	path_stack.clear();
}

/**
 * Open directory from path stored in path stack and add it to dir_stack
 **/
DIR * hwio_bus_devicetree::opendir_on_stack() {
	char *fpath = file_path_from_stack("");
	DIR *dir = opendir(fpath);

	if (dir == nullptr) {
		std::string err_msg(std::string("Can not open dir on stack") + fpath);
		free(fpath);
		throw device_tree_format_err(err_msg);
	} else {
		free(fpath);
	}

	dir_stack.push_back(dir);
	return dir;
}

/**
 * Go to next directory in current directory
 * */
DIR * hwio_bus_devicetree::go_next_dir(DIR *curr) {
	struct dirent *d;

	while ((d = readdir(curr)) != nullptr) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		if (path_is_dir(d->d_name))
			break;
	}
	if (d != nullptr) {
		top_dir_checked_for_dev = false;
		path_stack.push_back(strdup(d->d_name));
		return opendir_on_stack();
	}
	return nullptr;
}

/**
 * Iterator, Searches for another device in devicetree
 *
 * * preorder, depth-first, using dir_stack and recursion of device_next
 *
 **/
hwio_device_mmap * hwio_bus_devicetree::device_next() {
	if (dir_stack.size() == 0)
		return nullptr;

	if (!top_dir_checked_for_dev) {
		auto actual_dir = dir_stack.back();
		top_dir_checked_for_dev = true;
		if (dir_has_file(actual_dir, "reg")) {
			// this dir can be device, try load it
			auto dev = dev_from_dir(actual_dir);
			rewinddir(actual_dir);
			if (dev != nullptr)
				return dev;
		}
		return device_next();
	} else {
		while (dir_stack.size() > 0) {

			// this dir can contains devices, reset directory walking and, walk directories
			DIR *dir = go_next_dir(dir_stack.back());

			// if in directory is not any another directory return from it
			if (dir == nullptr) {
				closedir(dir_stack.back());
				dir_stack.pop_back();
				free(path_stack.back());
				path_stack.pop_back();
			}
			// recursively
			return device_next();

		}
	}

	return nullptr;
}

vector<ihwio_dev *> hwio_bus_devicetree::find_devices(
		const vector<hwio_comp_spec> & spec) {
	vector<ihwio_dev*> devs;
	for (auto d : _all_devices)
		devs.push_back(reinterpret_cast<ihwio_dev*>(d));

	return hwio_bus_primitive::filter_device_by_spec(devs, spec);
}

}
