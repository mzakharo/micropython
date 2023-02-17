import pandas as pd
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS


from config import token

org = "home"
bucket = "main"
influx_client = InfluxDBClient(url="http://nas.local:8086", token=token, org=org)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)


df = pd.read_csv('log.csv', header=None, names=['time', 'mac', 'type'])
df.set_index('time', inplace=True)
df.index = pd.to_datetime(df.index, format='%Y%m%d_%H%M%S').tz_localize('US/Eastern')

points = []
for i, v in df.iterrows():
    ts = int(i.timestamp())
    mac = v['mac']
    print(i, ts,  mac)
    point = Point('sniffer') \
          .field(mac, 1) \
          .time(ts, WritePrecision.S)
    points.append(point)
write_api.write(bucket, org, points)

