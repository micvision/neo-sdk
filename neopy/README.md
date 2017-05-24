neopy module
---

[toc]

### ENVIRONMENT

This module requires `libneo.so` to be installed.

### INSTALLATION

1. Linux:

	Install `neopy` module locally:
	``` bash
	python setup.py install --user
	```

2. Windows:

	You also must install the correct version of [neo](https://github.com/micvision/neo-sdk) library.
    At the same time, you should download python and adding it to you environment.
    ``` bash
    python.exe setup.py install --user
    ```

### USAGE

1. Linux:

    ``` bash
    python -m neopy /dev/ttyACM0
    ```

2. Windows:

	``` bash
    python.exe -m neopy COM8
    ```

    *NOTE*: you should change the serial port to what show on you computer.

### API

``` python
class neo:
    ### Construct of neo class
    def __init__(neo_device, port, bitrate = None) -> neo device

    ### Destruct of neo class
    def __exit__(neo_device, *args):               -> void

    ### Start scanning api, using C language neo_device_start_scanning function
    def start_scanning(neo_device):                -> void

    ### Stop scanning
    def stop_scanning(neo_device):                 -> void

    ### Get motor speed
    def get_motor_speed(neo_device):               -> int

    ### Set motor speed
    def set_motor_speed(neo_device, speed):        -> void

    ### Get sample rate
    def get_sample_rate(neo_device):               -> int

    ### Set sample rate
    def set_sample_rate(neo_device, speed):        -> void

    ### Get scan data
    def get_scans(neo_device):                     -> scan

    ### Reset the device
    def reset(neo_device):                         -> void
```