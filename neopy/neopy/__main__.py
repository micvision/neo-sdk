#!/usr/bin/env python
# coding=utf-8

import itertools
import sys
import neopy

def main():
    ### USAGE: python -m neopy /dev/ttyACM0
    if len(sys.argv) < 2:
        sys.exit('python -m neopy /dev/ttyACM0')

    dev = sys.argv[1]
    ### Named a device as neo_device
    with neopy.neo(dev) as neo_device:
        ### Set motor speed: 5
        neo_device.set_motor_speed(5)
        ### Get temp motor speed
        # speed = neo_device.get_motor_speed()

        ### Print out speed
        # print('Motor Speed: {} Hz'.format(speed))
        ### Device start scanning, and get data
        neo_device.start_scanning()

        ### Print out the scan data
        for scan in itertools.islice(neo_device.get_scans(), 10):
            print(scan)

        ### Device stop scanning
        neo_device.stop_scanning()
        ### Device set motor speed 0 to stop the motor
        neo_device.set_motor_speed(0)

main()
