#!/usr/bin/env python3
import sys

ssid = sys.argv[1]
password = sys.argv[2]
mqtt = sys.argv[3]

print(f'SSID: {ssid}  password: {password} MQTT: {mqtt}')

with open('modules/secrets.py', 'w') as f:
    f.write(f'WIFI_SSID="{ssid}"\n')
    f.write(f'WIFI_PASSWORD="{password}"\n')
    f.write(f'MQTT_SERVER="{mqtt}"\n')
