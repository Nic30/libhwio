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

static
char *file_path_from_stack(path_ref_t path, const char *fname) {
	assert(fname != nullptr);

	const size_t fnamelen = strlen(fname);
	size_t plen = 0;

	for (auto p : path) {
		assert(p != nullptr);
		plen += strlen((const char *) p) + 1;
	}

	char *full = (char *) malloc(plen + fnamelen + 1);

	char *p = full;
	for (auto frag : path) {
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

static
int path_stat(path_ref_t path, const char *fname, struct stat *st) {
	char *fpath = file_path_from_stack(path, fname);
	if (fpath == nullptr)
		goto clean_and_exit;

	if (stat(fpath, st))
		goto clean_and_exit;

	free(fpath);
	return 0;

	clean_and_exit: if (fpath != nullptr)
		free(fpath);
	return 1;
}

static
int path_is_file(path_ref_t path, const char *fname) {
	struct stat st;
	if (path_stat(path, fname, &st))
		return 0;

	return S_ISREG(st.st_mode); // is file
}

static
int path_is_dir(path_ref_t path, const char *fname) {
	struct stat st;
	if (path_stat(path, fname, &st))
		return 0;

	return S_ISDIR(st.st_mode);
}

static
int dir_has_file(DIR *curr, path_ref_t path, const char *fname) {
	rewinddir(curr);
	struct dirent *d;

	while ((d = readdir(curr)) != nullptr) {
		if (strcmp(d->d_name, fname))
			continue;

		if (path_is_file(path, fname))
			return 1;
	}

	return 0;
}

static FILE *path_fopen(path_ref_t path, const char *fname, const char *mode) {
	char *fpath = file_path_from_stack(path, fname);
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

static
void dev_parse_reg(path_ref_t path, const char *fname, hwio_phys_addr_t * base,
		hwio_phys_addr_t * size) {
	FILE *regf = path_fopen(path, fname, "r");
	assert(regf != nullptr);

	size_t length = 0;
	const char *content = file_read(regf, &length);
	fclose(regf);

	// some devices can have reg file of different size because they have different size of address
	if (length != 2 * sizeof(hwio_phys_addr_t)) {
		free((void *) content);
		auto fp = file_path_from_stack(path, fname);
		auto errmsg = std::string(
				"Base and size has different size than expected ("
						+ to_string(length) + "B), 32b/64b system problem: ")
				+ fp;
		free(fp);
		throw device_tree_format_err(errmsg);
	}

	*base = *((hwio_phys_addr_t*) content);
	*size = *((hwio_phys_addr_t*) (content + (length / 2)));

	delete[] content;
}

static vector<char *> dev_parse_compat(path_ref_t path, const char *fname) {
	vector<char *> compatibility_strings;
	FILE *regf = path_fopen(path, fname, "r");

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

hwio_device_mmap *hwio_bus_devicetree::dev_from_dir(DIR *curr,
		path_ref_t path) {
	assert(path.size() > 1); // the root is never a device

	char * name = path.back();

	vector<hwio_comp_spec> spec;
	hwio_phys_addr_t base = 0, size = 0;
	struct dirent *d;
	rewinddir(curr);
	while ((d = readdir(curr)) != nullptr) {
		if (!path_is_file(path, d->d_name))
			continue;

		try {
			if (!strcmp(d->d_name, "reg"))
				dev_parse_reg(path, "reg", &base, &size);
		} catch (const device_tree_format_err & err) {
			return nullptr;
		}

		if (!strcmp(d->d_name, "compatible")) {
			for (auto cs : dev_parse_compat(path, "compatible")) {
				spec.push_back(hwio_comp_spec(cs));
				free(cs);
			}
		}
	}

	auto dev = new hwio_device_mmap(spec, base, size, mem_path);
	dev->name(name);
	return dev;
}

hwio_bus_devicetree::hwio_bus_devicetree(const char * device_tree_path,
		const char * mem_path) :
		mem_path(mem_path) {

	actual_dir = opendir(device_tree_path);
	if (actual_dir == nullptr)
		throw device_tree_format_err("Can not open root device-tree folder");
	actual_path.push_back(strdup(device_tree_path));
	auto dev = device_next();
	while (dev != nullptr) {
		_all_devices.push_back(dev);
		dev = device_next();
	}

	if (actual_dir != nullptr) {
		closedir(actual_dir);
		actual_dir = nullptr;
	}

	for (auto p : actual_path)
		free(p);
	actual_path.clear();
}

static DIR *opendir_on_stack(path_ref_t path) {
	char *fpath = file_path_from_stack(path, "");
	DIR *dir = opendir(fpath);
	free(fpath);
	if (dir == nullptr)
		throw device_tree_format_err("Can not open dir on stack");

	return dir;
}

DIR * hwio_bus_devicetree::go_next_dir(DIR *curr, path_ref_t path) {
	struct dirent *d;

	while ((d = readdir(curr)) != nullptr) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		if (path_is_dir(path, d->d_name))
			break;
	}
	if (d != nullptr) {
		path.push_back(strdup(d->d_name));
		return opendir_on_stack(path);
	}
	return nullptr;
}

DIR * hwio_bus_devicetree::go_up_next_dir(vector<char *> & path) {
	DIR *next = nullptr;

	do {
		if (path.size() == 1) // never loose the root dir
			return nullptr;

		const char *dname = (const char *) path.back();
		path.pop_back();

		DIR *dir = opendir_on_stack(path);
		if (dir == nullptr) {
			free((void *) dname);
			return nullptr;
		}

		struct dirent *d;
		while ((d = readdir(dir)) != nullptr) {
			if (!strcmp(d->d_name, dname))
				break;
		}
		free((void *) dname);

		next = go_next_dir(dir, path);
		if (next == nullptr) {
			closedir(dir);
			continue;
		}

		closedir(dir);
	} while (next == nullptr);

	return next;
}

hwio_device_mmap * hwio_bus_devicetree::device_next() {
	if (actual_dir == nullptr)
		return nullptr;

	hwio_device_mmap * dev = nullptr;

	while (dev == nullptr && actual_dir != nullptr) {
		if (dir_has_file(actual_dir, actual_path, "reg")) {
			dev = dev_from_dir(actual_dir, actual_path);

			if (dev == nullptr)
				return nullptr;
		}

		rewinddir(actual_dir);
		DIR *dir = go_next_dir(actual_dir, actual_path);

		if (dir == nullptr)
			dir = go_up_next_dir(actual_path);

		closedir(actual_dir);
		actual_dir = dir;
	}

	return dev;
}

vector<ihwio_dev *> hwio_bus_devicetree::find_devices(
		const vector<hwio_comp_spec> & spec) {
	vector<ihwio_dev*> devs;
	for (auto d : _all_devices)
		devs.push_back(reinterpret_cast<ihwio_dev*>(d));

	return hwio_bus_primitive::filter_device_by_spec(devs, spec);
}

}
