#include "hwio_bus_composite.h"

namespace hwio {

std::vector<ihwio_dev *> hwio_bus_composite::find_devices(
		const std::vector<hwio_comp_spec> & spec) {
	std::vector<ihwio_dev*> res;
	for(auto b: _buses) {
		for (auto d: b->find_devices(spec)) {
			res.push_back(d);
		}
	}
	return res;
}

}
