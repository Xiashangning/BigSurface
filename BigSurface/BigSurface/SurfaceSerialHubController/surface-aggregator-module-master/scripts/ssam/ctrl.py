#!/usr/bin/env python3
import os
import sys
import struct

import libssam
from libssam import Controller, Request


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
        print(f'  request <tc> <tid> <cid> <iid> <flags> [payload...]')
        print(f'    basic command with optional payload')
        print(f'')
        print(f'Arguments:')
        print(f'  <tc>:          command target category')
        print(f'  <tid>:         command target ID')
        print(f'  <cid>:         command ID')
        print(f'  <iid>:         command instance ID')
        print(f'  <flags>:       request flags')
        print(f'  [payload...]:  optional payload bytes, separated by whitespace')

    elif cmd_name == 'request':
        tc = int(sys.argv[2], 0)
        tid = int(sys.argv[3], 0)
        cid = int(sys.argv[4], 0)
        iid = int(sys.argv[5], 0)
        flg = int(sys.argv[6], 0)
        pld = [int(x, 0) for x in sys.argv[7:]]

        rqst = Request(tc, tid, cid, iid, flg, pld)

        with Controller() as ctrl:
            rsp = ctrl.request(rqst)
            if rsp:
                print(' '.join(['{:02x}'.format(x) for x in rsp]))


if __name__ == '__main__':
    main()
