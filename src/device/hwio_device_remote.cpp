#include "hwio_device_remote.h"

#include <assert.h>
#include <sstream>

using namespace std;
namespace hwio {

void hwio_device_remote::assert_response(Hwio_packet_header * h,
		HWIO_CMD expected, const string & msg) {
	if (h->command != expected) {
		stringstream ss;
		ss << msg << (int) h->command;
		if (h->command == HWIO_CMD_MSG) {
			auto err = reinterpret_cast<ErrMsg*>(server->rx_buffer);
			ss << err->err_code << ": " << err->msg;
		}
		throw hwio_error_rw(ss.str());
	}
}

hwio_device_remote::hwio_device_remote(vector<hwio_comp_spec> spec,
		hwio_client_to_server_con * server, dev_id_t id) :
		server(server), id(id), spec(spec) {
}

const vector<hwio_comp_spec> & hwio_device_remote::get_spec() const {
	return spec;
}

void hwio_device_remote::attach() {
	// devices are automatically attached on server
}

void hwio_device_remote::read(hwio_phys_addr_t offset, void *__restrict dst,
		size_t n) {

#ifdef LOG_INFO
	LOG_INFO << "[CLIENT] Read " << (int) id << ": " << name_get() << " 0x"
	<< hex << offset << ", " << dec << endl;
#endif
	assert(
			n <= UINT16_MAX && "[TODO] split large data to multiple transactions");
	auto buff = reinterpret_cast<HwioFrame<RdReq>*>(server->tx_buffer);
	buff->header.body_len = sizeof(RdReq);
	buff->header.command = HWIO_CMD_READ;
	buff->body.addr = offset;
	buff->body.devId = id;
	buff->body.size = n;

	server->tx_pckt();

	Hwio_packet_header h;
	server->rx_pckt(&h);
	assert_response(&h, HWIO_CMD_READ_RESP,
			"Wrong response from server on read request ");

	auto resp = reinterpret_cast<RdResp*>(server->rx_buffer);
#ifdef LOG_INFO
	LOG_INFO << "[CLIENT] Read data: " << resp->data << endl;
#endif

	memcpy(dst, (void *) resp->data, n);
}

uint8_t hwio_device_remote::read8(hwio_phys_addr_t offset) {
	uint8_t res;
	read(offset, &res, sizeof(res));
	return res;
}

uint32_t hwio_device_remote::read32(hwio_phys_addr_t offset) {
	uint32_t res;
	read(offset, &res, sizeof(res));
	return res;
}

uint64_t hwio_device_remote::read64(hwio_phys_addr_t offset) {
	uint64_t res;
	read(offset, &res, sizeof(res));
	return res;
}

void hwio_device_remote::write(hwio_phys_addr_t offset, const void * data,
		size_t n) {
	assert(
			n <= UINT16_MAX && "[TODO] split large data to multiple transactions");
#ifdef LOG_INFO
	LOG_INFO << "[CLIENT] Write " << (int) id << ": " << name_get() << " 0x"
	<< hex << offset << ", " << dec << data << endl;
#endif
	auto buff = reinterpret_cast<HwioFrame<WrReq>*>(server->tx_buffer);
	buff->header.body_len = sizeof(buff->body._) + n;
	buff->header.command = HWIO_CMD_WRITE;
	buff->body._.addr = offset;
	buff->body._.devId = id;
	buff->body._.size = id;

	memcpy(buff->body.data, data, n);
	server->tx_pckt();
}
void hwio_device_remote::write8(hwio_phys_addr_t offset, uint8_t val) {
	write(offset, &val, sizeof(val));
}
void hwio_device_remote::write32(hwio_phys_addr_t offset, uint32_t val) {
	write(offset, &val, sizeof(val));
}
void hwio_device_remote::write64(hwio_phys_addr_t offset, uint64_t val) {
	write(offset, &val, sizeof(val));
}

hwio_device_remote::~hwio_device_remote() {
}

}
