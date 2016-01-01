#!/usr/bin/env python

##living up to the license:
##This product includes GeoLite data created by MaxMind, available from
## <a href="http://www.maxmind.com">http://www.maxmind.com</a>.


# -*- coding: UTF-8 -*-

import mechanize
import GeoIP
import logging

from BeautifulSoup import BeautifulSoup

from datetime import datetime, date
from dateutil import tz

from time import strftime, strptime, mktime, struct_time, time, ctime, localtime, sleep
from getopt import getopt
#import os, sys
#import envoy #calls bash commands as seperate threads.. unused.. for now..
import collections #used to form a collection of passes

import socket
from IPy import IP

def refresh_passes(isvisible):
    #print "refresh_passes called"
    html = get_html(isvisible)
    rows = html_to_rows(html)
    passes = rows_to_sets(rows)
    return (passes)


def get_html(isvisible):

    #not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
    VisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)
    AllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)

    #http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=56.156361&lng=10.188631&alt=12&tz=CET
    #http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=56.156361&lng=10.188631&alt=12&tz=CET

    #VisibleURL = 'http://62.212.66.171/iss/visible.htm'
    #VisibleURL = 'http://62.212.66.171/iss/visible_no_passes.html'

    #AllURL = 'http://62.212.66.171/iss/all.html'
    #AllURL = 'http://62.212.66.171/iss/visible_but_no_passes.htm'

    br = mechanize.Browser()
    br.set_handle_robots(False)
    # Get the ISS PASSES pages:


    if isvisible:
        print '    Retrieving list of visible passes'
        Html = br.open(VisibleURL).read()
    else:
        print '    Retrieving list of regular passes'
        Html = br.open(AllURL).read()

    return(Html)


def html_to_rows(html):
    #print 'html_to_rows called'

    #print "    raw html:"
    #print html

        # In the past, Beautiful Soup hasn't been able to parse the Heavens Above HTML.
        # To get around this problem, we extract just the table of ISS data and set
        # it in a well-formed HTML skeleton. If there is no table of ISS data, create
        # an empty table
    try:
        #print "    trying to split"
        Table = html.split(r'<table class="standardTable"', 1)[1] #split after first "standard table" tag, return 2nd portion
        #print "Table after first split:"
        #print Table
        #Table = Table.split(r'<tr class="tablehead">', 1)[1] #split after first "tablehead" tag return second portion
        Table = Table.split(r'<thead>', 1)[1] #split after first "tablehead" tag return second portion
        #print "Table after second split:"
        #print Table
        Table = Table.split(r'</thead>', 1)[1] #split after first "tablehead" tag return second portion , again.
        Table = Table.split(r'</tr>', 1)[1] #split after first "</tr>" tag return second portion
        Table = Table.split(r'</table>', 1)[0] #split after first "</table>" return first portion

        #print "Table after all splits:"
        #print Table

    except IndexError:
        print "    no html to split, creating empty table.."
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

    #print "    created a new html... parsing"
    # Parse the newly created HTML.
    Soup = BeautifulSoup(newHtml)

    #Collect only the data rows of the table.

    #print "    creating collection of rows"
    Rows = Soup.findAll('table')[0].findAll('tr')[0:]
    #print 'The parsed rows:'
    #print Rows

    #print "    done."
    return (Rows)

def rows_to_sets(Rows):    #calls the rowparser for all the available rows, returns a set of passes.
    #print "rows_to_sets called"

    #passes = collections.deque(maxlen=50) #magic number...
    passes = collections.deque()

    for row in Rows:
        #print row
        (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag) = rowparser(row)
            ##insert age check here?
        passes.append([start,max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag])

    #print "passes:"
    #print passes
    #print "    done."
    return (passes)



def rowparser(row):
    #print "rowparser called"

    try:

        cols = row.findAll('td')
        dStr = cols[0].a.string

        try:

            mag = float(cols[1].string)
        except:

            mag = None

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

        (start,startUnix) = maketime(dStr,t1Str)
        (max,maxUnix) = maketime(dStr,t2Str)
        (end,endUnix) = maketime(dStr,t3Str)

        #print (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag)
        return (start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag)
    except: #No passes in
        print "    error in row, returning pass with None type in all fields"
        return (None, None, None, None, None, None, None, None, None, None)

#              0     1    2    3     4     5         6        7        8      9
#                                                    ^-The magic happens here.

def maketime(dStr,timestring):
    #print "maketime called"

    #time magic - source timezone is GMT/UTC, remember that!
    #look at: http://stackoverflow.com/questions/4770297/python-convert-utc-datetime-string-to-local-datetime
    from_zone = tz.tzutc()
    to_zone = tz.gettz(timezone) #determined from the IP of the source of the request

    #create string to be parsed:
    string = '%s %s %s' % (dStr, date.today().year, timestring) #this will break if next pass is in next calendar year. (FIXED in 2 lines!)
    #parse the created string into a datetime opbject:
    dt = datetime(*strptime(string, '%d %b %Y %H:%M:%S')[0:7])
    #if we are in december and dt is a date in january, assume it's next year (it always will be)
    if date.today().month==12 and dt.month==1:
            dt.replace(year=date.today().year+1)
    #timezome magic goes here:
    utc = dt.replace(tzinfo=from_zone)
    local_time = utc.astimezone(to_zone) #in local time from here - local time for whoever is doing the lookup
    unix_time =int(mktime(local_time.timetuple()))
    return(local_time,unix_time)

def getnextpass(passes): #returns the next future pass
    #print "getnextpass called"

    for isspass in passes:
        #print isspass
        if isspass[6]>currenttime:
            return(isspass)

def which_pass_is_next(visible,regular): #determines whether the next visible or regular pass is first
    #print "which_pass_is_next called"
    if visible is None:
        return regular
    elif regular[6]+600 > visible[6]: #do a ten minute check to see if the visible pass isn't a delayed subset of the regular passes
                                      #(regular passes always start and end at 10degrees elevation, visible passes sometimes start higher)
        return visible
    else:
        return regular

def passes_too_old(passes): #checks the age of the passes returns false if we're still good.
    #print "passes_too_old called"
    try:
        for isspass in passes:
        #print isspass
            if isspass[6]>currenttime: #check starttime for passes in the list
                return(False)

        return(True)
        #if (passes[-1][6]<currenttime): #is the last entry in the deque in the past?
        #    return(True)
    except IndexError:  #No data exists.. that's kind of too old... right?
            return(True)

incomingPort = 1337
remotePort = 1337
# A UDP server listening for packets on port 1337:
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
listen_addr = ("",incomingPort)
UDPSock.bind(listen_addr)

# Global Vars.
lat = 0
lon = 0
timezone = tz.tzlocal() #changed as soon as a lookup occurs

#last_html_get_unix_time = 0
last_visible_get_unix_time = 0
last_regular_get_unix_time = 0
html_cooldown_time = 86400 #24 hrs

try:

    logging.basicConfig(filename='ISS.log',level=logging.DEBUG)

    print 'Started @ %s' %(ctime())
    logging.info(str(ctime()) + ': Started')

    DST = localtime().tm_isdst
    if DST:
        print 'Daylight savings is active'
    else:
        print 'Daylight savings is inactive'

    print "Ready and waiting for inbound on port: %s"%incomingPort
    logging.info('Listening on port: '+str(incomingPort))

    while True:

        # Report on all data packets received and
        # where they came from in each case (as this is UDP, each may be from a different source and it's up to the server to sort this out!)

        #recvfrom waits for incoming data:
        data,addr = UDPSock.recvfrom(1024)


        remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string



        logging.info(str(ctime()) + ': RX: \"' + str(data.rstrip('\n')) + '\" from ' + str(remoteIP))

        gi = GeoIP.open("GeoLiteCity.dat", GeoIP.GEOIP_STANDARD) #get your own at http://dev.maxmind.com/geoip/legacy/geolite/

        gir = gi.record_by_addr(remoteIP)
        #gir = gi.record_by_addr('81.169.145.71') #somewhere in berlin

        lat=gir['latitude']
        lon=gir['longitude']
        timezone=gir['time_zone']

        print
        print ' RX: "%s" @ %s from %s' % (data.rstrip('\n'), ctime(), remoteIP)
        print ' Latitude: %s' %lat
        print ' Longitude: %s' %lon
        print ' Timezone: %s' %timezone
        print

        currenttime = int(time()) #Update time
        DST = localtime().tm_isdst #Update DST data

        if last_visible_get_unix_time==0: #if passes have never been recieved = first run
            logging.info('Retrieving passes.')
            visiblepasses = refresh_passes(True)
            regularpasses = refresh_passes(False)
            firstIP=remoteIP
            last_visible_get_unix_time=currenttime
            last_regular_get_unix_time=currenttime

        if remoteIP!=firstIP:
            print '     WARNING:     '
            print '     Change of client IP address. Pass data most likely invalid!!     '
            logging.warning('Change of client IP address. Pass data most likely invalid!!')

        #check the age of the passes, refresh them if neccesary, but only if quarantine isn't set:
        if currenttime>last_visible_get_unix_time+html_cooldown_time:
            visible_quarantine=False
            print "Quarantine for visible passes inactive."
            print
            #logging.info('Visible passes quarantine NOT active.')
        else:
            visible_quarantine=True
            seconds_to_lift=html_cooldown_time-(currenttime-last_visible_get_unix_time)
            unixtime_at_lift=localtime(currenttime+seconds_to_lift)
            logging.info('Visible pass quarantine active.')
            print "Quarantine for visible passes ACTIVE, here be dragons. normal operations will resume in %s seconds @ %s"%(seconds_to_lift, strftime('%d/%m %H:%M:%S',unixtime_at_lift))
            print

        if currenttime>last_regular_get_unix_time+html_cooldown_time:
            regular_quarantine=False
            print "Quarantine for regular passes inactive."
            print
            #logging.info('Regular passes quarantine NOT active.')
        else:
            regular_quarantine=True
            seconds_to_lift=html_cooldown_time-(currenttime-last_regular_get_unix_time)
            unixtime_at_lift=localtime(currenttime+seconds_to_lift)
            logging.info('Regular pass quarantine active.')
            print "Quarantine for regular passes ACTIVE, here be dragons. normal operations will resume in %s seconds @ %s"%(seconds_to_lift, strftime('%d/%m %H:%M:%S',unixtime_at_lift))
            print


        if (visible_quarantine is False):
            if passes_too_old(visiblepasses):
                logging.info('Refreshing visible passes.')
                print "Visible pass list for %s outdated, refreshing..." %(remoteIP)
                visiblepasses.clear()
                visiblepasses=refresh_passes(True)
                last_visible_get_unix_time=currenttime
        else:
            if passes_too_old(visiblepasses):
                logging.warning('Ran out of visible passes before end of quarantine.')
                print "Visible pass data outdated (or empty). But not enough time has passed since last get from heavens-above.com"
                #print visiblepasses
                visiblepasses.clear()

        if (regular_quarantine is False):
            if passes_too_old(regularpasses):
                logging.info('Refreshing regular passes.')
                print "Regular pass list for %s outdated, refreshing..." %(remoteIP)
                regularpasses.clear()
                regularpasses=refresh_passes(False)
                last_regular_get_unix_time=currenttime
        else:
            if passes_too_old(regularpasses):
                logging.warning('Ran out of regular passes before end of quarantine.')
                print "Regular pass data outdated (or empty). But not enough time has passed since last get from heavens-above.com"
                #print regularpasses
                regularpasses.clear()
            #this means that there will be no data in the deque, and that the bad data string will be sent if asked.



        if (data.strip() == 'iss?'):
            try:
                print "Checking for passes."

                next_visible_pass = getnextpass(visiblepasses)
                next_regular_pass = getnextpass(regularpasses)
                next_pass = which_pass_is_next(next_visible_pass,next_regular_pass)

                print 'The next pass of the ISS above %s, %s is:' % (lat,lon)
                #next_pass[9] is magnitude, which is 'None' if it's not a visible pass...
                if next_pass[9] is None:

                    print "Not visible, and will start in %s seconds @ %s" %(next_pass[6]-currenttime, next_pass[0].strftime('%d/%m %H:%M:%S'))
                    logging.info('Next pass is visible and will occur at '+ str(next_pass[0].strftime('%d/%m %H:%M:%S')))
                    logging.info('TX: R' + str(DST) + str(next_pass[6]) + str(next_pass[3]) + str(next_pass[7]) + str(next_pass[4]) + str(next_pass[8]) + str(next_pass[5]))
                    MESSAGE='R\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (DST, next_pass[6],next_pass[3],next_pass[7],next_pass[4],next_pass[8],next_pass[5])
                else:
                    print "VISIBLE! It will start in %s seconds @ %s" %(next_pass[6]-currenttime, next_pass[0].strftime('%d/%m %H:%M:%S'))
                    logging.info('Next pass is visible and will occur at '+ str(next_pass[0].strftime('%d/%m %H:%M:%S')))
                    logging.info('TX: V' + str(DST) + str(next_pass[9]) + str(next_pass[6]) + str(next_pass[3]) + str(next_pass[7]) + str(next_pass[4]) + str(next_pass[8]) + str(next_pass[5]))
                    MESSAGE='V\0%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (DST, next_pass[9], next_pass[6],next_pass[3],next_pass[7],next_pass[4],next_pass[8],next_pass[5])

                    #    (DST, V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)



            except:
                MESSAGE='fail at this end, sorry'
                logging.warning('parsing of data failed.')

            UDPSock.sendto(MESSAGE, (remoteIP, remotePort))
            print
            print ' TX: %s' % (MESSAGE)
            print
            print

except Exception as e:
    print "An error occurred, here's a thing: " + str(e)
    logging.warning(str(ctime()) + ': Fatal failure! Error message:')
    logging.warning(str(e))
