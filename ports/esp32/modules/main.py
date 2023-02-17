import utime as time
import uos as os
import network
import machine
from machine import deepsleep, I2C, Pin, RTC
from feathers2 import set_ldo2_power, set_led
import urtc

DEBUG=False
#DEBUG=True

if DEBUG:
    MIN_1 = (5)
    MIN_5 = (5)
    MIN_10 = (10)
    MIN_15 = (10)
    MIN_30 = (10)
else:
    MIN_1 = (1*60)
    MIN_5 = (5*60)
    MIN_10 = (10*60)
    MIN_15 = (15*60)
    MIN_30 = (30*60)


#time to sense
AWAKE_TIME = MIN_10
#time to sleep during sensing
SLEEP_TIME = MIN_10
#time to sleep after detecting a packet
COOLDOWN = SLEEP_TIME
#time to sleep due to the schedule
SCHED_TIME = MIN_30



i2c = I2C(0, scl=Pin(9), sda=Pin(8))
rtc = urtc.PCF8523(i2c)

# change to True if you want to write the time!
if False:   
    # year month day weekday, hour, minute, second, microsecond
    t = (2022,  07,   17, 6, 8,  1,  0,    0 )
    print("Setting time to:", t)
    rtc.datetime(t)

'''
if rtc.lost_power():
    print("WARNING: neeed to reset RTC")
if rtc.battery_low():
    print("WARNING: RTC battery low")
'''
rtc0 = RTC()
def sync_time():
    rtc0.datetime(tuple(rtc.datetime()))
sync_time()

def read():
    with open('log.csv') as f:
        print(f.read())

def disable():
    network.promiscuous_disable()

def my_deepsleep(seconds):
    disable()
    deepsleep(seconds * 1000)

def mac_to_str(a):
    return ("%02x:%02x:%02x:%02x:%02x:%02x" % (((a[0])), ((a[1])), ((a[2])),
                                               ((a[3])), ((a[4])), (a[5])))

day = {0 : 'mon',
        1 : 'tue',
        2 : 'wed',
        3 : 'thu',
        4 : 'fri',
        5 : 'sat',
        6 : 'sun',
        }

def get_time():
    t = time.localtime()
    return f'{t[0]}{t[1]:02d}{t[2]:02d}_{t[3]:02d}{t[4]:02d}{t[5]:02d}'
    #return (day[t[6]],t[3],t[4], t[1], t[2])

def get_time_sched():
    t = time.localtime()
    return (day[t[6]],t[3],t[4], t[1], t[2])


def monitor_callback(mac_in):
    if mac_in is None:
        print('got none')
        return
    mac = mac_to_str(mac_in)
    if not mac_in[0] & 0x2:
        suffix = 'G'
    else:
        suffix = 'L'
        mac = '00:00:00:00:00:00'
    t = get_time()
    s ="{},{},{}\n".format(t,mac,suffix)
    with open('log.csv', 'a') as f:
        f.write(s)
    print(s, end="")
    set_led(True)
    time.sleep(1)
    set_led(False)
    if not DEBUG:
        my_deepsleep(COOLDOWN)

def sleep_schedule(t):
    wkd,hour,_,_,_ = t 
    if (hour >= 7 and hour <=19):# and (wkd in ['sat', 'sun'] or hour >= 16):
        return False
    return True #sleep sleep sleep

def check_schedule():
    t = get_time_sched()
    to_sleep = sleep_schedule(t)
    print(f'{t}-sched, sleep={to_sleep}')
    if to_sleep and not DEBUG:
        my_deepsleep(SCHED_TIME)

def clear():
    disable()
    os.remove('log.csv')

def enable(ch=6):
    check_schedule()
    sta_if = network.WLAN(network.STA_IF)
    sta_if.active(True)
    print("Promiscuous mode on channel {}".format(network.set_channel(ch)))
    network.promiscuous_enable(monitor_callback)
    while True:
        time.sleep(AWAKE_TIME)
        #sync_time()
        #check_schedule()
        my_deepsleep(SLEEP_TIME)

if machine.reset_cause() != machine.DEEPSLEEP_RESET:
    set_led(True)
    for i in range(5):
        print(f'waiting for user reset: {i}/5')
        time.sleep(1)

#for low power
set_ldo2_power(False)
set_led(False) 
machine.freq(80_000_000) #any lower hangs wifi

enable()
