import logging
import socket
from time import ctime, localtime
from IPy import IP

from issPassClass import IssPassUtil
from HeavensAboveDataStore import locationSpecificISSpassStorage


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
        logging.info(f'{ctime()}: Started')

        DST = localtime().tm_isdst
        if DST:
            print('Daylight savings is active')
        else:
            print('Daylight savings is inactive')

        print(f'Ready and waiting for inbound on port: {self.incomingPort}')
        logging.info(f'Listening on port: {self.incomingPort}')

        self.datastore={}

        while True:
            # Report on all data packets received and
            # where they came from in each case (as this is UDP, each may be from a different source and it's up to the server to sort this out!)

            # recvfrom waits for incoming data:
            data, addr = self.UDPSock.recvfrom(1024)

            remoteIP = IP(addr[0]).strNormal()  # convert address of packet origin to string

            logging.info(f'{ctime()}: RX: \"{data.rstrip()}\" from  {remoteIP}')


            if (data.rstrip() == b'iss?'):

                    lat,lon = IssPassUtil.getLatLonFromIP(remoteIP)
                    timezone = IssPassUtil.getTZfromLatLon(lat,lon)


                    key=f"{lat},{lon}"

                    print()
                    print(f' RX: "{data.strip()}" @ {ctime()} from {remoteIP}')
                    print(f' Coordinates: {lat},{lon}, key={key}')
                    print(f' Timezone: {timezone}')
                    print()


                    if not key in self.datastore:
                        print(f"Client location ({key}) not in datastore.. retreiving...")
                        self.datastore[key] = locationSpecificISSpassStorage(lat,lon)
                        print(f"Done! datastore now contains: {self.datastore.keys()}")


                    nextPass = self.datastore[key].getNextPass()
                    print(f"nextPass: {nextPass}")
                    dst = IssPassUtil.getClientDSTstr(timezone,nextPass.tStart)
                    MESSAGE = IssPassUtil.message(nextPass,dst)

                #    MESSAGE = 'fail at this end, sorry'
                #    logging.warning('parsing of data failed.')

                    self.UDPSock.sendto(MESSAGE.encode('ASCII'), (remoteIP, self.remotePort))
                    print()
                    print(' TX: %s' % (MESSAGE))
                    print('--------------------------------')
                    print()


IssDataServer()




