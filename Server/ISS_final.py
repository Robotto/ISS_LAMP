#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import mechanize
from BeautifulSoup import BeautifulSoup
from datetime import datetime, date, timedelta
from time import strftime, strptime, mktime, struct_time, time, ctime, localtime
from getopt import getopt
import os, sys, envoy


# Personalization.
latitude = 56.156361
longtitude = 10.188631
elevation = 40		#meters above sea level

#OHM:
#latitude = 52.6925433
#longtitude = 4.7553544

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



#http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
#http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
def ISS_PASS_GET():
  # Heavens Above URLs and login information.
  issAllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
  issVisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)

#TEST PAGES:
#  issAllURL = 'http://127.0.0.1/all_test_page.htm'
#  issVisibleURL = 'http://127.0.0.1/visible_test_page.htm'

  # Create virtual browser and get page.
  br = mechanize.Browser()
  br.set_handle_robots(False)

  # Get the ISS PASSES pages:
  print 'Retriveving list of all passes..'
  allHtml = br.open(issAllURL).read()
  print 'Retrieving list of visble passes specifically..'
  visibleHtml = br.open(issVisibleURL).read()


  print 'Parsing HTML into data rows...' 

  # In the past, Beautiful Soup hasn't been able to parse the Heavens Above HTML.
  # To get around this problem, we extract just the table of ISS data and set
  # it in a well-formed HTML skeleton. If there is no table of ISS data, create
  # an empty table.
  try:

      #allTable = allHtml.split(r'<tr class="clickableRow ', 1)[1] #split after first "clickable row" tag, return 2nd portion
      #allTable = allTable.split(r'>', 1)[1] #split after first ">" return second portion
      #allTable = allTable.split(r'</table>', 1)[0] #split after first "</table>" return first portion

      allTable = allHtml.split(r'<table class="standardTable"', 1)[1] #split after first "standard table" tag, return 2nd portion
      allTable = allTable.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion
      allTable = allTable.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion , again.
      allTable = allTable.split(r'</tr>', 1)[1] #split after first "</tr>" tag return second portion
      allTable = allTable.split(r'</table>', 1)[0] #split after first "</table>" return first portion

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
      #visibleTable = visibleHtml.split(r'<tr class="clickableRow ', 1)[1]
      #visibleTable = visibleTable.split(r'>', 1)[1]
      #visibleTable = visibleTable.split(r'</table>', 1)[0]
      
      visibleTable = visibleHtml.split(r'<table class="standardTable"', 1)[1] #split after first "standard table" tag, return 2nd portion
      visibleTable = visibleTable.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion
      visibleTable = visibleTable.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion , again.
      visibleTable = visibleTable.split(r'</tr>', 1)[1] #split after first "</tr>" tag return second portion
      visibleTable = visibleTable.split(r'</table>', 1)[0] #split after first "</table>" return first portion

      
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
  #allRows = allSoup.findAll('table')[0].findAll('tr')[0:]
  #visibleRows = visibleSoup.findAll('table')[0].findAll('tr')[0:]

  allRows = allSoup.findAll('table')[0].findAll('tr')[0:]
  visibleRows = visibleSoup.findAll('table')[0].findAll('tr')[0:]

  #print
  #print 'all:'
  #print allRows
  #print
  #print 'visible:'
  #print visibleRows
  #print
  #print

  #Printing:
  currenttime = int(time())

  #DEBUG: mathced with test pages so that next pass is visible and occcurs in 19 seconds:
  #currenttime =  1376339150

  print 'DEBUG: Current unix time: %s' % (currenttime)


  #set up som startup values for the pass starttimes:
  A_startUnix=0  
  V_startUnix=0  
  
  print 'parsing rows'

  #get data for the next future regular pass:
  All_rowCount=0
  while A_startUnix<currenttime: 
    try: (A_start, A_max, A_end, A_loc1, A_loc2, A_loc3, A_startUnix, A_maxUnix, A_endUnix) = parseRow(allRows[All_rowCount], 0)
    except: A_startUnix=currenttime 

    All_rowCount+=1

    print 'Checked pass no. %s for past timecode: %s' % (All_rowCount, A_startUnix)
  
  
  #get data for the next visible pass:
  Visible_rowCount=0
  while V_startUnix<currenttime:
    try: (V_start, V_max, V_end, V_loc1, V_loc2, V_loc3, V_startUnix, V_maxUnix, V_endUnix, V_mag) = parseRow(visibleRows[Visible_rowCount], 1)
    except: V_startUnix=currenttime #make sure next check doesn't fail 

    Visible_rowCount+=1

    print 'Checked visible pass no. %s for past timecode: %s' % (Visible_rowCount, V_startUnix)
  
	
  print 'The next pass of the ISS above %s, %s is:' % (latitude, longtitude)

  #check if it's the same pass and if so print the visible one!
  #Regular (non visible) passes always start at 10degrees above the horizon, and since visible passes are a subset of regular passes,
  #A visible pass will sometimes have a later timecode in the 'visible table' than in the 'all table', since a visible pass might start higher in the sky,
  #and therefore later. Even though it's the same pass in the two tables, the non visible one would take precedence, since it would have an earlier timecode
  #luckily, we know for a fact that there is no less than ~1½ hours between passes, which gives us plenty of wiggle room to determine whether the two passes
  #are actually the same.. 
  #therefore we check to see whether a visible pass starts up to 10 minutes (600 seconds) after a regular pass, and if so: Give the visual pass precedence

  #also: if no visible passes are in the data from HA. V_startUnix is set to curenttime, in order to end the while loop, this needs to be checked for:
  if (A_startUnix+600>V_startUnix & V_startUnix!=currenttime):
    print 'Visible pass no. %s, which is %s seconds in the future @ %s' % (Visible_rowCount, V_startUnix-currenttime, V_start.strftime('%d/%m %H:%M:%S'))
    return 'V\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)
#     return 'VISIBLE'
  else:
    print 'Regular pass no. %s, which is %s seconds in the future @ %s' % (All_rowCount, A_startUnix-currenttime, A_start.strftime('%d/%m %H:%M:%S'))
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
        print
	print '  RX: "%s" @ %s from %s' % (data.rstrip('\n'), ctime(), remoteIP) 
        if (data.strip() == 'iss?'):
		MESSAGE=ISS_PASS_GET()
                UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
		print '  TX: %s' % (MESSAGE)

	elif (data.strip() == 'dst?'):
		MESSAGE='%d' % (localtime().tm_isdst)
		UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
		print '  TX: %s' % (MESSAGE)
