import paho.mqtt.publish as publish
import sys
import hashlib
from binascii import hexlify

import modules.config as config 

NODE_ID = sys.argv[1]

if len(sys.argv) == 3:
    with open(sys.argv[2], 'rb') as f:
        b = f.read()

    BLOCKLEN = 4096
    n_blocks = (len(b) // BLOCKLEN) + 1
    hf = hashlib.sha256(b)
    padding = b'\xff'*((n_blocks*BLOCKLEN)-len(b))
    b += padding
    hf.update(padding)
    check_sha = hexlify(hf.digest()).decode()

    print('publishing len', len(b), 'sha', check_sha)
    publish.single(f'{config.NAME}/{NODE_ID}/ota/fw', b, qos=1, retain=True, hostname=config.MQTT_SERVER)
    publish.single(f'{config.NAME}/{NODE_ID}/ota/cmd', check_sha, qos=1, retain=True, hostname=config.MQTT_SERVER)
else:
    publish.single(f'{config.NAME}/{NODE_ID}/ota/cmd', '', qos=1, retain=True, hostname=config.MQTT_SERVER)
    publish.single(f'{config.NAME}/{NODE_ID}/ota/fw', '', qos=1, retain=True, hostname=config.MQTT_SERVER)
