from machine import WDT
class FakeWDT:
    def __init__(self, timeout):pass
    def feed(self): pass
wdt = WDT(timeout=70000)
#wdt = FakeWDT(timeout=70000)

import uos
from umqtt.robust import MQTTClient
import config
import io
from machine import unique_id

NODE_ID = "".join("%x" % b for b in unique_id())
MQTT_LOG_TOPIC = f'{config.NAME}/{NODE_ID}/logs'

def random_string(length=20):
    source = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890'
    return ''.join([source[x] for x in [(uos.urandom(1)[0] % len(source)) for _ in range(length)]])

class MyMQTT(MQTTClient):
    connected = False
    def connect(self):
        ret = super().connect()
        self.connected = True
        return ret
    def disconnect(self):
        ret = super().disconnect()
        self.connected = False
        return ret

client = MyMQTT(client_id=random_string(), server=config.MQTT_SERVER)

class DUP(io.IOBase):
    def __init__(self, client):
        self.s = bytearray()
        self.client = client
    def write(self, data):
        if data == b'\r\n':
            flush = True
        else:
            flush = False
            self.s += data
        if flush:
            if self.client.connected:
                self.client.publish(MQTT_LOG_TOPIC, self.s, qos=0, retain=True)
                self.s = bytearray()
            else:
                self.s += b'\r\n'
        return len(data)
    def readinto(self, data):
        return 0
uos.dupterm(DUP(client))

try:
    import app
    app.run(client, wdt)
except Exception as e:
    if not client.connected:
        import network
        wlan = network.WLAN(network.STA_IF)
        wlan.active(True)
        if not wlan.isconnected():
            wlan.connect(config.WIFI_SSID, config.WIFI_PASSWORD)
        while not wlan.isconnected(): pass
        client.connect()
    import sys
    sys.print_exception(e)
