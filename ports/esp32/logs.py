import paho.mqtt.client as mqtt
import modules.config as config 
import modules.secrets as secrets
import datetime
import sys

NODE_ID = sys.argv[1]

client = mqtt.Client()
client.connect(secrets.MQTT_SERVER);

def on_connect(client, a, b, c):
    mytopic = f'{config.NAME}/{NODE_ID}/logs'
    client.subscribe(mytopic)
def on_message(client, userdata, message,tmp=None):
    print(datetime.datetime.now(), message.payload.decode())
client.on_message = on_message;
client.on_connect = on_connect

client.loop_forever()
