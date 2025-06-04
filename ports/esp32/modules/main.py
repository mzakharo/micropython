from machine import WDT
wdt = WDT(timeout=60000)
wdt.feed()
import network
import time
import webrepl
import camera
from machine import Pin, reset
from umqtt.robust import MQTTClient


# Camera pins for ESP32-CAM (AI-Thinker module)
# These are common pin assignments, verify with your specific board
CAMERA_PINS = {
    'LED_FLASH': 4, # Flash LED pin
}

# MQTT Broker details
MQTT_BROKER = "nas.local" # e.g., "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/cam/image"
MQTT_CLIENT_ID = "esp32_cam_client"

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
 
def init_camera():
    print("Initializing camera...")
    camera.init(0, format=camera.JPEG, fb_location=camera.PSRAM, xclk_freq=camera.XCLK_10MHz, framesize=camera.FRAME_HD)

    ## Other settings:
    # flip up side down
    #camera.flip(1)
    # left / right
    #camera.mirror(1)

    # special effects
    #camera.speffect(camera.EFFECT_NONE)
    # The options are the following:
    # EFFECT_NONE (default) EFFECT_NEG EFFECT_BW EFFECT_RED EFFECT_GREEN EFFECT_BLUE EFFECT_RETRO

    # white balance
    #camera.whitebalance(camera.WB_NONE)
    # The options are the following:
    # WB_NONE (default) WB_SUNNY WB_CLOUDY WB_OFFICE WB_HOME

    # saturation
    #camera.saturation(0)
    # -2,2 (default 0). -2 grayscale 

    # brightness
    #camera.brightness(0)
    # -2,2 (default 0). 2 brightness

    # contrast
    #camera.contrast(0)
    #-2,2 (default 0). 2 highcontrast

    # Set quality
    #camera.quality(12) # 0-63, lower is higher quality

    print("Camera initialized.")

def enable_flash(state):
    flash_led = Pin(CAMERA_PINS['LED_FLASH'], Pin.OUT)
    flash_led.value(state)
    print(f"Flash LED {'ON' if state else 'OFF'}")

def main():
    do_connect()
    wdt.feed()
    
    try:
        init_camera()
    except Exception as e:
        print(e)
        reset()

    station = network.WLAN(network.STA_IF)
    while not station.isconnected():
        print("Connecting...")
        time.sleep(1)
    wdt.feed()

    # Display connection details
    print("Connected!")
    print("My IP Address:", station.ifconfig()[0])
    webrepl.start()

    # Connect to MQTT broker
    client = MQTTClient(MQTT_CLIENT_ID, MQTT_BROKER, port=MQTT_PORT)
    client.connect()
    print("Connected to MQTT broker.")

    while True:
        wdt.feed()

        # Enable flash
        enable_flash(1)
        
        print("Waiting 10 seconds before taking picture...")
        time.sleep(10)

        wdt.feed()
        buf = camera.capture()
        buf = camera.capture()

        # Disable flash after taking picture
        enable_flash(0)
        wdt.feed()
        if buf is not False:
            print(f"Picture taken, size: {len(buf)} bytes")

            # Publish the image
            client.publish(MQTT_TOPIC, buf, qos=0)
            print(f"Image published to topic: {MQTT_TOPIC}")

        wdt.feed()
        time.sleep(10)

    camera.deinit()

    #wait before restarting
    client.disconnect()
    print("Disconnected from MQTT broker.")

    #disconnect from Wi-Fi
    station = network.WLAN(network.STA_IF)
    station.active(False)

    reset()  


if __name__ == "__main__":
    main()
