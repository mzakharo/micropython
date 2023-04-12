# Prepare build:
```
git clone -b v4.4.4 --recursive https://github.com/espressif/esp-idf.git
source esp-idf/export.sh
git submodule update --init --recursive
```

# Configure WiFi & MQTT
python3 configure.py <WIFI_SSID> <WIFI_PASSWORD> <MQTT_SERVER>

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

# Publish firmware for OTA:
```
python3 publish_fw.py <NODE_ID>  build-UM_FEATHERS2/micropython.bin 
python3 publish_fw.py 84f7373c0e build-UM_FEATHERS2/micropython.bin 
```

# Get OTA logs:
```
python3 logs.py +
```

# To access webrepl:
```
https://github.com/mzakharo/upydev
```


