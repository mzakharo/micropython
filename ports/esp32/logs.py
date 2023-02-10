import paho.mqtt.client as mqtt
client = mqtt.Client()
client.connect('nas.local');


def on_connect(client, a, b, c):
    mytopic = 'tubby/logs'
    client.subscribe(mytopic)
def on_message(client, userdata, message,tmp=None):
    print(message.payload.decode())
client.on_message = on_message;
client.on_connect = on_connect

client.loop_forever()
