#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import mechanize
from BeautifulSoup import BeautifulSoup
from datetime import datetime, date, timedelta
from time import strftime, strptime, mktime, struct_time, time, ctime, localtime
from getopt import getopt
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

# Personalization.
latitude = 56.156361
longtitude = 10.188631
elevation = 40		#meters above sea level

#OHM:
#latitude = 52.6925433
#longtitude = 4.7553544


def get_visible_passes():
	VisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)

	br = mechanize.Browser()
	br.set_handle_robots(False)
	# Get the ISS PASSES pages:

	print 'Retrieving list of visible passes'

	visibleHtml = br.open(VisibleURL).read()

	return(visibleHtml)

def get_all_passes():
	AllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
	
	br = mechanize.Browser()
	br.set_handle_robots(False)
	# Get the ISS PASSES pages:

	print 'Retrieving list of all passes'

	visibleHtml = br.open(AllURL).read()

	return(allHtml)

def html_to_rows(html):

	print 'Parsing HTML into data rows...' 

  	# In the past, Beautiful Soup hasn't been able to parse the Heavens Above HTML.
  	# To get around this problem, we extract just the table of ISS data and set
  	# it in a well-formed HTML skeleton. If there is no table of ISS data, create
  	# an empty table.
  	try:
		Table = html.split(r'<table class="standardTable"', 1)[1] #split after first "standard table" tag, return 2nd portion
		Table = Table.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion
		Table = Table.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion , again.
		Table = Table.split(r'</tr>', 1)[1] #split after first "</tr>" tag return second portion
		Table = Table.split(r'</table>', 1)[0] #split after first "</table>" return first portion

	except IndexError:
    	Table = '<tr><td></td></tr>'

	newHtml = '''<html>
	<head>
	</head>
	<body>
	<table>
	%s
	</table>
	</body>
	</html>''' % Table
	
	# Parse the newly created HTML.
	Soup = BeautifulSoup(newHtml)
	  
	#Collect only the data rows of the table.
	
	Rows = Soup.findAll('table')[0].findAll('tr')[0:]
	#print 'The parsed rows:'
	#print Rows
	#print 
	
	return (Rows)


def check_data():
	#verifying current pass data 	


	#if fail return -1

	return(verification)




# Report on all data packets received and
# where they came from in each case (as this is
# UDP, each may be from a different source and it's
# up to the server to sort this out!)

print 'Started @ %s' %(ctime())

visiblehtml = get_visible_passes()
visibleRows = html_to_rows(visiblehtml)

allhtml = get_all_passes()
allRows = html_to_rows(allhtml)


while True:
	data,addr = UDPSock.recvfrom(1024)
    remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string
	#print data.strip(),addr

	print '  RX: "%s" @ %s from %s' % (data.rstrip('\n'), ctime(), remoteIP) 
    	if (data.strip() == 'iss?'):
			try:

				currenttime = int(time())

				#check age of data

				#brne -> get new data

				#parse data

				MESSAGE=GetNextPassFromRows(allRows,visibleRows)
	
			except:
				MESSAGE='fail at this end, sorry'			
		
		UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
		print '  TX: %s' % (MESSAGE)



