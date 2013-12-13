#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#                                                                                 /------------------------------\
#                                                                                   |                              |
#          Get page -> pull data -> check validity -- valid: -> save it -> WAIT FOR REQUEST-> respond to request ->/
#(check for quarantine)                  |                                        |
#                                        |                                        |
#                                        bad:                                     while <-------------------------<^<-----------\
#                                        |                                        |                                ^             |
#                                        quarantine for 24 hrs                    check timestamp of next pass --> in_future     |
#                                        |                                                   |                                   |
#                                        |                                                   |                                   |
#                                        restart                                             in_past                             |
#                                                                                            |                                   |
#                                                                                            |                                   /
#                                                                                            parse next pass data -> good ---> --
#                                                                                                        |
#                                                                                                        |
#                                                                                                        index err: out of future data --> restart
#  

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
elevation = 40    #meters above sea level

#OHM:
#latitude = 52.6925433
#longtitude = 4.7553544

def refresh_passes(isvisible):
  html = get_html(isvisible)
  rows = html_to_rows(html)
  passes = rows_to_sets(rows)
  return (passes)


def get_html(isvisible):
  #http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
  #http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=56.156361&lng=10.188631&alt=40&tz=CET
  #VisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
  #AllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=%s&tz=CET' %(latitude, longtitude, elevation)
  VisibleURL = 'http://62.212.66.171/iss/visible.htm'
  AllURL = 'http://62.212.66.171/iss/regular.htm'

  br = mechanize.Browser()
  br.set_handle_robots(False)
  # Get the ISS PASSES pages:

  print 'Retrieving list of passes'

  if isvisible:
    Html = br.open(VisibleURL).read()
  else:
    Html = br.open(AllURL).read()

  return(Html)


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

def rows_to_sets(Rows):  #calls the rowparser for all the available rows, returns a set of passes.
  index = 0
  for row in Rows:
    (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag) = rowparser(row)
      ##insert age check here?
    passes[index]=[start,max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag]
    index +=1
  return (passes)

def agechecker(passes): #checks the age of the passes
  for isspass in passes:
    if (isspass[0]<currenttime):
      passes.remove(isspass)
  return (passes)


def rowparser(row):

  cols = row.findAll('td')
  dStr = cols[0].a.string

  try:
    mag = float(cols[1].string)
  except:
    mag = 0

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

  #if isvisible:
  #  return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag)
  #else:
  #  return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, )

  return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag)



print 'Started @ %s' %(ctime())

#currenttime = int(time())
#DEBUG MODE:
currenttime = 1383691015

visiblepasses = agechecker(refresh_passes(True))
regularpasses = agechecker(refresh_passes(False))

while True:

# Report on all data packets received and
# where they came from in each case (as this is
# UDP, each may be from a different source and it's
# up to the server to sort this out!)
  data,addr = UDPSock.recvfrom(1024)
  remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string
  #print data.strip(),addr

  print '  RX: "%s" @ %s from %s' % (data.rstrip('\n'), ctime(), remoteIP)
  if (data.strip() == 'iss?'):
    try:

      currenttime = int(time())

      visiblepasses = agechecker(visiblepasses)
      regularpasses = agechecker(regularpasses)

#      if visiblepasses:

#      else:
      visiblepasses = agechecker(refresh_passes(True))
#         if visiblepasses:



        #brne -> get new data

        #parse data

      MESSAGE=GetNextPassFromRows(allRows,visibleRows)

    except:
      MESSAGE='fail at this end, sorry'

    UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
    print '  TX: %s' % (MESSAGE)



