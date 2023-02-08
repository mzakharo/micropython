import utime as time
import ujson as json
import uos
from machine import WDT, deepsleep, lightsleep
from feathers2 import set_ldo2_power, set_led
from umqtt.robust import MQTTClient
from machine import ADC, Pin
import network

from esp32 import Partition
try:
    Partition.mark_app_valid_cancel_rollback()
except Exception as e:
    print('mark', e)
for p in Partition.find(Partition.TYPE_APP):
    print(p)
print("active partition:", Partition(Partition.RUNNING).info()[4])

class FakeWDT:
    def __init__(self, timeout):pass
    def feed(self): pass

#wdt = WDT(timeout=70000)
wdt = FakeWDT(timeout=70000)

import webrepl
webrepl.start(password='admin')

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
#DISABLE_LIGHTSLEEP = True

#disable deep sleep
DISABLE_DEEPSLEEP = False
#DISABLE_DEEPSLEEP = True

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
    #Pin(LDO2, Pin.IN, pull=Pin.PULL_HOLD | Pin.PULL_DOWN)
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

def on_message(topic, msg):
    print('received', topic, msg)

def mqtt():
    wifi()
    #print('network config:', wlan.ifconfig())
    client = MQTTClient(client_id=uid, server=MQTT_SERVER)
    client.set_callback(on_message)
    client.connect()
    client.subscribe('tubby/ota_mode')
    return client


def measure(cnt):
    client = mqtt()
    set_ldo2_power(True)
    for _ in range(cnt):
        tic = time.ticks_ms()
        orp = o.read() - 1500
        #vbat = b.read() * 2
        status = dict(orp=float(orp))
        print(status)
        client.publish("feather/status", json.dumps(status), qos=0)
        wdt.feed()
        toc = time.ticks_ms()
        diff = time.ticks_diff(toc, tic)
        slp = 100 - diff
        if slp > 0:
            time.sleep_ms(slp)
        else:
            print('diff', diff)
    set_ldo2_power(False)
    client.disconnect()

def measure2(cnt):
    client = mqtt()
    set_ldo2_power(True)
    prev = -2000
    vs = []
    bcnt = 0
    orp_c_sent = False
    for i in range(cnt):
        tic = time.ticks_ms()
        v = o.read() - 1500
        wdt.feed()
        if prev is None:
            prev = v
        vs.append(v)
        if i % 50 == 0:
            mvs = median(vs)

            orp_c = None
            vs.clear()
            change = abs(mvs - prev)
            prev = mvs
            if (change <= 4):
                bcnt += 1
                if bcnt >= 2:
                    print(i, mvs, change,  'bcnt', bcnt, 'break')
                    orp_c = mvs
                else:
                    print(i, mvs, change,  'bcnt', bcnt)
            else:
                bcnt = 0
                print(i, mvs, change)
            orp = mvs
            status = dict(orp=float(orp))
            if i == 450:
                status['orp_static'] = orp
            if orp_c is not None and not orp_c_sent:
                orp_c_sent = True
                status['orp_c'] = orp_c
                status['orp_i'] = i
            client.publish("feather/status", json.dumps(status), qos=0)
        toc = time.ticks_ms()
        diff = time.ticks_diff(toc, tic)
        slp = 100 - diff
        if slp > 0:
            time.sleep_ms(slp)
        else:
            print('diff', diff)
    set_ldo2_power(False)
    client.disconnect()

def measure3():
    client = mqtt()
    N = 15
    set_ldo2_power(True)
    prev = -10000
    for i in range(N):
        tic = time.ticks_ms()
        wdt.feed()
        orps = [o.read() for _ in range(100)]
        orp = median(orps) - 1500
        d = orp - prev
        prev = orp
        status = dict(orp=float(orp))
        if i == N-1:
            status['orp_final'] = float(orp)
        print(i, status, 'delta', d)
        client.publish("feather/status", json.dumps(status), qos=1)
        toc = time.ticks_ms()
        diff = time.ticks_diff(toc, tic)
        slp = 1000 - diff
        if slp > 0:
            time.sleep_ms(slp)
        else:
            print('diff', diff)
    set_ldo2_power(False)
    client.disconnect()


def measure4():
    status = {}
    set_ldo2_power(True)
    s = time.ticks_ms()
    slept = 0
    for i in [2, 15, 30, 45, 60]:
        to_sleep = i * 1000 - slept
        wdt.feed()
        my_sleep_ms(to_sleep)
        slept += to_sleep

        orp = o.read() - 1500
        status[f'orp{i}'] = orp
        diff = time.ticks_diff(time.ticks_ms(), s)
        #status[f'i_orp{i}'] = diff
        print(i, diff, orp)
        
    set_ldo2_power(False)

    #get battery voltage
    vbat = b.read() * 2
    status['vbat'] = float(vbat)
    publish(status)

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
    return publish(status)


def publish(status):
    client = mqtt()
    if MQTT_DUMMY:
        print('status', status)
    else:
        client.publish(MQTT_TOPIC, json.dumps(status), qos=1)
    #client.disconnect()
    return client


while True:
    #measure2(10*60+1)
    #measure3()
    #measure4()
    client = measure5()
    if not DISABLE_DEEPSLEEP:
        my_go_deepsleep(SLEEP)
    print('done')
    client.disconnect()
    #wlan.active(False)
    for _ in range(5):
        wdt.feed()
        time.sleep(1)
