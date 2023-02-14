# Prepare build:
```
git clone -b v4.4.4 --recursive https://github.com/espressif/esp-idf.git
source esp-idf/export.sh
git submodule update --init --recursive
```

# Configure WiFi
python3 configure.py <SSID>  <PASSWORD>


# Edit constants in modules/config.py

- NAME
- MQTT_SERVER 

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
```

# Get OTA logs:
```
python3 logs.py <NODE_ID>
```

# To access webrepl:
```
https://github.com/mzakharo/upydev
```


