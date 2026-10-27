# Only Once
idf.py set-target esp32c6
idf.py menuconfig
idf.py reconfigure

# Build and Flash
source setup.sh
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 flash monitor

# Operation
On joystick power-up right slider switch picks startup mode:
- Slider to Right : Robot selection mode
- Slider to Left : Robot control mode

## Selection Mode
In selection mode, the device to connect to can be chosen from a pre-defined list.
Moving the slider switch to the left will leave selection mode and enter control mode.  Leaving selction mode will also save selected index to non-volatile memory.

## Control Mode
In control mode, a connection will made to the selected robot.  The right and left Once connected, the joystick signals will be sent to the connected robot as command inputs.

Moving the slider switch will not leave control-mode.  This is a precaution, to avoid
accidentally switching modes during a robot match.  To enter selection mode, the slider must be moved and power switch get cycled.

The roundtrip time (RTT), received signal strength (RSS), and battery voltage feedback will be screen.

If connection is dropped the joystick will automatically try to reconnect again.
A connection count is displayed on the screen.  It can be used to detect a robot than might be resetting because a brown-out or bad battery connection.

## Host simulator (Linux)

`sim/` runs the same LVGL screens on the desktop through SDL2. One LVGL window is the 172×320 panel. A second LVGL window feeds the stick, slider switch, RSSI, round-trip time, connection count, and robot battery.

```
sudo apt install libsdl2-dev
cmake -S sim -B sim/build
cmake --build sim/build
./sim/build/robojoy_ui
```