#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdexcept>
#include <vector>
#include <string.h>

#include "hwio_typedefs.h"
#include "hwio_comp_spec.h"

namespace hwio {

/*
 * Exception raised on read/write error in ihwio_dev
 * */
class hwio_error_rw: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

/*
 * Exception raised on error in hwio_dev initialization
 * */
class hwio_error_dev_init_fail: public std::runtime_error {
	using std::runtime_error::runtime_error;
};

/*
 * Interface for device classes
 * contains virtual read and write methods
 * */
class ihwio_dev {
	std::string _M_name;
public:
	ihwio_dev() :
			_M_name("") {
	}
	virtual const std::vector<hwio_comp_spec> & get_spec() const = 0;

	/*
	 * Allocate device to allow access to device
	 * @throw hwio_error_dev_init_fail
	 * */
	virtual void attach() = 0;

	/**
	 * Read data from the component. Platform dependent.
	 *
	 * @param offset The offset relative to the component's base.
	 * @param dst Pointer on buffer to store data in
	 * @param n Size of data to read
	 * @throw hwio_error_rw
	 */
	virtual void read(hwio_phys_addr_t offset, void *__restrict dst, size_t n) {
		while (true) {
			//if (n >= sizeof(uint64_t)) {
			//	*(uint64_t*) dst = read64(offset);
			//	n -= sizeof(uint64_t);
			//	offset += sizeof(uint64_t);
			//	dst = ((uint8_t*) dst) + sizeof(uint64_t);
			//	continue;
            //
			//} else
			if (n >= sizeof(uint32_t)) {
				*(uint32_t*) dst = read32(offset);
				n -= sizeof(uint32_t);
				offset += sizeof(uint32_t);
				dst = ((uint8_t*) dst) + sizeof(uint32_t);
				continue;

			} else {
				for (unsigned i = 0; i < n; i++)
					*((uint8_t*) dst + i) = read8(offset + i);
				return;
			}
		}
	}
	virtual uint8_t read8(hwio_phys_addr_t offset) = 0;
	virtual uint32_t read32(hwio_phys_addr_t offset) = 0;
	virtual uint64_t read64(hwio_phys_addr_t offset) = 0;

	/**
	 * Write data into the component.
	 *
	 * @param offset The offset relative to the component's base.
	 * @param c data to fill inn
	 * @param n specified number of bytes
	 * @throw hwio_error_rw
	 */
	virtual void memset(hwio_phys_addr_t offset, uint8_t c, size_t n) {
		uint32_t fill32 = c;
		fill32 |= c << 8 | c;
		fill32 |= fill32 << 2 * 8;
		uint64_t fill64 = (((uint64_t) fill32) << sizeof(uint32_t) * 8)
				| fill32;

		while (true) {
			//if (n >= sizeof(uint64_t)) {
			//	write64(offset, fill64);
			//	n -= sizeof(uint64_t);
			//	offset += sizeof(uint64_t);
			//	continue;
            //
			//} else
			if (n >= sizeof(uint32_t)) {
				write32(offset, fill32);
				n -= sizeof(uint32_t);
				offset += sizeof(uint32_t);
				continue;

			} else {
				for (unsigned i = 0; i < n; i++)
					write8(offset + i, c);
				return;
			}
		}
	}

	/*
	 * Write data of size n to device on specified offset
	 *
	 * @param offset The offset relative to the component's base.
	 * @param data The data to write into component
	 * @param n Size of data
	 * @throw hwio_error_rw
	 * */
	virtual void write(hwio_phys_addr_t offset, const void * data, size_t n) {
		while (true) {
			//if (n >= sizeof(uint64_t)) {
			//	write64(offset, *(uint64_t*) data);
			//	n -= sizeof(uint64_t);
			//	offset += sizeof(uint64_t);
			//	data = ((uint8_t*) data) + sizeof(uint64_t);
			//	continue;
            //
			//} else
			if (n >= sizeof(uint32_t)) {
				write32(offset, *(uint32_t*) data);
				n -= sizeof(uint32_t);
				offset += sizeof(uint32_t);
				data = ((uint8_t*) data) + sizeof(uint32_t);
				continue;

			} else {
				for (unsigned i = 0; i < n; i++)
					write8(offset + i, *((uint8_t*) data + i));
				return;
			}
		}
	}

	virtual void write8(hwio_phys_addr_t offset, uint8_t val) = 0;
	virtual void write32(hwio_phys_addr_t offset, uint32_t val) = 0;
	virtual void write64(hwio_phys_addr_t offset, uint64_t val) = 0;

	/**
	 * == operator for unique component searching
	 */
	virtual bool operator==(const ihwio_dev & other) const {
		if (this == &other)
			return true;
		auto specA = this->get_spec();
		auto specB = other.get_spec();
		unsigned i = 0;
		for (auto s : specA) {
			if (s != specB.at(i))
				return false;
			i++;
		}
		return true;
	}

	virtual bool operator!=(const ihwio_dev & other) const {
		return !(*this == other);
	}

	virtual void name(const std::string & name) {
		this->_M_name = name;
	}

	virtual const std::string & name() {
		return _M_name;
	}


	/**
	 * to string conversion for debug purposes
	 ***/
	virtual std::string to_str() = 0;

	virtual ~ihwio_dev() {
	}
};

}
