#pragma once

#include "ihwio_bus.h"

namespace hwio {

/**
 * Bus which has vector of devices and can perform device lookup from this vector
 * */
class hwio_bus_primitive: public ihwio_bus {
public:
	std::vector<ihwio_dev *> _all_devices;
	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;

	virtual ~hwio_bus_primitive() {
	}
};

}
