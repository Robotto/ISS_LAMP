import logging
import socket
from time import ctime
from IPy import IP
from HeavensAboveDataStore import locationSpecificISSpassStorage
from ip2geotools.databases.noncommercial import DbIpCity


class IssDataServer:


    def __init__(self):
        self.incomingPort = 1337
        self.remotePort = 1337
        # A UDP server listening for packets on port 1337:
        self.UDPSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.listen_addr = ("", self.incomingPort)
        self.UDPSock.bind(self.listen_addr)
        logging.basicConfig(filename='ISS.log', level=logging.DEBUG)

        print(f'Started @ {ctime()}')
        logging.info(f'STARTUP:{ctime()}: Started')

        print(f'Ready and waiting for inbound on port: {self.incomingPort}')
        logging.info(f'STARTUP:Listening on port: {self.incomingPort}')

        self.datastore={}

        while True:
            # Report on all data packets received and
            # where they came from in each case (as this is UDP, each may be from a different source and it's up to the server to sort this out!)

            # recvfrom waits for incoming data:
            data, addr = self.UDPSock.recvfrom(1024)

            remoteIP = IP(addr[0]).strNormal()  # convert address of packet origin to string

            logging.info(f'INCOMING:{ctime()}: RX: \"{data.rstrip()}\" from  {remoteIP}')


            if (data.rstrip() == b'iss?'):

                    self.prune() #Get rid of old data.

                    lat,lon = IssDataServer.getLatLonFromIP(remoteIP)

                    # Round down to 2 decimals so locations within a 1100ish meter radius can share a datastore.
                    # https://en.wikipedia.org/wiki/Decimal_degrees
                    # https://xkcd.com/2170/
                    key=f"{lat:.2f},{lon:.2f}"

                    print()
                    print(f' RX: "{data.strip()}" @ {ctime()} from {remoteIP}')
                    print(f' Coordinates: {lat},{lon}, key={key}')

                    if not key in self.datastore:
                        print(f"Client location ({key}) not in datastore.. retreiving...")
                        self.datastore[key] = locationSpecificISSpassStorage(lat,lon)
                        print(f"Done! datastore now contains: {self.datastore.keys()}")

                    #Get next pass from the datastore for client location (key):
                    nextPass = self.datastore[key].getNextPass()
                    print(f"nextPass: {nextPass}")

                    if nextPass is not False:
                        #Also: check if DST is active at that location,
                        #construct the message for the client.
                        PAYLOAD = nextPass.constructMessage(self.datastore[key].isDstAtClientLocation())

                    else:
                        PAYLOAD = 'fail at this end, sorry'
                        logging.error(f'something went wrong... nextPass does not exist for {key}...perhaps quarantine?')

                    self.UDPSock.sendto(PAYLOAD.encode('ASCII'), (remoteIP, self.remotePort))


                    logging.info(f'TX:{PAYLOAD}'.encode('ASCII'))
                    logging.info('--------------------------------')

                    print(f'TX: {PAYLOAD}')
                    print('--------------------------------')
                    print()

    @staticmethod
    def getLatLonFromIP(ipv4):
        # https://iplocation.io/
        # https://db-ip.com/<IPV4>
        # https://pypi.org/project/ip2geotools/
        #usage: IssPassUtil.getLatLonFromIP(ipv4)

        #MAGIC NUMBEEEERS!
        if '87.62.101.85' in ipv4:
            print("Oh hi Mark!")
            return 56.16097,10.20394

        response = DbIpCity.get(ipv4, api_key='free')
        return response.latitude,response.longitude

    def prune(self):
        #logging.info(f"Pruning!")
        for latLonKey in self.datastore.keys(): #Run through all keys in datastore
            if self.datastore[latLonKey].isStale(): #if the location specific datastore hasn't been used in 7 days...
                logging.info(f"{latLonKey} localised datastore is stale! Pruning...")
                self.datastore.pop(latLonKey) #remove from store
        #logging.info(f"Done Pruning!")


IssDataServer()




