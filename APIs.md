APIs for neo-f1
===
[TOC]

### APIs (C++)

1.
``` C++
int32_t neo_get_version(void);
```

Returns the neo sdk version.

2.
``` C++
static neo_error_s neo_error_construct(const char*);
void neo_error_destruct(neo_error_s*);
```

Construct and destruct of neo_error_s.

3.
``` C++
neo(const char* port);
neo(const char* port, int32_t baudrate);
```

Construct of neo device based on a serial device port (e.g. `/dev/ttyACM0` on Linux or `COM8` on Windows)
or a `baudrate` (default 115200).

4.
``` C++
void start_scanning(void);
void stop_scanning(void);
```

Neo device start/stop scanning api.

5.
``` C++
int32_t get_motor_speed(void);
void set_motor_speed(int32_t speed);
```

Neo device get/set motor speed [range: 0-10].

6.
``` C++
scan get_scan(void);
```

Neo device get the scan data.

7.
``` C++
void reset(void);
void calibrate(void);
```

Reset the neo device.

