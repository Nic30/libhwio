#pragma once

#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#include "ihwio_dev.h"
#include "hwio_typedefs.h"
#include "hwio_remote.h"
#include "hwio_client_to_server_con.h"

namespace hwio {

/*
 * Client side representation of device on remote HWIO server
 * */
class hwio_device_remote: public ihwio_dev {
	hwio_client_to_server_con * server;
	void assert_response(Hwio_packet_header * h, HWIO_CMD expected, const std::string & msg);
public:
	dev_id_t id;
	std::vector<hwio_comp_spec> spec;

	hwio_device_remote(std::vector<hwio_comp_spec> spec,
			hwio_client_to_server_con * server, dev_id_t id);

	virtual void attach() override;
	virtual const std::vector<hwio_comp_spec> & get_spec() const override;

	virtual void read(hwio_phys_addr_t offset, void *__restrict dst, size_t n)
			override;
	virtual uint8_t read8(hwio_phys_addr_t offset) override;
	virtual uint32_t read32(hwio_phys_addr_t offset) override;
	virtual uint64_t read64(hwio_phys_addr_t offset) override;

	virtual void write(hwio_phys_addr_t offset, const void * data, size_t n)
			override;
	virtual void write8(hwio_phys_addr_t offset, uint8_t val) override;
	virtual void write32(hwio_phys_addr_t offset, uint32_t val) override;
	virtual void write64(hwio_phys_addr_t offset, uint64_t val) override;

	virtual ~hwio_device_remote() override;
};

}
