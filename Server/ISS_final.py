#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import mechanize
from BeautifulSoup import BeautifulSoup
from datetime import datetime, date, timedelta
from time import strftime, strptime, mktime, struct_time, time, ctime
from getopt import getopt
import os, sys, envoy

usage = '''usage: iss-calendar [-uh]

options: -H : print ISS info, with human readable dates instead of unix epoch timecodes - DISABLED
         -h : help; print this message and quit
'''

# Personalization.
latitude = 56.156361
longtitude = 10.188631
elevation = 40		#meters above sea level
#maxMag = 2         # show passes at least this bright - NOT USED for ALL PASSES PAGE lookup
#minAlt = 0         # show passes at least this high
#earliest = 0      # show passes after this hour of day

# Parse a row of Heavens Above data and return the start date (datetime),
# the beginning, peak, and end sky positions
# (strings).
def parseRow(row, isvisible):
  cols = row.findAll('td')
  dStr = cols[0].a.string
  if isvisible:
    mag = float(cols[1].string)
    t1Str = ':'.join(cols[2].string.split(':'))
    t2Str = ':'.join(cols[5].string.split(':'))
    t3Str = ':'.join(cols[8].string.split(':'))
    alt1 = cols[3].string.replace(u'\xB0', '')
    az1 = cols[4].string
    alt2 = cols[6].string.replace(u'\xB0', '')
    az2 = cols[7].string
    alt3 = cols[9].string.replace(u'\xB0', '')
    az3 = cols[10].string
  else:
    t1Str = ':'.join(cols[1].string.split(':'))
    t2Str = ':'.join(cols[4].string.split(':'))
    t3Str = ':'.join(cols[7].string.split(':'))
    alt1 = cols[2].string.replace(u'\xB0', '')
    az1 = cols[3].string
    alt2 = cols[5].string.replace(u'\xB0', '')
    az2 = cols[6].string
    alt3 = cols[8].string.replace(u'\xB0', '')
    az3 = cols[9].string

  loc1 = '%s-%s' % (az1, alt1)
  loc2 = '%s-%s' % (az2, alt2)
  loc3 = '%s-%s' % (az3, alt3)

  startStr = '%s %s %s' % (dStr, date.today().year, t1Str)
  start = datetime(*strptime(startStr, '%d %b %Y %H:%M:%S')[0:7])
  startUnix = int(mktime(strptime(startStr, '%d %b %Y %H:%M:%S')))

 #print("Starttime unix string: %s") % (startUnix)

  maxStr = '%s %s %s'  % (dStr, date.today().year, t2Str)
  max = datetime(*strptime(maxStr, '%d %b %Y %H:%M:%S')[0:7])
  maxUnix = int(mktime(strptime(maxStr, '%d %b %Y %H:%M:%S')))
  
 #print("Maxtime unix string: %s") % (maxUnix)

  endStr = '%s %s %s' % (dStr, date.today().year, t3Str)
  end = datetime(*strptime(endStr, '%d %b %Y %H:%M:%S')[0:7])
  endUnix = int(mktime(strptime(endStr, '%d %b %Y %H:%M:%S')))
  
 #print("Endtime unix string: %s") % (endUnix)
  if isvisible:
    return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag)
  else:
    return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix)


# Parse command line options.
HumanPrint = False
optlist, args = getopt(sys.argv[1:], 'Hh')
for o, a in optlist:
  if o == '-H':
    HumanPrint = True
  else:
    print usage
    sys.exit()

#http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
#http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
def ISS_PASS_GET():
  # Heavens Above URLs and login information.
  issAllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
  issVisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)

  #issAllURL = 'http://127.0.0.1/all_test_page.htm'
  #issVisibleURL = 'http://127.0.0.1/visible_test_page.htm'

  # Create virtual browser and get page.
  br = mechanize.Browser()
  br.set_handle_robots(False)

  # Get the ISS PASSES pages.
  allHtml = br.open(issAllURL).read()
  visibleHtml = br.open(issVisibleURL).read()

  # In the past, Beautiful Soup hasn't been able to parse the Heavens Above HTML.
  # To get around this problem, we extract just the table of ISS data and set
  # it in a well-formed HTML skeleton. If there is no table of ISS data, create
  # an empty table.
  try:
      allTable = allHtml.split(r'<table id="ctl00_ContentPlaceHolder1_tblPasses"', 1)[1]
      allTable = allTable.split(r'>', 1)[1]
      allTable = allTable.split(r'</table>', 1)[0]
  except IndexError:
      allTable = '<tr><td></td></tr>'

  allHtml = '''<html>
  <head>
  </head>
  <body>
  <table>
  %s
  </table>
  </body>
  </html>''' % allTable

  try:
      visibleTable = visibleHtml.split(r'<table id="ctl00_ContentPlaceHolder1_tblPasses"', 1)[1]
      visibleTable = visibleTable.split(r'>', 1)[1]
      visibleTable = visibleTable.split(r'</table>', 1)[0]
  except IndexError:
      visibleTable = '<tr><td></td></tr>'

  visibleHtml = '''<html>
  <head>
  </head>
  <body>
  <table>
  %s
  </table>
  </body>
  </html>''' % visibleTable

  # Parse the newly created HTML.
  allSoup = BeautifulSoup(allHtml)
  visibleSoup = BeautifulSoup(visibleHtml)

  # Collect only the data rows of the table.
  allRows = allSoup.findAll('table')[0].findAll('tr')[2:]
  visibleRows = visibleSoup.findAll('table')[0].findAll('tr')[2:]


#  print allRows[1]

  #Printing:
  currenttime = int(time())

  #print 'DEBUG: Current unix time: %s' % (currenttime)

  rowCount=0


  A_startUnix=0  
  
#keep reading rows until a future pass is found: (HA.com sometimes has past passes in the table.)
  while A_startUnix<currenttime:

  #All passes:
    try: (A_start, A_max, A_end, A_loc1, A_loc2, A_loc3, A_startUnix, A_maxUnix, A_endUnix) = parseRow(allRows[rowCount], 0)
    except: A_startUnix=currenttime+1 
            A_start=0
            A_max=0
            A_end=0
            A_loc1=0
            A_loc2=0
            A_loc3=0
            A_startUnix=0
            A_maxUnix=0
            A_endUnix=0
    #ADD SOME DEFAULT VALUES FOR NO PASSES!!! 

    #check for any visible passes.
    try: (V_start, V_max, V_end, V_loc1, V_loc2, V_loc3, V_startUnix, V_maxUnix, V_endUnix, V_mag) = parseRow(visibleRows[rowCount], 1)
    except: V_startUnix=A_startUnix+1 #make sure next check doesn't fail 

    rowCount+=1

    print 'Checking pass no. %s for past timecode' % (rowCount)

  print 'Pass no. %s is %s seconds in the future' % (rowCount, A_startUnix-currenttime)
	
  #check if it's the same pass and if so print the visible one!
  if (V_startUnix<=A_startUnix):
  #  print 'DEBUG: PASS MATCH'
    return 'V\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)
#     return 'VISIBLE'
  else:
    return 'R\0%s\0%s\0%s\0%s\0%s\0%s' % (A_startUnix, A_loc1, A_maxUnix, A_loc2, A_endUnix, A_loc3)
#     return 'REGULAR'



#UDP HALLØJ:  

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
		MESSAGE=ISS_PASS_GET()
                UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
		print 'TX: %s' % (MESSAGE)
