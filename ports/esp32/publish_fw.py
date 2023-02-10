import paho.mqtt.publish as publish
import sys
import hashlib
from binascii import hexlify


if len(sys.argv) == 2:
    with open(sys.argv[1], 'rb') as f:
        b = f.read()

    BLOCKLEN = 4096
    n_blocks = (len(b) // BLOCKLEN) + 1
    hf = hashlib.sha256(b)
    padding = b'\xff'*((n_blocks*BLOCKLEN)-len(b))
    b += padding
    hf.update(padding)
    check_sha = hexlify(hf.digest()).decode()

    print('publishing len', len(b), 'sha', check_sha)
    publish.single('tubby/ota/fw', b, qos=1, retain=True, hostname='nas.local')
    publish.single('tubby/ota/cmd', check_sha, qos=1, retain=True, hostname='nas.local')
else:
    publish.single('tubby/ota/cmd', '', qos=1, retain=True, hostname='nas.local')
    publish.single('tubby/ota/fw', '', qos=1, retain=True, hostname='nas.local')
