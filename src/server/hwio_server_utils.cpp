#include "hwio_server.h"

using namespace std;
using namespace hwio;

HwioServer::PProcRes HwioServer::send_err(int err_code, const string & msg) {
	HwioFrame<ErrMsg>* m = reinterpret_cast<HwioFrame<ErrMsg>*>(tx_buffer);
	strncpy(m->body.msg, msg.c_str(), MAX_ERR_MSG_LEN);
	m->body.err_code = err_code;

	m->header.command = HWIO_CMD_MSG;
	m->header.body_len = sizeof(m->body.err_code) + strlen(m->body.msg);
#ifdef LOG_INFO
	LOG_ERR << "Error to client: " << msg << endl;
#endif
	// [TODO] use strlen to resolve real len
	return PProcRes(true, sizeof(ErrMsg));
}


ihwio_dev * HwioServer::client_get_dev(ClientInfo * client, dev_id_t devId) {
	if (devId < client->devices.size())
		return client->devices.at(devId);
	return nullptr;
}
