import controller,time
from tftlcd import LCD15
import sys

import machine
import network
import espnow
import json

gamepad = controller.CONTROLLER()
d = LCD15(portrait=1) 

WHITE = (255,255,255)
BLACK = (0,0,0)
RED = (255,0,0)
BLUE = (0,0,255)

d.fill(WHITE)
bg = WHITE

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

ADC_PIN = 2
adc = machine.ADC(machine.Pin(ADC_PIN))
adc.atten(machine.ADC.ATTN_11DB)

R1 = 40200.0        # Resistor R1 value (40.2K ohms) - use float for precision
R2 = 10000.0        # Resistor R2 value (10K ohms) - use float for precision


VOLTAGE_MULTIPLIER = (R1 + R2) / R2

# A WLAN interface must be active to send()/recv()
sta = network.WLAN(network.WLAN.IF_STA)  # Or network.WLAN.IF_AP
sta.active(True)
sta.config(channel=6)    # Change to the channel used by the proxy above.

now = espnow.ESPNow()
now.active(True)
now.config(rate=0x29) # 0x29 LORA 250KBps
peer = b'\xbb\xbb\xbb\xbb\xbb\xbb'   # MAC address of peer's wifi interface
now.add_peer(peer)      # Must add_peer() before send()


#相关图形显示，用于展示动作
""" d.drawCircle(60,115,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(60,175,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(30,145,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(90,145,15,color=BLACK,border=2,fillcolor=WHITE)

d.drawCircle(180,20,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(180,80,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(150,50,15,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(210,50,15,color=BLACK,border=2,fillcolor=WHITE)

d.drawCircle(30,85,10,color=BLACK,border=2,fillcolor=WHITE)
d.drawCircle(210,105,10,color=BLACK,border=2,fillcolor=WHITE)

d.drawRect(90, 90, 25, 15, color=BLACK, border=2, fillcolor=WHITE)
d.drawRect(125, 90, 25, 15, color=BLACK, border=2, fillcolor=WHITE)
 """
while True:    
    v = gamepad.read()
    try:
        now.send(peer, json.dumps(v))
        host, msg = now.recv(50)
        if msg:
            msg = json.loads(msg)
        else:
            msg = [0,0]
    except Exception as e:
        sys.print_exception(e)
        time.sleep_ms(20)
        continue
    
    vbat = adc.read_uv() / 1e6 * VOLTAGE_MULTIPLIER
    soc = estimate_soc_poly(vbat)
    val = now.peers_table[host] if host is not None else [0,0]
    print(v, msg, val, vbat, soc)

    err = False
    if (msg[0] == 0 or val[0] == 0) or soc < 15:
        err = True

    if err and bg != RED:
        d.fill(RED)
        bg = RED
    if not err and bg != WHITE:
        d.fill(WHITE)
        bg = WHITE

    d.printStr('rRSSi: '+str(msg[0])+'  ',10,45+30,color=BLACK,size=2)
    d.printStr('cRSSi: '+str(val[0])+'  ',10,45+60,color=BLACK,size=2)
    d.printStr('cBAT: '+str(soc)+'  ',10,45+90,color=BLACK,size=2)

        
    d.printStr('L-X: '+str(v[1])+'  ',10,15,color=BLACK,size=2)
    d.printStr('L-Y: '+str(v[2])+'  ',10,45,color=BLACK,size=2)
    
    #d.printStr('R-X: '+str(v[3])+'  ',130,120,color=BLACK,size=2)
    #d.printStr('R-Y: '+str(v[4])+'  ',130,150,color=BLACK,size=2)
    
    """  #按键动作判断并显示
    L = v[5]%16
    if L == 0: #上
        d.drawCircle(60,115,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(60,115,15,color=BLACK,border=2,fillcolor=WHITE)

    if L == 4: #下
        d.drawCircle(60,175,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(60,175,15,color=BLACK,border=2,fillcolor=WHITE)

    if L == 6: #左
        d.drawCircle(30,145,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(30,145,15,color=BLACK,border=2,fillcolor=WHITE)
        
    if L == 2: #右
        d.drawCircle(90,145,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(90,145,15,color=BLACK,border=2,fillcolor=WHITE)
    
    
    if v[5] & 1<<4: #Y
        d.drawCircle(180,20,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(180,20,15,color=BLACK,border=2,fillcolor=WHITE)
    
    if v[5] & 1<<6: #A
        d.drawCircle(180,80,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(180,80,15,color=BLACK,border=2,fillcolor=WHITE)
        
    if v[5] & 1<<7: #X
        d.drawCircle(150,50,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(150,50,15,color=BLACK,border=2,fillcolor=WHITE)
    
    if v[5] & 1<<5: #B
        d.drawCircle(210,50,15,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(210,50,15,color=BLACK,border=2,fillcolor=WHITE)
    
    if v[6] & 1<<4: #back
        d.drawRect(90, 90, 25, 15, color=BLACK, border=2, fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawRect(90, 90, 25, 15, color=BLACK, border=2, fillcolor=WHITE)

    if v[6] & 1<<5: #start
        d.drawRect(125, 90, 25, 15, color=BLACK, border=2, fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawRect(125, 90, 25, 15, color=BLACK, border=2, fillcolor=WHITE)
    
    if v[6] & 1<<6: #右摇杆确认键
        d.drawCircle(210,105,10,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(210,105,10,color=BLACK,border=2,fillcolor=WHITE)

    if v[6] & 1<<7: #左摇杆确认键
        d.drawCircle(30,85,10,color=BLACK,border=2,fillcolor=BLACK)
        time.sleep_ms(200)
        d.drawCircle(30,85,10,color=BLACK,border=2,fillcolor=WHITE) """
    
        
    time.sleep_ms(20)
