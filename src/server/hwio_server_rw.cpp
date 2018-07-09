#include "hwio_server.h"

using namespace std;
using namespace hwio;

HwioServer::PProcRes HwioServer::handle_read(ClientInfo * client,
		Hwio_packet_header header) {
	const RdReq* rdReq;

	if (header.body_len != sizeof(RdReq))
		return send_err(MALFORMED_PACKET, "READ: size too small");

	rdReq = reinterpret_cast<const RdReq*>(rx_buffer);
	ihwio_dev * dev = client_get_dev(client, rdReq->devId);
	if (!dev) {
		return send_err(ACCESS_DENIED, "READ: device is not allocated");
	}

#ifdef LOG_INFO
	LOG_INFO << "READ:" << (int) rdReq->devId << " 0x" << hex << rdReq->addr
	<< dec << endl;
#endif

	HwioFrame<RdResp> * resp = reinterpret_cast<HwioFrame<RdResp>*>(tx_buffer);
	resp->header.command = HWIO_CMD_READ_RESP;
	resp->header.body_len = rdReq->size;
	try {
		dev->read(rdReq->addr, resp->body.data, rdReq->size);
	} catch (std::runtime_error & err) {
		return send_err(IO_ERROR, "Can not read from device");
	}
	return PProcRes(false, sizeof(resp->header) + rdReq->size);
}

HwioServer::PProcRes HwioServer::handle_write(ClientInfo * client,
		Hwio_packet_header header) {
	stringstream ss;

	if (header.body_len < sizeof(WrReq))
		return send_err(MALFORMED_PACKET, "WRITE: size too small");

	const WrReq* wrReq = reinterpret_cast<WrReq*>(rx_buffer);
	if (wrReq->_.devId >= MAX_DEVICES)
		return send_err(MALFORMED_PACKET, "WRITE: wrong device id");

#ifdef LOG_INFO
	LOG_INFO << "WRITE: client:" << client->id << ", dev:"
	<< (int) wrReq->_.devId << " 0x" << hex << wrReq->_.addr << " "
	<< dec << wrReq->data << endl;
#endif
	auto dev = client_get_dev(client, wrReq->_.devId);
	if (!dev) {
		return send_err(ACCESS_DENIED, "WRITE: device is not allocated");
	}

	dev->write(wrReq->_.addr, wrReq->data, wrReq->_.size);

	return PProcRes(false, 0);
}
