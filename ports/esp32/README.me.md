Prepare build:

git clone -b v4.4.4 --recursive https://github.com/espressif/esp-idf.git
source esp-idf/export.sh
git submodule update --init --recursive

Build:
make

Erase flash:
make erase

Deploy:
make deploy

Publish firmware for OTA:
python3 publish_fw.py <NODE_ID>  build-UM_FEATHERS2/micropython.bin 

To access webrepl:
https://github.com/mzakharo/upydev


Get OTA logs:
python3 logs.py <NODE_ID>
