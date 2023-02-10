import paho.mqtt.publish as publish
import json

name = 'tubby'
model = 'um_feathers2'
manufacturer = 'oh boy'
uversion = 'fake me'

t = f'homeassistant/sensor/{name}/{name}_battery/config'
m = {'device_class' : 'voltage',
    'unit_of_measurement': 'mV',
    'name': f'{name} Battery',
    'state_topic': f'{name}/profiling',
    'unique_id' : f'ESPsensor{name}_battery',
    'device' : {
                    'identifiers' : 0,
                    'name' : name,
                    'sw_version' : uversion,
                    'model' : model,
                    'manufacturer' : manufacturer,
                },
    'value_template' : "{{ value_json.vbat }}",
    'force_update' : True,
    'expire_after' : 10800,
    }
publish.single(t, json.dumps(m), qos=1, retain=True, hostname='nas.local')

t = f'homeassistant/sensor/{name}/{name}_orp/config'
m = {'device_class' : 'voltage',
    'unit_of_measurement': 'mV',
    'name': f'{name} ORP',
    'state_topic': f'{name}/profiling',
    'unique_id' : f'ESPsensor{name}_orp',
    'device' : {
                    'identifiers': 0,
                    'name' : name,
                    'sw_version' : uversion,
                    'model' : model,
                    'manufacturer' : manufacturer,
                },
    'value_template' : "{{ value_json.orp }}",
    'force_update' : True,
    'expire_after' : 10800,
    }
publish.single(t, json.dumps(m), qos=1, retain=True, hostname='nas.local')

