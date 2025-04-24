import network
import espnow
import json
import machine
from machine import WDT
from machine import Pin, PWM
wdt = WDT(timeout=5000)

# A WLAN interface must be active to send()/recv()
sta = network.WLAN(network.WLAN.IF_STA)
sta.active(True)
sta.config(channel=6) 

now = espnow.ESPNow()
now.active(True)
now.config(rate=0x29) # 0x29 LORA 250KBps
peer = b'\xbb\xbb\xbb\xbb\xbb\xbb'   # MAC address of peer's wifi interface
now.add_peer(peer)


# --- Configuration ---
ADC_PIN_V_MID = 5
ADC_PIN_V_TOTAL = 4

# --- Resistor values for V_MID (Cell 1) divider ---
R1_MID = 100000.0  # R1b
R2_MID = 330000.0  # R2b
VOLTAGE_MULTIPLIER_MID = (R1_MID + R2_MID) / R2_MID * 1.045 # Approx 1.303

# --- Resistor values for V_TOTAL divider ---
R1_TOTAL = 100000.0 # R1a
R2_TOTAL = 47000.0  # R2a
VOLTAGE_MULTIPLIER_TOTAL = (R1_TOTAL + R2_TOTAL) / R2_TOTAL * 1.020 # Approx 3.128

ADC_ATTENUATION = machine.ADC.ATTN_11DB

adc_mid = machine.ADC(machine.Pin(ADC_PIN_V_MID))
adc_mid.atten(ADC_ATTENUATION)

adc_total = machine.ADC(machine.Pin(ADC_PIN_V_TOTAL))
adc_total.atten(ADC_ATTENUATION)


coefficients = [-330.54949578, 4873.56097706, -26754.59502068,  64907.5504285, -58774.37023182]
def estimate_soc_poly(voltage, coeffs=coefficients):
    #voltage = min(4.2, max(3.2, voltage))
    # Calculate polynomial value using Horner's method (more efficient)
    # P(V) = (((c_n*V + c_{n-1})*V + c_{n-2})*V + ... + c_1)*V + c_0
    soc = coeffs[0]
    for i in range(1, len(coeffs)):
        soc = soc * voltage + coeffs[i]
    soc_clamped = max(0.0, min(100.0, soc))
    return round(soc_clamped)

# Deadzone settings
DEADZONE_SIZE = 20      #  around center
JOY_CENTER = 127        # Center joystick value
JOY_MIN = 0             # Minimum joystick value
JOY_MAX = 255           # Maximum joystick value

def joystick_to_servo(joy_value, min_val, max_val, center_val):
    """
    Convert joystick value to servo PWM duty cycle.
    
    Parameters:
    - joy_value: Joystick input (0-255, center at 127)
    - joy_min: Minimum joystick value (default 0)
    - joy_max: Maximum joystick value (default 255)
    - joy_center: Center joystick value (default 127)
    
    Returns: PWM duty cycle value for servo control
    """
    # Ensure joy_value is within bounds
    joy_value = max(JOY_MIN, min(JOY_MAX, joy_value))
    
    # Check if in deadzone (near center)
    if (JOY_CENTER - DEADZONE_SIZE) <= joy_value <= (JOY_CENTER + DEADZONE_SIZE):
        return center_val
    
    # Map values below deadzone
    elif joy_value < (JOY_CENTER - DEADZONE_SIZE):
        effective_min = JOY_CENTER - DEADZONE_SIZE
        return int(min_val + (center_val - min_val) * 
                (joy_value - JOY_MIN) / (effective_min - JOY_MIN))
    
    # Map values above deadzone
    else:  # joy_value > (JOY_CENTER + DEADZONE_SIZE)
        effective_max = JOY_CENTER + DEADZONE_SIZE
        return int(center_val + (max_val - center_val) * 
                (joy_value - effective_max) / (JOY_MAX - effective_max))

ch0 = PWM(Pin(3), freq=50)  # Set the frequency to 50Hz
CH0_MIN_DUTY = 1000000  
CH0_MAX_DUTY = 2000000

CH0_MIN_SLOWEST = 1400000
CH0_MAX_SLOWEST = 1600000
CH0_MIN_FASTEST = 1200000
CH0_MAX_FASTEST = 1800000

CH0_CENTER_DUTY = (CH0_MIN_DUTY + CH0_MAX_DUTY) // 2  # Center position
ch0.duty_ns(CH0_CENTER_DUTY)


ch1 = PWM(Pin(2), freq=50)  # Set the frequency to 50Hz
CH1_MIN_DUTY = 1130000  
CH1_MAX_DUTY = 1730000
CH1_CENTER_DUTY = (CH1_MIN_DUTY + CH1_MAX_DUTY) // 2  # Center position
ch1.duty_ns(CH1_CENTER_DUTY)

def get_soc():
    vbat_mid = adc_mid.read_uv() / 1e6 * VOLTAGE_MULTIPLIER_MID
    vbat_total = adc_total.read_uv() / 1e6 * VOLTAGE_MULTIPLIER_TOTAL
    vbat2 = vbat_total - vbat_mid
    soc1 = estimate_soc_poly(vbat_mid)
    soc2 = estimate_soc_poly(vbat2)
    print(vbat_mid, vbat2, soc1, soc2)
    return (soc1, soc2)

while True:
    try:
        host, msg = now.recv(1000)
        if msg is None:
            ch0.duty_ns(CH0_CENTER_DUTY)
            ch1.duty_ns(CH1_CENTER_DUTY)
            print('No message received', get_soc())
            wdt.feed()
            continue
        else:
            controller=json.loads(msg)
    except Exception as e:
        print(e)
        continue

    soc = get_soc()

    if controller[5] & 1<<6: #A
        ch0.duty_ns(joystick_to_servo(controller[2], CH0_MIN_SLOWEST, CH0_MAX_SLOWEST, CH0_CENTER_DUTY))
    elif controller[5] & 1<<5: #B
        ch0.duty_ns(joystick_to_servo(controller[2], CH0_MIN_FASTEST, CH0_MAX_FASTEST, CH0_CENTER_DUTY))
    else:
        ch0.duty_ns(joystick_to_servo(controller[2], CH0_MIN_DUTY, CH0_MAX_DUTY, CH0_CENTER_DUTY))

    ch1.duty_ns(joystick_to_servo(controller[1], CH1_MIN_DUTY, CH1_MAX_DUTY, CH1_CENTER_DUTY))
    print(host, controller, now.peers_table[host])
    wdt.feed()
    buf = {'rssi': now.peers_table[host], 'soc': soc}
    try:
        now.send(peer, json.dumps(buf))
    except OSError as e:
        print(e)
        continue
