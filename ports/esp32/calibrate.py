import paho.mqtt.publish as publish
import sys
import modules.config as config 
import modules.secrets as secrets


if len(sys.argv) == 2:    
    SLEEP = int(sys.argv[1])
    print('publishing sleep', SLEEP)
    publish.single(f'{config.NAME}/sleep', str(SLEEP), qos=1, retain=True, hostname=secrets.MQTT_SERVER)
else:
    publish.single(f'{config.NAME}/sleep', '', qos=1, retain=True, hostname=secrets.MQTT_SERVER)
    print(f'sleep parameters cleared')