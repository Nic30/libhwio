#include "hwio_device_mmap.h"

#include <string.h>
#include <sstream>

namespace hwio {

const std::string hwio_device_mmap::DEFAULT_MEM_PATH = "/dev/mem";

hwio_device_mmap::hwio_device_mmap(hwio_comp_spec spec,
		hwio_phys_addr_t base_addr, hwio_phys_addr_t size,
		const std::string & mem_path) :
		hwio_device_mmap((std::vector<hwio_comp_spec> ) { spec }, base_addr,
				size) {
}

hwio_device_mmap::hwio_device_mmap(std::vector<hwio_comp_spec> spec,
		hwio_phys_addr_t base_addr, hwio_phys_addr_t size,
		const std::string & mem_file_name) : mem_file_name(mem_file_name),
		spec(spec), on_bus_base_addr(base_addr), on_bus_size(size) {
	unsigned page_size = sysconf(_SC_PAGESIZE);
	// round up addr space size
	size_t s = on_bus_size;
	if (s < page_size)
		s = page_size;
	if (s > s / page_size)
		s += page_size;
	addr_space_size = s;

	dev_mem = MAP_FAILED;
	fd = -1;
	// page has to be mmaped from the beginning, do address alignment
	page_addr = (base_addr & (~(page_size - 1)));
	page_offset = base_addr - page_addr;
}

void hwio_device_mmap::name(const std::string & name) {
	ihwio_dev::name(name);
	for (auto & s : spec) {
		s.name_set(name);
	}
}

void hwio_device_mmap::attach() {
	if (fd < 0) {
		fd = open(mem_file_name.c_str(), O_RDWR);
		if (fd <= 0) {
			std::stringstream ss;
			ss << "Can not open memory file: " << mem_file_name << ", "
					<< strerror(errno);
			throw hwio_error_dev_init_fail(ss.str());
		}
		/* mmap the device into memory */
		dev_mem = mmap(NULL, addr_space_size, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd, page_addr);

		if (dev_mem == MAP_FAILED)
			throw hwio_error_dev_init_fail("Can not mmap memory");
	}
}

const std::vector<hwio_comp_spec> & hwio_device_mmap::get_spec() const {
	return spec;
}

uint8_t hwio_device_mmap::read8(hwio_phys_addr_t offset) {
	return *((uint8_t *) ((char *) dev_mem + page_offset + offset));
}

uint32_t hwio_device_mmap::read32(hwio_phys_addr_t offset) {
	return *((uint32_t *) ((char *) dev_mem + page_offset + offset));
}

uint64_t hwio_device_mmap::read64(hwio_phys_addr_t offset) {
	uint64_t d = *((uint32_t *) ((char *) dev_mem + page_offset + offset));
	uint64_t tmp = (*((uint32_t *) ((char *) dev_mem + page_offset + offset + sizeof(uint32_t))));
	d |= tmp << sizeof(uint32_t) * 8;
	return d;
	// return *((uint64_t *) ((char *) dev_mem + page_offset + offset));
}

void hwio_device_mmap::write8(hwio_phys_addr_t offset, uint8_t val) {
	char * addr = ((char *) dev_mem + page_offset + offset);
	*((uint8_t *) addr) = val;
}

void hwio_device_mmap::write32(hwio_phys_addr_t offset, uint32_t val) {
	char * addr = ((char *) dev_mem + page_offset + offset);
	*((uint32_t *) addr) = val;
}

void hwio_device_mmap::write64(hwio_phys_addr_t offset, uint64_t val) {
	char * addr = ((char *) dev_mem + page_offset + offset);
	//*(uint64_t *)addr = val;
	*(uint32_t *) addr = (uint32_t) val;
	*(uint32_t *) (addr + sizeof(uint32_t)) = val >> (sizeof(uint32_t) * 8);
}

std::string hwio_device_mmap::to_str() {
	std::stringstream ss;
	const char * attached = (fd > 0 ? "yes" : "no");
	ss << "hwio_device_mmap: (base: 0x" << std::hex << on_bus_base_addr
			<< ", size:0x" << std::hex << on_bus_size << ", attached:"
			<< attached << ", page:0x" << std::hex << page_addr
			<< ", page_offset:0x" << std::hex << page_offset << ")"
			<< std::endl;
	ss << "    spec:" << std::endl;
	for (auto & s : get_spec()) {
		ss << "        " << s.to_str() << std::endl;
	}

	return ss.str();
}

hwio_device_mmap::~hwio_device_mmap() {
	if (dev_mem != (void*) -1)
		munmap(dev_mem, addr_space_size);

	if (fd > 0)
		close(fd);
}

}
