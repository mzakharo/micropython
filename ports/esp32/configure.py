#!/usr/bin/env python3
import sys


ssid = sys.argv[1]
password = sys.argv[2]

print(f'SSID: {ssid}  password: {password}')

with open('modules/secrets.py', 'w') as f:
    f.write(f'WIFI_SSID="{ssid}"\n')
    f.write(f'WIFI_PASSWORD="{password}"\n')
