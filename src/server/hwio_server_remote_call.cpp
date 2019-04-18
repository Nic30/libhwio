#include "hwio_server.h"

using namespace std;
using namespace hwio;

HwioServer::PProcRes HwioServer::handle_remote_call(ClientInfo * client,
		Hwio_packet_header header) {

	if (header.body_len < sizeof(RemoteCall) - 1)
		return send_err(MALFORMED_PACKET, "REMOTE CALL: size too small");

	auto rc = reinterpret_cast<const RemoteCall*>(rx_buffer);
	ihwio_dev * dev = client_get_dev(client, rc->dev_id);
	if (!dev) {
		return send_err(ACCESS_DENIED, "REMOTE CALL: device is not allocated");
	}
	std::string fn_name((char *) rc->fn_name);
	auto _plugin = plugins.find(fn_name);
	if (_plugin == plugins.end())
		return send_err(MALFORMED_PACKET,
				std::string("REMOTE CALL: plugin ") + fn_name
						+ " not registered on server");
	auto plugin = _plugin->second;

	if (log_level >= logDEBUG) {
		std::cout << "[DEBUG] REMOTE CALL:" << (int) rc->dev_id << rc->fn_name
				<< endl;
	}

	auto resp = reinterpret_cast<HwioFrame<RemoteCallRet>*>(tx_buffer);
	resp->header.command = HWIO_CMD_REMOTE_CALL_RET;
	resp->header.body_len = plugin.ret_size;
	try {
		plugin.fn(dev, (void*) rc->args, (void*) resp->body.ret);
	} catch (std::runtime_error & err) {
		return send_err(IO_ERROR,
				std::string("Call of plugin function raised and exception ")
						+ err.what());
	}
	if (plugin.ret_size != 0) {
            return PProcRes(false, sizeof(resp->header) + plugin.ret_size);
        } else {
            return PProcRes(false, 0);
        }
}

HwioServer::PProcRes HwioServer::handle_fast_remote_call(ClientInfo * client,
		Hwio_packet_header header) {

	if (header.body_len < sizeof(RemoteCallFast) - 1)
		return send_err(MALFORMED_PACKET, "REMOTE CALL: size too small");

	auto rc = reinterpret_cast<const RemoteCallFast*>(rx_buffer);
	ihwio_dev * dev = client_get_dev(client, rc->dev_id);
	if (!dev) {
		return send_err(ACCESS_DENIED, "REMOTE CALL: device is not allocated");
	}
	if(rc->fn_id >= plugins_fast.size()) {
            return send_err(MALFORMED_PACKET,
				std::string("REMOTE CALL: plugin with id ") + fn_name
						+ " is not registered on server");
	}
	auto plugin = plugins_fast[rc->fn_id];

	if (log_level >= logDEBUG) {
		std::cout << "[DEBUG] REMOTE CALL:" << (int) rc->dev_id << rc->fn_name
				<< endl;
	}

	auto resp = reinterpret_cast<HwioFrame<RemoteCallRet>*>(tx_buffer);
	resp->header.command = HWIO_CMD_REMOTE_CALL_RET;
	resp->header.body_len = plugin.ret_size;
	try {
		plugin.fn(dev, (void*) rc->args, (void*) resp->body.ret);
	} catch (std::runtime_error & err) {
		return send_err(IO_ERROR,
				std::string("Call of plugin function raised and exception ")
						+ err.what());
	}
	if (plugin.ret_size != 0) {
            return PProcRes(false, sizeof(resp->header) + plugin.ret_size);
        } else {
            return PProcRes(false, 0);
        }
}

HwioServer::PProcRes HwioServer::handle_get_rpc_fn_id(ClientInfo * client, Hwio_packet_header header) {
	if (header.body_len < sizeof(GetRemoteCallId))
		return send_err(MALFORMED_PACKET, "READ: size too small");

	auto rc = reinterpret_cast<const GetRemoteCallId*>(rx_buffer);
	auto resp = reinterpret_cast<HwioFrame<RemoteCallRet>*>(tx_buffer);
	resp->header.command = HWIO_CMD_GET_REMOTE_CALL_ID_RESP;
	resp->header.body_len = sizeof(RemoteCallRet);
	resp->body.found = false;
	resp->body.fn_id = -1;
	ihwio_dev * dev = client_get_dev(client, rc->dev_id);
	if (!dev) {
		return send_err(ACCESS_DENIED, "REMOTE CALL: device is not allocated");
	}
	std::string fn_name((char *) rc->fn_name);
	auto _plugin = plugins.find(fn_name);
	if (_plugin != plugins.end()) {
		auto plugin = _plugin->second;
		for (int32_t i = 0; i < plugins_fast.size(); i++) {
			if (plugin == plugins_fast[i]) {
				resp->body.found = true;
				resp->body.fn_id = i;
				break;
			}
		}
	}
	return PProcRes(false, sizeof(HwioFrame<RemoteCallRet>));
}
