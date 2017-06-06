#!/usr/bin/env python
# coding=utf-8

import ctypes
import ctypes.util
import collections

libneo = ctypes.cdll.LoadLibrary(ctypes.util.find_library('neo'))

libneo.neo_get_version.restype = ctypes.c_int32
libneo.neo_get_version.argtypes = None

libneo.neo_is_abi_compatible.restype = ctypes.c_bool
libneo.neo_is_abi_compatible.argtypes = None

libneo.neo_error_message.restype = ctypes.c_char_p
libneo.neo_error_message.argtypes = [ctypes.c_void_p]

libneo.neo_error_destruct.restype = None
libneo.neo_error_destruct.argtypes = [ctypes.c_void_p]

libneo.neo_device_construct_simple.restype = ctypes.c_void_p
libneo.neo_device_construct_simple.argtypes = [ctypes.c_char_p, ctypes.c_void_p]

libneo.neo_device_construct.restype = ctypes.c_void_p
libneo.neo_device_construct.argtypes = [ctypes.c_char_p, ctypes.c_int32, ctypes.c_void_p]

libneo.neo_device_destruct.restype = None
libneo.neo_device_destruct.argtypes = [ctypes.c_void_p]

libneo.neo_device_start_scanning.restype = None
libneo.neo_device_start_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libneo.neo_device_stop_scanning.restype = None
libneo.neo_device_stop_scanning.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libneo.neo_device_get_scan.restype = ctypes.c_void_p
libneo.neo_device_get_scan.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libneo.neo_scan_destruct.restype = None
libneo.neo_scan_destruct.argtypes = [ctypes.c_void_p]

libneo.neo_scan_get_number_of_samples.restype = ctypes.c_int32
libneo.neo_scan_get_number_of_samples.argtypes = [ctypes.c_void_p]

libneo.neo_scan_get_angle.restype = ctypes.c_int32
libneo.neo_scan_get_angle.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libneo.neo_scan_get_distance.restype = ctypes.c_int32
libneo.neo_scan_get_distance.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libneo.neo_scan_get_signal_strength.restype = ctypes.c_int32
libneo.neo_scan_get_signal_strength.argtypes = [ctypes.c_void_p, ctypes.c_int32]

libneo.neo_device_get_motor_speed.restype = ctypes.c_int32
libneo.neo_device_get_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libneo.neo_device_set_motor_speed.restype = None
libneo.neo_device_set_motor_speed.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libneo.neo_device_get_sample_rate.restype = ctypes.c_int32
libneo.neo_device_get_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

libneo.neo_device_set_sample_rate.restype = None
libneo.neo_device_set_sample_rate.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p]

libneo.neo_device_reset.restype = None
libneo.neo_device_reset.argtypes = [ctypes.c_void_p, ctypes.c_void_p]


def _error_to_exception(error):
    assert error
    what = libneo.neo_error_message(error)
    libneo.neo_error_destruct(error)
    return RuntimeError(what.decode('ascii'))


class Scan(collections.namedtuple('Scan', 'samples')):
    pass


class Sample(collections.namedtuple('Sample', 'angle distance signal_strength')):
    pass


class neo:
    ### Construct of neo class
    def __init__(self, port, bitrate = None):
        self.scoped = False
        self.args = [port, bitrate]
        self.scoped = True
        self.device = None

        assert libneo.neo_is_abi_compatible(), 'Your installed libneo is not ABI compatible with these bindings'

        error = ctypes.c_void_p();

        simple = not self.args[1]
        config = all(self.args)

        assert simple or config, 'No arguments for bitrate, required'

        if simple:
            port = ctypes.string_at(self.args[0].encode('ascii'))
            device = libneo.neo_device_construct_simple(port, ctypes.byref(error))

        if config:
            port = ctypes.string_at(self.args[0].encode('ascii'))
            bitrate = ctypes.c_int32(self.args[1])
            device = libneo.neo_device_construct(port, bitrate, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        self.device = device

    ### Build for `with` function
    def __enter__(self):
        self.scoped = True
        self.device = None

        assert libneo.neo_is_abi_compatible(), 'Your installed libneo is not ABI compatible with these bindings'

        error = ctypes.c_void_p();

        simple = not self.args[1]
        config = all(self.args)

        assert simple or config, 'No arguments for bitrate, required'

        if simple:
            port = ctypes.string_at(self.args[0].encode('ascii'))
            device = libneo.neo_device_construct_simple(port, ctypes.byref(error))

        if config:
            port = ctypes.string_at(self.args[0].encode('ascii'))
            bitrate = ctypes.c_int32(self.args[1])
            device = libneo.neo_device_construct(port, bitrate, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        self.device = device

        return self

    ### Destruct of neo class
    def __exit__(self, *args):
        self.scoped = False

        if self.device:
            libneo.neo_device_destruct(self.device)

    def _assert_scoped(self):
        assert self.scoped, 'Use the `with` statement to guarantee for deterministic resource management'

    ### Start scanning api, using C language neo_device_start_scanning function
    def start_scanning(self):
        self._assert_scoped()

        error = ctypes.c_void_p();
        libneo.neo_device_start_scanning(self.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    ### Stop scanning
    def stop_scanning(self):
        self._assert_scoped();

        error = ctypes.c_void_p();
        libneo.neo_device_stop_scanning(self.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    ### Get motor speed
    def get_motor_speed(self):
        self._assert_scoped()

        error = ctypes.c_void_p()
        speed = libneo.neo_device_get_motor_speed(self.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        return speed

    ### Set motor speed
    def set_motor_speed(self, speed):
        self._assert_scoped()

        error = ctypes.c_void_p()
        libneo.neo_device_set_motor_speed(self.device, speed, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    ### Get sample rate
    def get_sample_rate(self):
        self._assert_scoped()

        error = ctypes.c_void_p()
        speed = libneo.neo_device_get_sample_rate(self.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

        return speed

    ### Set sample rate
    def set_sample_rate(self, speed):
        self._assert_scoped()

        error = ctypes.c_void_p()
        libneo.neo_device_set_sample_rate(self.device, speed, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)

    ### Get scan data
    def get_scans(self):
        self._assert_scoped()

        error = ctypes.c_void_p()

        while True:
            scan = libneo.neo_device_get_scan(self.device, ctypes.byref(error))

            if error:
                raise _error_to_exception(error)

            num_samples = libneo.neo_scan_get_number_of_samples(scan)

            samples = [Sample(angle=libneo.neo_scan_get_angle(scan, n),
                              distance=libneo.neo_scan_get_distance(scan, n),
                              signal_strength=libneo.neo_scan_get_signal_strength(scan, n))
                       for n in range(num_samples)]

            libneo.neo_scan_destruct(scan)

            yield Scan(samples=samples)

    ### Reset the device
    def reset(self):
        self._assert_scoped();

        error = ctypes.c_void_p();
        libneo.neo_device_reset(self.device, ctypes.byref(error))

        if error:
            raise _error_to_exception(error)
