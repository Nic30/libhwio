#include "hwio_bus_remote.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#include "hwio_device_remote.h"
#include "hwio_comp_spec.h"

namespace hwio {

hwio_bus_remote::hwio_bus_remote(const char * host) :
		server(host) {
	server.connect_to_server();
}

static void copy_str_to_resp(const std::string & src, char * dst) {
	memset(dst, 0, MAX_NAME_LEN);
	strncpy(dst, src.c_str(), MAX_NAME_LEN);
}

void hwio_bus_remote::spec_list_to_query(std::vector<hwio_comp_spec> spec) {
	auto f = reinterpret_cast<HwioFrame<DevQuery> *>(server.tx_buffer);
	f->header.command = HWIO_CMD_QUERY;
	f->header.body_len = sizeof(Dev_query_item) * spec.size();
	int i = 0;
	for (auto & s : spec) {
		auto & item = f->body.items[i];
		copy_str_to_resp(s.name, item.device_name);
		copy_str_to_resp(s.vendor, item.vendor_name);
		copy_str_to_resp(s.type, item.type_name);

		item.version = s.version;
		i++;
	}
}

ihwio_dev * hwio_bus_remote::hwio_dev_from_id(std::vector<hwio_comp_spec> spec,
		dev_id_t id) {
	for (auto d : seen_devices)
		if (((hwio_device_remote*) d)->id == id)
			return d;
	auto d = new hwio_device_remote(spec, &server, id);
	seen_devices.push_back(d);
	return d;
}

std::vector<ihwio_dev *> hwio_bus_remote::find_devices(
		const std::vector<hwio_comp_spec> & spec) {

	spec_list_to_query(spec);
	server.tx_pckt();

	Hwio_packet_header h;
	server.rx_pckt(&h);

	std::vector<ihwio_dev *> res;

	if (h.command != HWIO_CMD_QUERY_RESP)
		throw std::runtime_error("Wrong response on device query");

	auto items = reinterpret_cast<DevQueryResp*>(server.rx_buffer);

	for (unsigned i = 0; i < h.body_len / sizeof(dev_id_t); i++) {
		ihwio_dev * dev = hwio_dev_from_id(spec, items->ids[i]);
		res.push_back(dev);
#ifdef LOG_INFO
		LOG_INFO << "[CLIENT hwio_comp_find_spec]: received item "
				<< (int) items->ids[i] << std::endl;
#endif
	}
	return res;
}

hwio_bus_remote::~hwio_bus_remote() {
	for (auto dev : seen_devices) {
		delete dev;
	}
}

}
