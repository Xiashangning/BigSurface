#!/usr/bin/env python3
import sys

import libssam
from libssam import Controller, Request, REQUEST_HAS_RESPONSE


COMMANDS = {
    "lock":       Request(0x11, 0x01, 0x06, 0x00),
    "unlock":     Request(0x11, 0x01, 0x07, 0x00),
    "request":    Request(0x11, 0x01, 0x08, 0x00),
    "confirm":    Request(0x11, 0x01, 0x09, 0x00),
    "heartbeat":  Request(0x11, 0x01, 0x0a, 0x00),
    "cancel":     Request(0x11, 0x01, 0x0b, 0x00),
    "get-state":  Request(0x11, 0x01, 0x0c, 0x00, REQUEST_HAS_RESPONSE),
    "get-opmode": Request(0x11, 0x01, 0x0d, 0x00, REQUEST_HAS_RESPONSE),
    "get-status": Request(0x11, 0x01, 0x11, 0x00, REQUEST_HAS_RESPONSE),
}


def print_usage_and_exit():
    print(f"Usage:")
    print(f"  {sys.argv[0]} help")
    print(f"  {sys.argv[0]} <command>")
    exit(1)


def print_help_and_exit():
    print(f"Usage:")
    print(f"  {sys.argv[0]} help")
    print(f"  {sys.argv[0]} <command>")
    print(f"")
    print(f"Commands:")

    for cmd in COMMANDS.keys():
        print(f"  {cmd}")

    exit(1)


def main():
    if len(sys.argv) != 2:
        print_usage_and_exit()

    cmd = sys.argv[1]

    if cmd == "help":
        print_help_and_exit()

    rqst = COMMANDS.get(cmd)

    if rqst is None:
        print(f"Invalid command: '{sys.argv[1]}'")
        print(f"")
        print_usage_and_exit()

    with Controller() as ctrl:
        rsp = ctrl.request(rqst)
        if rsp:
            print(' '.join(['{:02x}'.format(x) for x in rsp]))


if __name__ == '__main__':
    main()
