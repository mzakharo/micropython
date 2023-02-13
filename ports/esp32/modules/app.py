import utime as time
import ujson as json
import uos
import io
from machine import deepsleep, lightsleep, unique_id
from feathers2 import set_ldo2_power, set_led, AMB_LIGHT
from machine import ADC, Pin, reset
from esp32 import Partition
import config
import network
import tfmicro #custom user module for tflite-micro models

PROFILING=False
PROFILING=True

#NOTE: disable lightsleep/deepsleep to allow USB-UART to stay connected

DISABLE_LIGHTSLEEP = False
#DISABLE_LIGHTSLEEP = True

#disable deep sleep
DISABLE_DEEPSLEEP = False
#DISABLE_DEEPSLEEP = True

NUM_SAMPLES = 20
if PROFILING:
    NUM_SAMPLES = 2

#how long to wait between measurements
SLEEP = 1800_000 # 30 minutes
if PROFILING:
    SLEEP = 5_000

#how long to wait for sensor to calibrate
ORP_SLEEP = 60_000
if PROFILING:
    ORP_SLEEP = 3_000

#sleep due to wlan connect error
SLEEP_FAST = 60_000

NODE_ID = "".join("%x" % b for b in unique_id())

name = config.NAME
MQTT_OTA_CMD_TOPIC = f'{name}/{NODE_ID}/ota/cmd'
MQTT_OTA_FW_TOPIC = f'{name}/{NODE_ID}/ota/fw'
MQTT_STATE_TOPIC = f'{name}/{NODE_ID}/status'
MQTT_BALBOA_TOPIC = f'balboa/temp'

ORP_PIN = const(1)
BAT_PIN = const(3)
PH_PIN = const(7)

def wifi(wdt, ssid, password):
    wlan = network.WLAN(network.STA_IF)
    s = time.ticks_ms()

    wlan.active(True)

    if not wlan.isconnected():
        wlan.connect(ssid, password)

    diff = 0
    while not wlan.isconnected():
        diff = time.ticks_diff(time.ticks_ms(), s)
        if diff > 15_000:
            my_go_deepsleep(SLEEP_FAST)
        else:
            wdt.feed()
    print('connected in ', diff)

def mean(v):
    return sum(v) / len(v)

def median(v):
    l = sorted(v)
    return l[len(v)//2]

def my_go_deepsleep(duration):
    Pin(ORP_PIN, Pin.IN, Pin.PULL_DOWN, hold=True)
    Pin(PH_PIN, Pin.IN, Pin.PULL_DOWN, hold=True)
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

class State:
    def __init__(self):
        self.prevent_sleep = False
        self.subscribe_ota = False
        self.measured = False
        self.check_sha = b''
        self.check = {}
        self.temp = 40.0
        try:
            with open('check_sha.txt', 'r') as f:
                self.check = json.loads(f.read())
        except Exception as e:
            print('check err', e)

#automatic temperature compensation
# from here: https://docs.rbr-global.com/support/instruments/compensation-equations/ph-simple-temperature-correction-of-ph
def atc(ph, temp):
    return ph -0.0032 * (ph - 7.0)*(temp-25.0)


def resample(array):
    keys = array[0].keys()
    out = {}
    for key in keys:
        out[key] = []
    for v in array:
        for key in keys:
            out[key].append(v[key])
    for key in keys:
        out[key] = mean(out[key])
    return out


def run(client, wdt):

    uversion = uos.uname().version
    print('FW version: ', uversion)
    print('Node ID: ', NODE_ID)

    #import esp
    #esp.osdebug(0, esp.LOG_DEBUG)
    #tic = time.ticks_ms()


    set_led(False) #disable for low power

    #ADC.ATTN_0DB: No attenuation (100mV - 950mV)
    #ADC.ATTN_2_5DB: 2.5dB attenuation (100mV - 1250mV)
    #ADC.ATTN_6DB: 6dB attenuation (150mV - 1750mV)
    #ADC.ATTN_11DB: 11dB attenuation (150mV - 2450mV)

    o = ADC(Pin(ORP_PIN), atten=ADC.ATTN_11DB) 
    p = ADC(Pin(PH_PIN),  atten=ADC.ATTN_11DB) 
    b = ADC(Pin(BAT_PIN), atten=ADC.ATTN_11DB) 

    s = State()

    def on_message(topic, msg):
        topic = topic.decode()
        #print('received', topic, msg[:100])
        if topic == MQTT_OTA_CMD_TOPIC:
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

        elif topic == MQTT_OTA_FW_TOPIC:
            print('OTA write: fw len:', len(msg))
            s.prevent_sleep = False
            wdt.feed()
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
        elif topic == MQTT_BALBOA_TOPIC:
            s.temp = float(msg)
            #print('Balboa temp %f' % s.temp)


    def discovery(client):
        model = uos.uname().machine
        sha = s.check.get('sha', '')[:8]

        device =  {
                            'identifiers' : NODE_ID,
                            'name' : name,
                            'sw_version' : uversion + '_' + sha,
                            'model' : model,
                            'manufacturer' : NODE_ID, # for HA UI  -> easy display
                        }

        t = f'homeassistant/sensor/{NODE_ID}/{name}_battery/config'
        m = {'device_class' : 'voltage',
            'unit_of_measurement': 'mV',
            'icon' : 'mdi:battery-outline',
            'name': f'{name} Battery',
            'state_topic': MQTT_STATE_TOPIC,
            'unique_id' : f'ESPsensor{NODE_ID}_battery',
            'device' : device,
            'value_template' : "{{ value_json.vbat }}",
            'force_update' : True,
            'expire_after' : 3 * SLEEP // 1000,
            }
        client.publish(t, json.dumps(m), qos=0, retain=True)

        t = f'homeassistant/sensor/{NODE_ID}/{name}_orp/config'
        m = {'device_class' : 'voltage',
            'unit_of_measurement': 'mV',
            'name': f'{name} ORP',
            'state_topic': MQTT_STATE_TOPIC,
            'unique_id' : f'ESPsensor{NODE_ID}_orp',
            'device' : device,
            'value_template' : "{{ value_json.orp }}",
            'force_update' : True,
            'expire_after' : 3 * SLEEP // 1000,
            }
        client.publish(t, json.dumps(m), qos=0, retain=True)

        t = f'homeassistant/sensor/{NODE_ID}/{name}_temp/config'
        m = {'device_class' : 'temperature',
            'unit_of_measurement': "C",
            'name': f'{name} Temperature',
            'state_topic': MQTT_STATE_TOPIC,
            'unique_id' : f'ESPsensor{NODE_ID}_temp',
            'device' : device,
            'value_template' : "{{ value_json.temp }}",
            'force_update' : True,
            'expire_after' : 3 * SLEEP // 1000,
            }
        client.publish(t, json.dumps(m), qos=0, retain=True)

        t = f'homeassistant/sensor/{NODE_ID}/{name}_ph/config'
        m = {
            'unit_of_measurement': "dB",
            'name': f'{name} PH',
            'icon' : 'mdi:ph',
            'state_topic': MQTT_STATE_TOPIC,
            'unique_id' : f'ESPsensor{NODE_ID}_ph',
            'device' : device,
            'value_template' : "{{ value_json.ph }}",
            'force_update' : True,
            'expire_after' : 3 * SLEEP // 1000,
            }
        client.publish(t, json.dumps(m), qos=0, retain=True)

        t = f'homeassistant/sensor/{NODE_ID}/{name}_fb_ppm/config'
        m = {
            'unit_of_measurement': 'ppm',
            'name': f'{name} Free Bromine',
            'icon' : 'mdi:chemical-weapon',
            'state_topic': MQTT_STATE_TOPIC,
            'unique_id' : f'ESPsensor{NODE_ID}_fb_ppm',
            'device' : device,
            'value_template' : "{{ value_json.fb_ppm }}",
            'force_update' : True,
            'expire_after' : 3 * SLEEP // 1000,
            }
        client.publish(t, json.dumps(m), qos=1, retain=True) #last one with qos=1 so that we can receive balboa temperature

    def mqtt():
        wifi(wdt, config.WIFI_SSID, config.WIFI_PASSWORD)
        client.set_callback(on_message)
        client.connect()
        client.subscribe(MQTT_OTA_CMD_TOPIC)
        client.subscribe(MQTT_BALBOA_TOPIC) # for temperature
        discovery(client)
        return client


    def measure():
        set_ldo2_power(True)
        wdt.feed()
        my_sleep_ms(ORP_SLEEP)
        wdt.feed()

        values = []
        for i in range(NUM_SAMPLES):
            status = {}
            orp = o.read_uv() // 1000 - 1500
            status['orp'] = orp
            vbat = b.read_uv() // 1000 * 2
            status['vbat'] = vbat
            phv = p.read_uv() / 1e6
            ph = (-5.6548 * phv) + 15.509
            status['ph'] = ph 
            values.append(status)
            if i != NUM_SAMPLES - 1:
                my_sleep_ms(1_000)

        set_ldo2_power(False)

        status = resample(values)

        #convert from float
        status['orp'] = round(status['orp'])
        status['vbat'] = round(status['vbat'])

        return status

    while True:

        if not s.measured:
            status = measure()
            mqtt()

            #perform ATC (Automatic Temperature Compensation) correction to PH
            status['ph'] = atc(status['ph'], s.temp)
            #estimate free bromine ppm
            fb = tfmicro.fc(status['orp'], status['ph'])
            if fb is None:
                fb = -1.0
            fb *= 2.25
            status['fb_ppm'] = fb
            status['temp'] = s.temp

            client.publish(MQTT_STATE_TOPIC, json.dumps(status), qos=1)

            #mark success
            s.measured = True
            Partition.mark_app_valid_cancel_rollback()

        if s.subscribe_ota:
            s.subscribe_ota = False
            client.subscribe(MQTT_OTA_FW_TOPIC)

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
