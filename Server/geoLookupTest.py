
ip='87.62.101.85'
from ip2geotools.databases.noncommercial import DbIpCity
response = DbIpCity.get(ip, api_key='free')
print(f"city: {response.city}")
print(f"lat,lon: {response.latitude},{response.longitude}")
