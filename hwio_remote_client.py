#!/usr/bin/env python3

import socket
from enum import Enum
from struct import pack, unpack, error


SERVER_IP = '127.0.0.1'
SERVER_PORT = 8890


class HwioErr(Exception):
    pass


class HWIO_CMD():
    """
    Command codes used by hwio server
    """
    READ = 1  # read from device
    # 1B device ID (has to be associated with this client)
    # 4B addr
    READ_RESP = 2
    # 4B data
    WRITE = 3  # write to device
    # 1B device ID (has to be associated with this client)
    # 4B addr, 4B data
    ECHO_REQUEST = 4
    ECHO_REPLY = 5
    QUERY = 6  # query all devices available on server with compatibility list
    # 128B string for vendor, 128B string for type, 3x4B for version
    QUERY_RESP = 7  # response wit
    # nB device ID
    # number of devices can be 0
    BYE = 8  # client request to terminate connection
    ERROR_MSG = 9
    # 4B err code
    # 1024B of err msg string


class HwioFrame():
    """
    Header for frames used by hwio server
    """
    HEADER_FORMAT = "<BH"
    HEADER_LEN = 3

    def __init__(self, cmd, data):
        self.cmd = cmd
        self.data = data

    def __bytes__(self):
        return pack(self.HEADER_FORMAT, self.cmd, len(self.data)) + self.data

    def __repr__(self):
        return "<HwioFrame cmd:%d, %r>" % (self.cmd, self.data)


class HwioVersion():
    FORMAT = "iii"

    def __init__(self, major, minor, subminor):
        self.major = major
        self.minor = minor
        self.subminor = subminor

    def __bytes__(self):
        return pack("iii", self.major, self.minor, self.subminor)

    def __repr__(self):
        return "<HwioVersion %d.%d.%d>" % (self.major, self.minor, self.subminor)


HWIO_ANY_VERSION = HwioVersion(-1, -1, -1)


class HwioSpec():
    """
    HWIO compatibility string
    """
    STR_WIDTH = 128

    def __init__(self, vendor, _type, version=HwioVersion(-1, -1, -1), name=None):
        assert isinstance(version, HwioVersion)
        if name is None:
            name = ""
        self.name = name
        self.vendor = vendor
        self.type = _type
        self.version = version

    def __bytes__(self):
        name = bytes(self.name)
        name.ljust(self.STR_WIDTH, '\0')

        vendor = bytes(self.vendor)
        vendor.ljust(self.STR_WIDTH, '\0')

        _type = bytes(self.type)
        _type.ljust(self.STR_WIDTH, '\0')

        return b"".join([name, vendor, _type, bytes(self.version)])

    def __repr__(self):
        return "<HwioSpec %s, %s, %r>" % (self.vendor, self.type, self.version)


class HwioErr(Exception):
    pass


class HWIO_device():
    MAX_NAME_LEN = 128
    TIMEOUT = 3000  # [ms]
    ADDR_FORMAT = "I"  # uint32_t
    DEV_ID_FORMAT = "B"
    SIZE_FORMAT = "H"  # short int
    # ADDR_FORMAT = "Q" # uint64_t
    READ_FORMAT = "<" + DEV_ID_FORMAT + ADDR_FORMAT + SIZE_FORMAT
    WRITE_FORMAT = "<" + DEV_ID_FORMAT + ADDR_FORMAT + SIZE_FORMAT
    QUERY_RESP_FORMAT = "<B" + DEV_ID_FORMAT
    NAME_FORMAT = '{}s'.format(MAX_NAME_LEN)
    QUERY_ITEM_FORMAT = "<" + NAME_FORMAT + NAME_FORMAT + HwioVersion.FORMAT

    def __init__(self, connection, compatibilityList, devIndex=0, name=""):
        self._connection = connection
        self.name = name
        if not self.echo():
            raise HwioErr("Server is not responding on echo")

        self._devId = list(self._query(compatibilityList))[devIndex]
        if self._devId is None:
            raise HwioErr("There is no such a device on server")

    def recvFrame(self):
        data = self._connection.recv(HwioFrame.HEADER_LEN)
        try:
            cmd, length = unpack(HwioFrame.HEADER_FORMAT, data)
        except error as e:
            raise HwioErr(e, data)

        if length:
            body = self._connection.recv(length)
        else:
            body = b""

        return HwioFrame(cmd, body)

    def sendFrame(self, cmd, body):
        frame = HwioFrame(cmd, body)
        self._connection.send(bytes(frame))

    def _query(self, compatibilityList):
        """
        Query devices for compatibility list on server
        """
        b = []
        for c in compatibilityList:
            ver = c.version
            _c = pack(self.QUERY_ITEM_FORMAT,
                      c.name.encode(),
                      c.vendor.encode(),
                      c.type.encode(),
                      ver.major,
                      ver.minor,
                      ver.subminor)
            b.append(_c)

        self.sendFrame(HWIO_CMD.QUERY, b"".join(b))

        f = self.recvFrame()
        if f.cmd != HWIO_CMD.QUERY_RESP:
            raise HwioErr("Wrong query response", f)

        foundCnt = len(f.data) // len(self.DEV_ID_FORMAT)
        print("Found %d components" % (foundCnt))
        if not foundCnt:
            raise HwioErr("No such a device on server")

        devIds = unpack(self.DEV_ID_FORMAT * foundCnt, f.data)
        it = iter(devIds)
        for x in it:
            devId = (x, next(it))
            yield devId

    def ping(self):
        self.sendFrame(HWIO_CMD.ECHO_REQUEST, b"")
        rx = self.recvFrame()
        if rx.cmd != HWIO_CMD.ECHO_REPLY or len(rx.data):
            raise HwioErr("Wrong response on ping", rx)
        else:
            return True

    def read8(self, offset):
        return self.read(offset, 1)

    def read16(self, offset):
        return self.read(offset, 2)

    def read32(self, offset):
        return self.read(offset, 4)

    def read64(self, offset):
        return self.read(offset, 8)

    def read(self, offset, size):
        """
        read 4 bytes from device

        :param offset: offset in device where to read from
        """
        req = pack(self.READ_FORMAT,
                   self._devId,
                   offset, size)
        self.sendFrame(HWIO_CMD.READ, req)

        resp = self.recvFrame()
        if resp.cmd != HWIO_CMD.READ_RESP:
            raise HwioErr("Wrong response type", resp.cmd)

        rdata = unpack("I", resp.data)[0]
        return rdata

    def write8(self, offset, data):
        if isinstance(data, int):
            data = pack("B", data)
        else:
            assert len(data) == 1
        return self.write(offset, data)

    def write16(self, offset, data):
        if isinstance(data, int):
            data = pack("H", data)
        else:
            assert len(data) == 1
        return self.write(offset, data)

    def write32(self, offset, data):
        if isinstance(data, int):
            data = pack("I", data)
        else:
            assert len(data) == 1
        return self.write(offset, data)

    def write64(self, offset, data):
        if isinstance(data, int):
            data = pack("Q", data)
        else:
            assert len(data) == 1
        return self.write(offset, data)

    def write(self, offset, data):
        """
        write bytes to device

        :param offset: offset in device where to read from
        :param data: data (bytes) to write
        """
        size = len(data)
        f = pack(self.WRITE_FORMAT,
                 self._devId,
                 offset,
                 size) + data
        self.sendFrame(HWIO_CMD.WRITE, f)

    def bye(self):
        self.sendFrame(HWIO_CMD.BYE, b"")


if __name__ == "__main__":
    compList = [HwioSpec("ug4-150", "axi_tsu", HWIO_ANY_VERSION)]
    # compList = [HwioSpec("xlnx","microblaze", HWIO_ANY_VERSION)]
    # compList = [HwioSpec("uprobe", "axi_dpdk_dma", HwioVersion(-1, -1, -1))]
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_IP, SERVER_PORT))
        s.settimeout(1.0)
        dev = HWIO_device(s, compList)
        for i in range(10):
            dev.write32(0x10, 1)
            dev.write32(0x20, 2)
            print("r:", dev.read32(0x10))
            dev.write32(0x20, 2)
            print("r:", dev.read32(0x30))

        dev.bye()
