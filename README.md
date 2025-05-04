# Only Once
idf.py set-target esp32c6
idf.py menuconfig
idf.py reconfigure

# Build and Flash
source setup.sh
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 flash monitor
