import time

from dateutil import parser, tz
import datetime
print(time.time())
ip='87.62.101.85'
from ip2geotools.databases.noncommercial import DbIpCity
response = DbIpCity.get(ip, api_key='free')
print(f"city: {response.city}")
print(f"lat,lon: {response.latitude},{response.longitude}")

#https://koalatea.io/timezone-from-location/

#from timezonefinder import TimezoneFinder
#tf = TimezoneFinder()
from timezonefinder import TimezoneFinderL
tf = TimezoneFinderL(in_memory=True)  # reuse

timezone = tf.timezone_at(lng=response.longitude, lat=response.latitude)
print(f"TZ: {timezone}")

#isdst_now_in = lambda zonename: bool(datetime.datetime.now(tz.gettz(zonename)).dst())


#print(isdst_now_in(timezone))
#Now: 1664199730
#Not DST: 1669466524
print(int(datetime.datetime.fromtimestamp(1664199730,tz.gettz(timezone)).dst().seconds/3600))
print(int(datetime.datetime.fromtimestamp(1669466524,tz.gettz(timezone)).dst().seconds/3600))