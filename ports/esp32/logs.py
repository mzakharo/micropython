import paho.mqtt.client as mqtt
import modules.config as config 

client = mqtt.Client()
client.connect(config.MQTT_SERVER);

def on_connect(client, a, b, c):
    mytopic = f'{config.NAME}/logs'
    client.subscribe(mytopic)
def on_message(client, userdata, message,tmp=None):
    print(message.payload.decode())
client.on_message = on_message;
client.on_connect = on_connect

client.loop_forever()
