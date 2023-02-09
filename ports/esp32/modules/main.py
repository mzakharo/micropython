import utime as time
import ujson as json
import uos
import io
from machine import WDT, deepsleep, lightsleep
from feathers2 import set_ldo2_power, set_led, AMB_LIGHT
from umqtt.robust import MQTTClient
from machine import ADC, Pin, reset
import network

from esp32 import Partition
try:
    Partition.mark_app_valid_cancel_rollback()
except Exception as e:
    print('mark', e)
for p in Partition.find(Partition.TYPE_APP):
    print(p)
print("active partition:", Partition(Partition.RUNNING).info()[4])

uversion = uos.uname().version
print('FW version: ', uversion)

class FakeWDT:
    def __init__(self, timeout):pass
    def feed(self): pass

#wdt = WDT(timeout=70000)
wdt = FakeWDT(timeout=70000)

PROFILING=False
PROFILING=True

#how long to wait between measurements
SLEEP = 600_000 #3600_000
if PROFILING:
    SLEEP = 5_000

#how long to wait for sensor to calibrate
ORP_SLEEP = 60_000
if PROFILING:
    ORP_SLEEP = 3_000

# do not send mqtt
MQTT_DUMMY = False
#MQTT_DUMMY = True

#disable lightsleep to allow prints
DISABLE_LIGHTSLEEP = False
DISABLE_LIGHTSLEEP = True

#disable deep sleep
DISABLE_DEEPSLEEP = False
DISABLE_DEEPSLEEP = True

SLEEP_FAST = 60_000

WIFI_SSID = 'wireless_ext'
if PROFILING:
    WIFI_SSID = 'wireless'
WIFI_PASSWORD = 'W@terl004ever'
MQTT_SERVER = '192.168.50.96'
MQTT_TOPIC = 'feather/status'
if PROFILING:
    MQTT_TOPIC = 'feather/profiling'


ORP_PIN = const(1)
BAT_PIN = const(3)
LDO2 = const(21)

#import esp
#esp.osdebug(0, esp.LOG_DEBUG)
#tic = time.ticks_ms()

def random_string(length=20):
    source = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890'
    return ''.join([source[x] for x in [(uos.urandom(1)[0] % len(source)) for _ in range(length)]])

def mean(v):
    return sum(v) / len(v)

def median(v):
    l = sorted(v)
    return l[len(v)//2]


def my_go_deepsleep(duration):
    Pin(ORP_PIN, Pin.IN, Pin.PULL_DOWN, hold=True)
    Pin(BAT_PIN, Pin.IN, Pin.PULL_UP, hold=True)
    #Pin(AMB_LIGHT, Pin.IN, Pin.PULL_UP, hold=True)
    deepsleep(duration)
    #from feathers2 import go_deepsleep
    #go_deepsleep(duration)


def my_sleep_ms(duration):
    if DISABLE_LIGHTSLEEP:
        time.sleep_ms(duration)
    else:
        lightsleep(duration)

set_led(False) #disable for low power
o = ADC(Pin(ORP_PIN)) #Yellow 
o.atten(ADC.ATTN_11DB)    # set 11dB input attenuation (voltage range roughly 0.0v - 3.6v)
o.width(ADC.WIDTH_13BIT)

b = ADC(Pin(BAT_PIN)) #Yellow 
b.atten(ADC.ATTN_11DB)    # set 11dB input attenuation (voltage range roughly 0.0v - 3.6v)
b.width(ADC.WIDTH_13BIT)

uid = random_string()

wlan = network.WLAN(network.STA_IF)

def wifi():
    s = time.ticks_ms()

    wlan.active(True)

    if not wlan.isconnected():
        wlan.connect(WIFI_SSID, WIFI_PASSWORD)

    diff = 0
    while not wlan.isconnected():
        diff = time.ticks_diff(time.ticks_ms(), s)
        if diff > 15_000:
            my_go_deepsleep(SLEEP_FAST)
        else:
            wdt.feed()
    print('connected in ', diff)
    return wlan

'''
while True:
    wlan = wifi()
    wlan.disconnect()
    wlan.active(False)
    wdt.feed()
'''

client = MQTTClient(client_id=uid, server=MQTT_SERVER)

class State:
    def __init__(self):
        self.prevent_sleep = False
        self.subscribe_ota = False
        self.measured = False
        self.check_sha = b''
        self.check = {}
        try:
            with open('check_sha.txt', 'r') as f:
                self.check = json.loads(f.read())
        except Exception as e:
            print('check err', e)



s = State()

def on_message(topic, msg):
    print('received', topic, msg[:100])
    if topic == b'tubby/ota/cmd':
        if msg == b'webrepl':
            print('enabling webrepl')
            import webrepl
            webrepl.start(password='admin')
            s.prevent_sleep = True
        else:
            sha = msg.decode()
            part = Partition(Partition.RUNNING).info()[4]
            my_sha = s.check.get('sha', '')
            my_part = s.check.get('part', '')

            if my_sha == sha and my_part == part:
                print('OTA skip: already running this image')
                return

            print(f'OTA Queue: my_sha: {my_sha}, sha: {sha} my_part: {my_part} part: {part}')
            s.prevent_sleep = True
            s.subscribe_ota = True
            s.check_sha = sha

    elif topic == b'tubby/ota/fw':
        print('OTA write: fw len:', len(msg))
        s.prevent_sleep = False
        from ota import OTA
        fw = io.BytesIO(msg)
        ot = OTA(fw, 0, check_sha=s.check_sha, tls=True)
        BLOCKLEN = 4096
        n_blocks = (len(msg) // BLOCKLEN) + 1
        ot.do_ota(blocks=n_blocks, debug=True)
        ot.check_ota()
        with open('check_sha.txt', 'w') as f:
            f.write(json.dumps({'sha': s.check_sha, 'part' :  Partition(Partition.BOOT).info()[4]}))
        print('Rebooting device...')
        time.sleep(1)
        reset()


def mqtt():
    wifi()
    #print('network config:', wlan.ifconfig())
    client.set_callback(on_message)
    client.connect()
    client.subscribe('tubby/ota/cmd')
    return client

def measure5():
    status = {}
    set_ldo2_power(True)
    wdt.feed()
    my_sleep_ms(ORP_SLEEP)
    wdt.feed()
    orp = o.read() - 1500
    set_ldo2_power(False)
    status[f'orp60'] = orp
    #get battery voltage
    vbat = b.read() * 2
    status['vbat'] = float(vbat)
    status['sha'] = s.check.get('sha', '')
    status['ver'] = uversion
    publish(status)
    return True


def publish(status):
    mqtt()
    if MQTT_DUMMY:
        print('status', status)
    else:
        client.publish(MQTT_TOPIC, json.dumps(status), qos=1)


while True:
    wdt.feed()

    if not s.measured:
        s.measured = measure5()

    if s.subscribe_ota:
        s.subscribe_ota = False
        client.subscribe('tubby/ota/fw')

    if s.prevent_sleep:
        client.check_msg()
        continue

    if not DISABLE_DEEPSLEEP:
        my_go_deepsleep(SLEEP)

    print('done')
    s = State()
    client.disconnect()
    #wlan.active(False)
    for _ in range(5):
        wdt.feed()
        time.sleep(1)
