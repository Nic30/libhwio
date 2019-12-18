#pragma once

#include "hwio.h"

#include <iostream>

namespace hwio {

#define PACKED __attribute__((__packed__))
//#define LOG_INFO (std::cout << "[INFO] ")
#define LOG_ERR (std::cout << "[ERROR] ")

// these limit are not restriction of implementation
static const int MAX_DEVICES = 256;
static const int BUFFER_SIZE = 2048;
static const int MAX_NAME_LEN = 128;
// they are there for detection of suspicious accesses
static const int MAX_ITEMS_PER_QUERY = 5;
static const int MAX_ITEMS_PER_QUERY_RESP = 5;
static const int MAX_ERR_MSG_LEN = 1024;
static const int MAX_CLIENTS = 32;
static const int MAX_DATA_LEN = 1400;

// can not use Linux phys_addr_t because hwio supports only 32bit
typedef uint32_t physAddr_t;
typedef char data_t[MAX_DATA_LEN];
typedef uint16_t dev_id_t;

struct PACKED Hwio_packet_header {
	uint8_t command;
	uint16_t body_len;
};

template<typename bodyT>
struct PACKED HwioFrame {
	Hwio_packet_header header;
	bodyT body;
};

// messages
struct PACKED RdReq {
	dev_id_t devId;
	physAddr_t addr;
	uint16_t size;
};

struct PACKED RemoteCall {
	dev_id_t dev_id;
	uint8_t fn_name[MAX_NAME_LEN];
	char args[0]; // arbitrary data size
};

struct PACKED RemoteCallFast {
	dev_id_t dev_id;
	uint32_t fn_id;
	char args[0]; // arbitrary data size
};

struct PACKED GetRemoteCallId {
	dev_id_t dev_id;
	uint8_t fn_name[MAX_NAME_LEN];
};

struct PACKED GetRemoteCallIdResp {
	bool found;
	uint32_t fn_id;
};

struct PACKED RemoteCallRet {
	uint8_t ret[0]; // arbitrary data size
};

struct PACKED RdResp {
	char data[0]; // arbitrary data size
};

struct PACKED WrReq {
	RdReq _;
	char data[0];
};

struct PACKED Dev_query_item {
	char device_name[MAX_NAME_LEN];
	char vendor_name[MAX_NAME_LEN];
	char type_name[MAX_NAME_LEN];
	hwio_version version;
};

struct PACKED DevQuery {
	Dev_query_item items[MAX_ITEMS_PER_QUERY];
};

struct PACKED ErrMsg {
	int err_code;
	char msg[MAX_ERR_MSG_LEN];
};

struct PACKED DevQueryResp {
	dev_id_t ids[MAX_ITEMS_PER_QUERY_RESP];
};

struct PACKED RdReqMulti {
	dev_id_t devId;
	physAddr_t addr;
	uint16_t size;
};

// Command codes used by hwio server
enum HWIO_CMD {
	HWIO_CMD_READ = 1,  // read from device
	// HwioFrame<RdReq>
	HWIO_CMD_READ_RESP = 2, // response with read data
	// HwioFrame<RdResp>
	HWIO_CMD_WRITE = 3,	// write to device
	// HwioFrame<WrReq>
	HWIO_CMD_PING_REQUEST = 4,
	// 1B cmd
	HWIO_CMD_PING_REPLY = 5,
	// 1B cmd
	HWIO_CMD_QUERY = 6,	// query all devices available on server with compatibility list
	// HwioFrame<DevQuery>
	HWIO_CMD_QUERY_RESP = 7,	// response with device ids
	// HwioFrame<DevQueryResp>
	//  number of devices can be 0
	HWIO_CMD_BYE = 8,	// client request to terminate connection
	// 1B cmd
	HWIO_CMD_MSG = 9,
	// 1B cmd
	// 4B signed code
	// 1024B of err msg string
	HWIO_CMD_REMOTE_CALL = 10,
	// HwioFrame<RemoteCall>
	HWIO_CMD_REMOTE_CALL_RET = 11,
	// HwioFrame<RemoteCallRet>
        HWIO_CMD_READ_MULTIPLE = 12,
        // HwioFrame<RdReqMulti>
        HWIO_CMD_READ_MULTIPLE_RESP = 13,
        // HwioFrame<RdMultiResp>
        HWIO_CMD_WRITE_MULTIPLE = 14,
        // HwioFrame<WrReqMulti>
        HWIO_CMD_WRITE_KEYHOLE = 15,
        // HwioFrame<WrReqRange>
        HWIO_CMD_REMOTE_CALL_FAST = 16,
        // HwioFrame<RemoteCallFast>
        HWIO_CMD_GET_REMOTE_CALL_ID = 17,
        // HwioFrame<GetRemoteCallId>
        HWIO_CMD_GET_REMOTE_CALL_ID_RESP = 17,
        // HwioFrame<GetRemoteCallIdResp>
};

// error codes for messages used by hwio server
enum HWIO_SERVER_ERROR {
	UNKNOWN_COMMAND = 1,
	MALFORMED_PACKET = 2,
	DEV_CNT_EXCEEDED = 3,
	ACCESS_DENIED = 4,
	IO_ERROR = 5,
};

}
