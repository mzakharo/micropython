import network
import time
import webrepl

def do_connect():
    # WiFi SSID and Password
    wifi_ssid = "wireless"
    wifi_password = ""

    # Wireless config : Station mode
    station = network.WLAN(network.STA_IF)
    station.active(True)
    if not station.isconnected():
        # Try to connect to WiFi access point
        print("Connecting...")
        station.connect(wifi_ssid, wifi_password)
 
do_connect()
station = network.WLAN(network.STA_IF)
while not station.isconnected():
    print("Connecting...")
    time.sleep(1)

# Display connection details
print("Connected!")
print("My IP Address:", station.ifconfig()[0])
webrepl.start()

