#!/usr/bin/env python3
import json
import sys
import struct

import libssam
from libssam import Controller, Request


def dump_raw_data(data):
    print(json.dumps([x for x in data]))


class SurfaceLegacyKeyboard:
    def __init__(self, ctrl):
        self.ctrl = ctrl

    def get_device_descriptor(self, entry):
        flags = libssam.REQUEST_HAS_RESPONSE
        rqst = Request(0x08, 0x02, 0x00, 0x00, flags, bytes([entry]))
        return self.ctrl.request(rqst)

    def get_hid_feature_report(self, num):
        flags = libssam.REQUEST_HAS_RESPONSE
        rqst = Request(0x08, 0x02, 0x0b, 0x00, flags, bytes([num]))
        return self.ctrl.request(rqst)

    def set_capslock_led(self, state):
        state_byte = 0x01 if state else 0x00
        rqst = Request(0x08, 0x02, 0x01, 0x00, 0, bytes([state_byte]))
        self.ctrl.request(rqst)


class SurfaceHidDevice:
    def __init__(self, ctrl, instance):
        self.ctrl = ctrl
        self.target_categorty = 0x15
        self.target_id = 0x02
        self.instance_id = instance

    def get_device_descriptor(self, entry):
        n = 0x76
        done = False
        data = bytearray()

        while not done:
            done, part = self._get_device_descriptor_part(entry, len(data), n)
            data += part

        return data

    def _get_device_descriptor_part(self, entry, offset, length):
        payload = struct.pack("<BIIB", entry, offset, length, 0)

        rqst = Request(self.target_categorty, self.target_id, 0x04,
                       self.instance_id, libssam.REQUEST_HAS_RESPONSE, payload)
        resp = self.ctrl.request(rqst)

        head, data = resp[:10], resp[10:]
        _, _, _, done = struct.unpack("<BIIB", head)

        return done, data


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
        print(f'  legacy-get-descriptor <entry>')
        print(f'    get the HID device descriptor identified by <entry>')
        print(f'')
        print(f'    <entry>:  0  HID descriptor')
        print(f'              1  HID report descriptor')
        print(f'              2  device attributes')
        print(f'')
        print(f'  legacy-get-feature-report <num>')
        print(f'    get the HID feature report identified by <num>')
        print(f'')
        print(f'  legacy-set-capslock-led <state>')
        print(f'    set the capslock led state')
        print(f'')
        print(f'  hid-get-descriptor <iid> <entry>')
        print(f'    get the HID device descriptor identified by <entry>')
        print(f'    for device instance with ID <iid>')
        print(f'')
        print(f'    <entry>:  0  HID descriptor')
        print(f'              1  HID report descriptor')
        print(f'              2  device attributes')

    elif cmd_name == 'legacy-get-descriptor':
        entry = int(sys.argv[2], 0)

        with Controller() as ctrl:
            dev = SurfaceLegacyKeyboard(ctrl)
            dump_raw_data(dev.get_device_descriptor(entry))

    elif cmd_name == 'legacy-get-feature-report':
        num = int(sys.argv[2], 0)

        with Controller() as ctrl:
            dev = SurfaceLegacyKeyboard(ctrl)
            dump_raw_data(dev.get_hid_feature_report(num))

    elif cmd_name == 'legacy-set-capslock-led':
        state = int(sys.argv[2], 0)

        with Controller() as ctrl:
            dev = SurfaceLegacyKeyboard(ctrl)
            dev.set_capslock_led(state)

    elif cmd_name == 'hid-get-descriptor':
        iid = int(sys.argv[2], 0)
        entry = int(sys.argv[3], 0)

        with Controller() as ctrl:
            dev = SurfaceHidDevice(ctrl, iid)
            dump_raw_data(dev.get_device_descriptor(entry))

    else:
        print(f'Invalid command: \'{cmd_name}\', try \'{sys.argv[0]} help\'')
        sys.exit(1)


if __name__ == '__main__':
    main()
