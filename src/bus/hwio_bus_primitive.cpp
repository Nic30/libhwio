#include "hwio_bus_primitive.h"

namespace hwio {

std::vector<ihwio_dev *> hwio_bus_primitive::find_devices(
		const std::vector<hwio_comp_spec> & spec) {
	return filter_device_by_spec(_all_devices, spec);
}

}
