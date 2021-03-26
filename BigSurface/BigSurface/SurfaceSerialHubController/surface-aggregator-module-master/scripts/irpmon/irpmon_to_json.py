#!/usr/bin/env python3
from __future__ import print_function
import sys
import codecs
import json


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


class FrameCtrl:
    def __init__(self, type, len, pad, seq):
        self.type = type
        self.len = len
        self.pad = pad
        self.seq = seq

    def __str__(self):
        return f"type: {hex(self.type)}, len: {hex(self.len)}, pad: {hex(self.pad)}, " + \
               f"seq: {hex(self.seq)}"

    def to_dict(self):
        return {
            "type": self.type,
            "len": self.len,
            "pad": self.pad,
            "seq": self.seq,
        }


class FrameCmd:
    def __init__(self, type, tc, out, inc, iid, rqid_lo, rqid_hi, cid):
        self.type = type
        self.tc = tc
        self.outgoing = out
        self.incoming = inc
        self.iid = iid
        self.rqid_lo = rqid_lo
        self.rqid_hi = rqid_hi
        self.cid = cid

    def __str__(self):
        return f"type: {hex(self.type)}, tc: {hex(self.tc)}, out: {hex(self.outgoing)}, " + \
               f"in: {hex(self.incoming)}, iid: {hex(self.iid)}, rqid_lo: {hex(self.rqid_lo)}, " + \
               f"rqid_hi: {hex(self.rqid_hi)}, cid: {hex(self.cid)}"

    def to_dict(self):
        return {
            "type": self.type,
            "tc": self.tc,
            "outgoing": self.outgoing,
            "incoming": self.incoming,
            "iid": self.iid,
            "rqid_lo": self.rqid_lo,
            "rqid_hi": self.rqid_hi,
            "cid": self.cid,
        }


def parse_file(file):
    function = 'NONE'
    data = False
    lines = []
    cmddata = []

    for line in file:
        if line.startswith("Major function ="):
            function = line.split('=', 1)[1].strip()
        elif line.startswith("Data (Hexer)"):
            data = True
        elif data and line.startswith("  "):
            lines.append(line.strip())
        elif data:
            if function == 'Read' or function == 'Write':
                for l in lines:
                    strdata = l.split("\t")[1]
                    bytedata = [int(x, 16) for x in strdata.split()]
                    cmddata += bytedata

            data = False
            lines = []

    return bytes(cmddata)


def parse_syn(data):
    if data[0:2] != bytes([0xaa, 0x55]):
        eprint("warning: expected SYN, skipping data until next SYN")

        for i in range(1, len(data)):
            if data[i] == 0xaa and data[i+1] == 0x55:
                return data[i+2:]

    return data[2:]


def parse_ter(data):
    if data[0:2] != bytes([0xff, 0xff]):
        print("error: expected TER")
        print(data)
        exit(1)

    return data[2:]


def skip_checksum(data):
    return data[2:]


def parse_frame_ctrl(data):
    ty = data[0]
    len = data[1]
    pad = data[2]
    seq = data[3]

    return data[4:], FrameCtrl(ty, len, pad, seq)


def parse_frame_cmd(data):
    ty = data[0]
    tc = data[1]
    out = data[2]
    inc = data[3]
    iid = data[4]
    rqid_lo = data[5]
    rqid_hi = data[6]
    cid = data[7]

    return data[8:], FrameCmd(ty, tc, out, inc, iid, rqid_lo, rqid_hi, cid)


def parse_commands(data):
    records = []

    while data:
        data = parse_syn(data)
        data, ctrl = parse_frame_ctrl(data)
        data = skip_checksum(data)

        if ctrl.type == 0x00 or ctrl.type == 0x80:
            data, cmd = parse_frame_cmd(data)

            payload_len = ctrl.len - 8
            data, pld = data[payload_len:], data[0:payload_len]

            data = skip_checksum(data)

            record = {"ctrl": ctrl.to_dict(), "cmd": cmd.to_dict(), "payload": list(pld)}
            records.append(record)

        elif ctrl.type == 0x40:
            data = parse_ter(data)
            records.append({"ctrl": ctrl.to_dict()})
        elif ctrl.type == 0x04:
            data = parse_ter(data)
            records.append({"ctrl": ctrl.to_dict()})

    return records


def main(in_file):
    with codecs.open(in_file, 'r', encoding='utf-8', errors='ignore') as fd:
        data = parse_file(fd)

    print(json.dumps(parse_commands(data)))


if __name__ == '__main__':
    main(sys.argv[1])
