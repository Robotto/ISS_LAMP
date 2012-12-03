#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import mechanize
from time import strftime
from BeautifulSoup import BeautifulSoup
from datetime import datetime, date, timedelta
from time import strptime, mktime, struct_time, time
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
maxMag = 2         # show passes at least this bright - NOT USED for ALL PASSES PAGE lookup
minAlt = 0         # show passes at least this high
earliest = 0      # show passes after this hour of day

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

# Heavens Above URLs and login information.
#issAllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
#issVisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)

issAllURL = 'http://62.212.66.171/all_test_page.htm'
issVisibleURL = 'http://62.212.66.171/visible_test_page.htm'

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

#Printing:
currenttime = int(time())

print 'Current unix time: %s' % (currenttime)

#All passes:
(A_start, A_max, A_end, A_loc1, A_loc2, A_loc3, A_startUnix, A_maxUnix, A_endUnix) = parseRow(allRows[0], 0)


#Visible passes:
if visibleRows: #check for any visible passes.

	(V_start, V_max, V_end, V_loc1, V_loc2, V_loc3, V_startUnix, V_maxUnix, V_endUnix, V_mag) = parseRow(visibleRows[0], 1)
else:
	V_startUnix=A_startUnix+1 #make sure next check doesn't fail
# 	print 'DEBUG: no visible passes'
	
#check if it's the same pass and if so print the visible one!
if (V_startUnix<=A_startUnix):
#  print 'DEBUG: PASS MATCH'
  print 'Start: %s Mag: %s @ %s, Max: %s @ %s, End: %s @ %s' % (V_startUnix, V_mag, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)
else:
  print 'Start: %s @ %s, Max: %s @ %s, End: %s @ %s' % (A_startUnix, A_loc1, A_maxUnix, A_loc2, A_endUnix, A_loc3)
  


#
#for row in allRows:
#  (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix) = parseRow(row, 0)
#  if (int(loc2.split('-')[1]) >= minAlt) & (startUnix>currenttime):
##output:
#   print 'Start: %s @ %s, Max: %s @ %s, End: %s @ %s' % (startUnix, loc1, maxUnix, loc2, endUnix, loc3)
#
#for row in visibleRows:
#  (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag) = parseRow(row, 1)
#
##  if  
#
#  if (int(loc2.split('-')[1]) >= minAlt) & (startUnix>currenttime):
##output:
#    print 'Start: %s Mag: %s @ %s, Max: %s @ %s, End: %s @ %s' % (startUnix, mag, loc1, maxUnix, loc2, endUnix, loc3)


