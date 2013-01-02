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

options: -u : print ISS info with unix timecodes instead of human timecodes
         -h : help; print this message and quit
'''

# Personalization.
latitude = 56.156361
longtitude = 10.188631
elevation = 40		#meters above sea level
maxMag = 0         # show passes at least this bright - NOT USED for ALL PASSES PAGE lookup
minAlt = 0         # show passes at least this high
earliest = 0      # show passes after this hour of day

# Parse a row of Heavens Above data and return the start date (datetime),
# the beginning, peak, and end sky positions
# (strings).
def parseRow(row):
  cols = row.findAll('td')
  dStr = cols[0].a.string

  t1Str = ':'.join(cols[2].string.split(':'))
#  print 't1str: %s' %t1Str
  t2Str = ':'.join(cols[5].string.split(':'))
#  print 't2str: %s' %t2Str
  t3Str = ':'.join(cols[8].string.split(':'))
#  print 't3str: %s' %t3Str

  #intensity = float(cols[1].string)
  alt1 = cols[3].string.replace(u'\xB0', '')
  print 'alt1: %s' %alt1


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

  return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix)


# Parse command line options.
justPrint = True
optlist, args = getopt(sys.argv[1:], 'uh')
for o, a in optlist:
  if o == '-u':
    justPrint = False
  else:
    print usage
    sys.exit()

# Heavens Above URLs and login information.
issURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)

# Create virtual browser and get page.
br = mechanize.Browser()
br.set_handle_robots(False)

# Get the ISS ALL PASSES page.
iHtml = br.open(issURL).read()

# In the past, Beautiful Soup hasn't been able to parse the Heavens Above HTML.
# To get around this problem, we extract just the table of ISS data and set
# it in a well-formed HTML skeleton. If there is no table of ISS data, create
# an empty table.
try:
    table = iHtml.split(r'<tr class="clickableRow ', 1)[1]
    table = table.split(r'>', 1)[1]
    table = table.split(r'</table>', 1)[0]

except IndexError:
    table = '<tr><td></td></tr>'

html = '''<html>
<head>
</head>
<body>
<table>
%s
</table>
</body>
</html>''' % table

# Parse the HTML.
soup = BeautifulSoup(html)

# Collect only the data rows of the table.
rows = soup.findAll('table')[0].findAll('tr')[2:]

# Go through the data rows, adding only bright, high events within
# the next week to my "home" calendar.
print 'Current unix time: %s' % (int(time()))
for row in rows:
  (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix) = parseRow(row)

  if int(loc2.split('-')[1]) >= minAlt:

   if justPrint:
	print '%s: Start: %s @ %s, Max: %s @ %s, End: %s @ %s' %\
  (start.strftime('%b %d'), start.strftime('%H:%M:%S'), loc1, max.strftime('%H:%M:%S'), loc2, end.strftime('%H:%M:%S'), loc3)
   else:	
	struct_time
	print 'Start: %s @ %s, Max: %s @ %s, End: %s @ %s' %\
  (startUnix, loc1, maxUnix, loc2, endUnix, loc3)


