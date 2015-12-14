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

struct PACKED RdResp {
	char data[0];
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

// Command codes used by hwio server
enum HWIO_CMD {
	HWIO_CMD_READ = 1,  // read from device
	// 1B cmd, 1B device ID (has to be associated with this client)
	// 4B addr
	HWIO_CMD_READ_RESP = 2,
	// 1B cmd
	// 4B data
	HWIO_CMD_WRITE = 3,	// write to device
	// 1B cmd, 1B device ID (has to be associated with this client)
	// 4B addr, 4B data
	HWIO_CMD_PING_REQUEST = 4,
	// 1B cmd
	HWIO_CMD_PING_REPLY = 5,
	// 1B cmd
	HWIO_CMD_QUERY = 6,	// query all devices available on server with compatibility list
	// 1B cmd, 1B cnt
	// 128B string for vendor, 128B string for type, 3x4B for version
	HWIO_CMD_QUERY_RESP = 7,	// response with device ids
	// 1B cmd, 1B number of devices, nB device ID
	//  number of devices can be 0
	HWIO_CMD_BYE = 8,	// client request to terminate connection
	// 1B cmd
	HWIO_CMD_MSG = 9
// 1B cmd
// 4B signed code
// 1024B of err msg string
};

// error codes for messages used by hwio server
enum hwio_server_error {
	UNKNOWN_COMMAND = 1,
	MALFORMED_PACKET = 2,
	DEV_CNT_EXCEEDED = 3,
	ACCESS_DENIGHT = 4,
	IO_ERROR = 5,
};

}
