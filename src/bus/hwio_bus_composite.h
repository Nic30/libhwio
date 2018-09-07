#pragma once

#include "ihwio_bus.h"


namespace hwio {

/**
 * Containter of vector<ihwio_bus*> which represetns them as one
 **/
class hwio_bus_composite: public ihwio_bus {
public:
	std::vector<ihwio_bus*> _buses;

	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) override;

	virtual ~hwio_bus_composite() override {
		for (auto b : _buses) {
			delete b;
		}
	}
};

}
