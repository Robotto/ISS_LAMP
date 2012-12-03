#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import socket
from IPy import IP
incomingPort = 1337
remotePort = 1337
MESSAGE="RESPONSE"
# A UDP server listening for packets on port 1337:

# Set up a UDP server
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

# Listen on port 1337, FTW!
# (to all IP addresses on this system)
listen_addr = ("",incomingPort)
UDPSock.bind(listen_addr)

# Report on all data packets received and
# where they came from in each case (as this is
# UDP, each may be from a different source and it's
# up to the server to sort this out!)
while True:
        data,addr = UDPSock.recvfrom(1024)
	remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string
        #print data.strip(),addr
	print data.strip()
	print remoteIP
	if (data.strip() == 'respond'):
		UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
