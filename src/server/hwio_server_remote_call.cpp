#include "hwio_server.h"

using namespace std;
using namespace hwio;

HwioServer::PProcRes HwioServer::handle_remote_call(ClientInfo * client,
		Hwio_packet_header header) {

	if (header.body_len < sizeof(RemoteCall) - 1)
		return send_err(MALFORMED_PACKET, "READ: size too small");

	auto rc = reinterpret_cast<const RemoteCall*>(rx_buffer);
	ihwio_dev * dev = client_get_dev(client, rc->dev_id);
	if (!dev) {
		return send_err(ACCESS_DENIED, "READ: device is not allocated");
	}
	std::string fn_name((char *)rc->fn_name);
	auto _plugin = plugins.find(fn_name);
	if (_plugin == plugins.end())
		return send_err(MALFORMED_PACKET,
				std::string("REMOTE CALL: plugin ") + fn_name
						+ " not registered on server");
	auto plugin = _plugin->second;

#ifdef LOG_INFO
	LOG_INFO << "REMOTE CALL:" << (int) rc->dev_id << rc->fn_name << endl;
#endif

	auto resp = reinterpret_cast<HwioFrame<RemoteCallRet>*>(tx_buffer);
	resp->header.command = HWIO_CMD_REMOTE_CALL_RET;
	resp->header.body_len = plugin.ret_size;
	try {
		plugin.fn(dev, (void*)rc->args, (void*)resp->body.ret);
	} catch (std::runtime_error & err) {
		return send_err(IO_ERROR,
				std::string("Call of plugin function raised and exception ")
						+ err.what());
	}
	return PProcRes(false, sizeof(resp->header) + plugin.ret_size);
}
