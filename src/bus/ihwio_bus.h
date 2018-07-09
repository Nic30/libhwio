#pragma once

#include <vector>

#include "hwio_comp_spec.h"
#include "ihwio_dev.h"

namespace hwio {

/**
 * Interface for device lookup
 */
class ihwio_bus {
public:
	static std::vector<ihwio_dev*> filter_device_by_spec(
			const std::vector<ihwio_dev*> devices,
			const std::vector<hwio_comp_spec> spec) {
		/**
		 * * Name is identifier of device
		 * * Device can have more compatibility strings
		 **/
		std::vector<ihwio_dev *> res;
		for (auto dev : devices) {
			auto dev_specs = dev->get_spec();
			if (dev_specs.size() == 0) {
				// search all name matches
				auto name = dev->name();
				if (name != "")
					for (auto & s : spec) {
						if (s.name != "" && name == s.name) {
							res.push_back(dev);
							break;
						}
					}
			} else {
				// search all compatibility string matches
				bool matched = false;
				for (auto & s : spec) {
					for (auto & ds : dev_specs) {
						if (ds == s) {
							res.push_back(dev);
							matched = true;
							break;
						}
					}
					if (matched)
						break;
				}
			}
		}
		return res;
	}
	/**
	 * Find devices devices specified by spec.
	 *
	 * @param spec name, type, vector and version of device specifications
	 */
	virtual std::vector<ihwio_dev *> find_devices(
			const std::vector<hwio_comp_spec> & spec) = 0;

	virtual ~ihwio_bus() {
	}
};

}
