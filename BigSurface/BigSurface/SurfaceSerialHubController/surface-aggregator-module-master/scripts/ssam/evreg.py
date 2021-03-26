#!/usr/bin/env python3
import sys
import os

import libssam
from libssam import Controller, Request


EVCMDS = {
    'sam-enable':  {'tid': 1, 'tc': 0x01, 'cid': 0x0b, 'iid': 0x00, 'snc': 0x01},
    'sam-disable': {'tid': 1, 'tc': 0x01, 'cid': 0x0c, 'iid': 0x00, 'snc': 0x01},
    'kip-enable':  {'tid': 2, 'tc': 0x0e, 'cid': 0x27, 'iid': 0x00, 'snc': 0x01},
    'kip-disable': {'tid': 2, 'tc': 0x0e, 'cid': 0x28, 'iid': 0x00, 'snc': 0x01},
    'reg-enable':  {'tid': 2, 'tc': 0x21, 'cid': 0x01, 'iid': 0x00, 'snc': 0x01},
    'reg-disable': {'tid': 2, 'tc': 0x21, 'cid': 0x02, 'iid': 0x00, 'snc': 0x01},
}


def lo16(rqid):
    return rqid & 0xff


def hi16(rqid):
    return (rqid >> 8) & 0xff


def event_payload(rqid, tc, iid, seq):
    return bytes([tc, seq, lo16(rqid), hi16(rqid), iid])


def command(tc, tid, cid, iid, snc, payload):
    return Request(tc, tid, cid, iid, snc, payload, 8)


def event_command(name, ev_tc, ev_seq, ev_iid):
    payload = event_payload(ev_tc, ev_tc, ev_iid, ev_seq)
    return command(**EVCMDS[name], payload=payload)


def run_command(rqst):
    with Controller() as ctrl:
        data = ctrl.request(rqst)
        if data:
            print(' '.join(['{:02x}'.format(x) for x in data]))


def main():
    cmd_name = sys.argv[1]

    if cmd_name == 'help':
        print(f'Usage:')
        print(f'  {sys.argv[0]} <command> [args...]')
        print(f'')
        print(f'Commands:')
        print(f'  help')
        print(f'    display this help message')
        print(f'')

        for cmd in reversed(sorted(list(EVCMDS.keys()))):
            evsys, ty = cmd.split('-', 1)
            print(f'  {cmd} <ev_tc> <ev_seq> <ev_iid>')
            print(f'    {ty} event using {evsys.upper()} subsystem')
            print(f'')

        print(f'Arguments:')
        print(f'  <ev_tc>:       event target category')
        print(f'  <ev_seq>:      event-is-sequenced flag')
        print(f'  <ev_iid>:      event instance ID')

    else:
        ev_tc = int(sys.argv[2], 0)
        ev_seq = int(sys.argv[3], 0)
        ev_iid = int(sys.argv[4], 0)

        run_command(event_command(cmd_name, ev_tc, ev_seq, ev_iid))


if __name__ == '__main__':
    main()
