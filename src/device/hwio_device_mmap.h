#pragma once

#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#include "hwio_typedefs.h"
#include "ihwio_dev.h"
#include "hwio_comp_spec.h"

namespace hwio {

/*
 * Device mmaped from /dev/mem or other file
 * */
class hwio_device_mmap: public ihwio_dev {
	// informations about device
	std::vector<hwio_comp_spec> spec;

	// file descriptor of file for mmaped memory
	int fd;
	// page addr and offset where is the memory of device mmaped
	hwio_phys_addr_t page_addr, page_offset;
	// size of mmaped address space
	size_t addr_space_size;
	// pointer on mmaped device memory
	void *dev_mem;

	// name of file from which device should be mmaped
	const char * mem_file_name;

public:
	// base address and size of device from bus
	const hwio_phys_addr_t on_bus_base_addr;
	const hwio_phys_addr_t on_bus_size;
	static const char * DEFAULT_MEM_PATH;
	/*
	 * @param devI base address where address space of device starts
	 **/
	hwio_device_mmap(const hwio_device_mmap & other) = delete;
	hwio_device_mmap(hwio_comp_spec spec, hwio_phys_addr_t base_addr,
			hwio_phys_addr_t size, const char * mem_path = DEFAULT_MEM_PATH);
	hwio_device_mmap(std::vector<hwio_comp_spec> spec,
			hwio_phys_addr_t base_addr, hwio_phys_addr_t size,
			const char * mem_path = DEFAULT_MEM_PATH);
	virtual void name(const std::string & name) override;

	virtual void attach() override;
	virtual const std::vector<hwio_comp_spec> & get_spec() const override;

	virtual uint8_t read8(hwio_phys_addr_t offset) override;
	virtual uint32_t read32(hwio_phys_addr_t offset) override;
	virtual uint64_t read64(hwio_phys_addr_t offset) override;

	virtual void write8(hwio_phys_addr_t offset, uint8_t val) override;
	virtual void write32(hwio_phys_addr_t offset, uint32_t val) override;
	virtual void write64(hwio_phys_addr_t offset, uint64_t val) override;

	virtual ~hwio_device_mmap() override;
};

}

