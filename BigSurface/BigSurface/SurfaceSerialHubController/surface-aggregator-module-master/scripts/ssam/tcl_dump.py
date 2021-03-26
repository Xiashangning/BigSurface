#!/usr/bin/env python
import struct
import sys
import time

import libssam
from libssam import Controller, Request


def pack_block_data(id, offset, size, end):
    return struct.pack('<HIHB', id, offset, size, end)


def unpack_block_data(buf):
    return struct.unpack('<HIHB', buf)


def unpack_buffer(buf):
    return unpack_block_data(buf[:9]), buf[9:]


def build_command(iid, buf_id, offset, size):
    return Request(
        target_category=0x0c,
        target_id=1,
        command_id=0x0c,
        instance_id=iid,
        flags=libssam.REQUEST_HAS_RESPONSE,
        payload=pack_block_data(buf_id, offset, size, 0))


def main():
    iid = int(sys.argv[1], 0)
    buf_id = int(sys.argv[2], 0)

    with Controller() as ctrl:
        offset = 0
        readlen = 0x20
        buffer = bytes()
        while True:
            try:
                print(f"reading {offset}:{readlen}")
                data = ctrl.request(build_command(iid, buf_id, offset, readlen))
            except TimeoutError:
                continue

            (_, _, nread, end), bufdata = unpack_buffer(data)
            print(f"read {offset}:{nread}")
            buffer += bufdata
            offset += nread

            if end != 0:
                break

    out = open(f"tclbuf_iid{iid}_bufid{buf_id}.bin", "wb")
    out.write(buffer)


if __name__ == '__main__':
    main()
