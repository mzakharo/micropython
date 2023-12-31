# Prepare the repository  (Under Linux or WSL):
```
git clone https://github.com/mzakharo/tubby.git
cd tubby
git submodule update --init
cd tflite-micro-esp-examples/
git submodule update --init
cd ../micropython/ports/esp32/
```

# Install Tools
```
sudo apt install build-essential cmake
pip3 install paho-mqtt
git clone -b v4.4.4 --recursive https://github.com/espressif/esp-idf.git
source esp-idf/export.sh
make submodules
```

# Configure WiFi & MQTT
```
python3 configure.py <WIFI_SSID> <WIFI_PASSWORD> <MQTT_SERVER>
```

# Build:
```
make
```

# Erase flash:
```
make erase
```

# Flash firmware:
```
make deploy
```

# Get MQTT logs:
```
python3 logs.py +
```

Note the NODE_ID

# Publish firmware for OTA:
```
python3 publish_fw.py <NODE_ID>  build-UM_FEATHERS2/micropython.bin
Example:
python3 publish_fw.py 84f7373c0e build-UM_FEATHERS2/micropython.bin 
```

# Clear firmware for OTA:
```
python3 publish_fw.py <NODE_ID>
Example:
python3 publish_fw.py 84f7373c0e
```




