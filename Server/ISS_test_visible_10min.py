#!/usr/bin/env python
# -*- coding: UTF-8 -*-

from datetime import datetime, date, timedelta
from time import strftime, strptime, mktime, struct_time, time, ctime
#from getopt import getopt
import os, sys, envoy


import socket
from IPy import IP
incomingPort = 1337
remotePort = 1337
# A UDP server listening for packets on port 1337:
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

# Listen on port 1337, FTW!
# (to all IP addresses on this system)
listen_addr = ("",incomingPort)
UDPSock.bind(listen_addr)

# Report on all data packets received and
# where they came from in each case (as this is
# UDP, each may be from a different source and it's
# up to the server to sort this out!)

print 'Started @ %s' %(ctime())


while True:
        data,addr = UDPSock.recvfrom(1024)
        remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string
        #print data.strip(),addr
	print 'RX: %s @ %s from %s' % (data, ctime(), remoteIP) 
        if (data.strip() == 'respond'):
		currenttime = int(time())
		#start in 5 minutes and 10 seconds
		tstart = currenttime + 60*5 + 10
		#max is 10 seconds later
		tmax = tstart + 10
		#end is 10 seconds after that
		tend = tmax + 10

		MESSAGE='V\0-2.0\0%s\0SE-10\0%s\0S-22\0%s\0SW-10' % (tstart,tmax,tend)
                UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
		print 'TX: %s' % (MESSAGE)
