import paho.mqtt.client as mqtt
import modules.config as config 
import modules.secrets as secrets
import sys
import pandas as pd
import json
import time

client = mqtt.Client()
client.connect(secrets.MQTT_SERVER);

def on_connect(client, a, b, c):
    mytopic = f'{config.NAME}/+/status'
    client.subscribe(mytopic)
def on_message(client, userdata, message,tmp=None):
    msg = json.loads(message.payload)
    if '_raw' in msg:
        now = time.strftime("%Y%m%d_%H%M%S")
        fname = f'cal_{now}.csv'
        df = pd.DataFrame(msg['_raw'])
        df.to_csv(fname)
        print(fname)
        print(df)
    else:
        print(msg)
client.on_message = on_message;
client.on_connect = on_connect

client.loop_forever()
