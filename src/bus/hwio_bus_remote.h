#pragma once

#include "hwio_remote.h"
#include "ihwio_bus.h"
#include "hwio_client_to_server_con.h"

namespace hwio {

/**
 * Bus which is using devices on remote HWIO server
 */
class hwio_bus_remote: public ihwio_bus {
	hwio_client_to_server_con server;

	/**
	 * Build device query from provided spec into server tx buffer
	 * */
	void spec_list_to_query(std::vector<hwio_comp_spec> spec);
	/**
	 * Vector of seen devices items are of type hwio_device_remote *
	 * */
	std::vector<ihwio_dev *> seen_devices;

public:
	hwio_bus_remote(const hwio_bus_remote & other) = delete;
	hwio_bus_remote(const char * host);

	/**
	 * Lookup device in seen_devices or construct new and add it to seen_devices
	 *
	 * @return hwio_device_remote *
	 * */
	ihwio_dev * hwio_dev_from_id(std::vector<hwio_comp_spec> spec, dev_id_t id);

	/**
	 * Iter devices specified by spec.
	 *
	 * @param spec  vector of device specifications
	 */
	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;

	virtual ~hwio_bus_remote();
};

}
