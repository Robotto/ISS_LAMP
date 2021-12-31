#!/usr/bin/env python
# -*- coding: UTF-8 -*-
import sys, os

import mechanize
import ipinfo
import logging

from bs4 import BeautifulSoup

from datetime import datetime, date
from dateutil import tz

from time import strftime, strptime, mktime, time, ctime, localtime

import socket
from IPy import IP

def refresh_passes(isvisible):
    #print "refresh_passes called"
    html = get_html(isvisible)
    rows = html_to_rows(html)
    passes = rows_to_list_of_passes(rows)
    return (passes)


def get_html(isvisible):

    #not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
    VisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)
    AllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)

    br = mechanize.Browser()
    br.set_handle_robots(False)
    # Get the ISS PASSES pages:
    if isvisible:
        print(f'    Retrieving list of visible passes from {VisibleURL}')
        Html = br.open(VisibleURL).read()
    else:
        print(f'    Retrieving list of regular passes from {AllURL}')
        Html = br.open(AllURL).read()

    return(Html.decode('UTF-8'))


def html_to_rows(html):
    Soup = BeautifulSoup(html,features="html5lib")
    Rows = Soup.findAll('tr', {"class": "clickableRow"})
    return (Rows)

def rows_to_list_of_passes(Rows):    #calls the rowparser for all the available rows, returns a set of passes.
    passes = []
    for row in Rows:
        passes.append(rowparser(row))
    print(passes)
    return (passes)

def rowparser(row):
    #print "rowparser called"

    try:
        cols = row.findAll('td')
        dStr = cols[0].a.string

        #visible passes have a magnitude, non-visible passes do not:
        try:
            mag = float(cols[1].string)
        except:
            mag = None

        t1Str = ':'.join(cols[2].string.split(':'))
        t2Str = ':'.join(cols[5].string.split(':'))
        t3Str = ':'.join(cols[8].string.split(':'))
        alt1 = cols[3].string.replace('\xB0', '') #remove '°'
        az1 = cols[4].string
        alt2 = cols[6].string.replace('\xB0', '')
        az2 = cols[7].string
        alt3 = cols[9].string.replace('\xB0', '')
        az3 = cols[10].string

        loc1 = '%s-%s' % (az1, alt1)
        loc2 = '%s-%s' % (az2, alt2)
        loc3 = '%s-%s' % (az3, alt3)

        #TODO: what if a pass starts and ends on different sides of midnight?
        # like this one: https://heavens-above.com/passdetails.aspx?lat=56.1609&lng=10.2042&loc=Unspecified&alt=12&tz=UCT&satid=25544&mjd=59465.9980749864&type=A

        (start,startUnix) = maketime(dStr,t1Str)
        (max,maxUnix) = maketime(dStr,t2Str)
        (end,endUnix) = maketime(dStr,t3Str)

        return [start, max, end, loc1, loc2, loc3, startUnix, maxUnix, endUnix, mag]
    except:
        print("    error in row, returning pass with None type in all fields")
        return [None, None, None, None, None, None, None, None, None, None]

#              0     1    2    3     4     5         6        7        8      9
#                                                    ^-The magic happens here.

def maketime(dStr,timestring):
    #print "maketime called"

    #time magic - source timezone is GMT/UTC, remember that!
    #look at: http://stackoverflow.com/questions/4770297/python-convert-utc-datetime-string-to-local-datetime
    from_zone = tz.tzutc()
    to_zone = tz.gettz(timezone) #determined from the IP of the source of the request

    #if we are in december and dStr is a date in january, the pass is in the next year
    if 'Jan' in dStr and date.today().month==12:
        inferredYear = date.today().year+1
    else:
        inferredYear=date.today().year

    #create string to be parsed:
    string = '%s %s %s' % (dStr, inferredYear, timestring) 
    #parse the created string into a datetime opbject:
    dt = datetime(*strptime(string, '%d %b %Y %H:%M:%S')[0:7])
    
    #timezome magic goes here:
    utc = dt.replace(tzinfo=from_zone)
    local_time = utc.astimezone(to_zone) #in local time from here - local time for whoever is doing the lookup
    unix_time = int(mktime(local_time.timetuple()))
    return(local_time,unix_time)

def getnextpass(passes): #returns the next future pass
    print("getnextpass called")
    
    for isspass in passes:
        if isspass[6] < currenttime: #remove outdated passes
            passes.remove(isspass)
        else:
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


incomingPort = 1337
remotePort = 1337
# A UDP server listening for packets on port 1337:
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
listen_addr = ("",incomingPort)
UDPSock.bind(listen_addr)


#last_html_get_unix_time = 0
last_visible_get_unix_time = 0
last_regular_get_unix_time = 0
html_cooldown_time = 86400 #24 hrs


logging.basicConfig(filename='ISS.log',level=logging.DEBUG)

print(f'Started @ {ctime()}')
logging.info(f'{ctime()}: Started')

DST = localtime().tm_isdst
if DST:
    print('Daylight savings is active')
else:
    print('Daylight savings is inactive')

print(f'Ready and waiting for inbound on port: {incomingPort}')
logging.info(f'Listening on port: {incomingPort}')

#init ipinfo handler:
handler = ipinfo.getHandler()

while True:

    # Report on all data packets received and
    # where they came from in each case (as this is UDP, each may be from a different source and it's up to the server to sort this out!)

    #recvfrom waits for incoming data:
    data,addr = UDPSock.recvfrom(1024)

    remoteIP=IP(addr[0]).strNormal() #convert address of packet origin to string

    logging.info(f'{ctime()}: RX: \"{data.rstrip()}\" from  {remoteIP}')

    details = handler.getDetails(remoteIP)

    #hardcoding lat/lon for a quick and dirty fix.. TODO: do it right later, when migrating to python3
    lat = "56.1609"
    lon = "10.2042"
    #lat=details.loc.split(',')[0]
    #lon=details.loc.split(',')[1]

    #timezone=details.timezone
    timezone= 'Europe/Copenhagen'

    print()
    print(f' RX: "{data.strip()}" @ {ctime()} from {remoteIP}')
    print(f' Latitude: {lat}')
    print(f' Longitude: {lon}')
    print(f' Timezone: {timezone}')
    print()

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
        print('     WARNING:     ')
        print('     Change of client IP address. Pass data most likely invalid!!     ')
        logging.warning('Change of client IP address. Pass data most likely invalid!!')

    #check the age of the passes, refresh them if neccesary, but only if quarantine isn't set:
    if currenttime>last_visible_get_unix_time+html_cooldown_time:
        visible_quarantine=False
        print("Quarantine for visible passes inactive.")
        #logging.info('Visible passes quarantine NOT active.')
    else:
        visible_quarantine=True
        seconds_to_lift=html_cooldown_time-(currenttime-last_visible_get_unix_time)
        unixtime_at_lift=localtime(currenttime+seconds_to_lift)
        logging.info('Visible pass quarantine active.')
        print("Quarantine for visible passes ACTIVE, here be dragons. normal operations will resume in %s seconds @ %s"%(seconds_to_lift, strftime('%d/%m %H:%M:%S',unixtime_at_lift)))

    if currenttime>last_regular_get_unix_time+html_cooldown_time:
        regular_quarantine=False
        print("Quarantine for regular passes inactive.")
        #logging.info('Regular passes quarantine NOT active.')
    else:
        regular_quarantine=True
        seconds_to_lift=html_cooldown_time-(currenttime-last_regular_get_unix_time)
        unixtime_at_lift=localtime(currenttime+seconds_to_lift)
        logging.info('Regular pass quarantine active.')
        print("Quarantine for regular passes ACTIVE, here be dragons. normal operations will resume in %s seconds @ %s"%(seconds_to_lift, strftime('%d/%m %H:%M:%S',unixtime_at_lift)))


    if len(visiblepasses) < 1:
        infoStr = f'Visible pass list for {remoteIP} has been emptied! '
        if visible_quarantine is False:
            infoStr += "Refreshing now!"
            visiblepasses = refresh_passes(True)
            last_visible_get_unix_time = currenttime
        else:
            infoStr += 'But not enough time has passed since last get from heavens-above.com :('
        print(infoStr)

    if len(regularpasses) < 1:
        infoStr = f'Regular pass list for {remoteIP} has been emptied! '
        if regular_quarantine is False:
            infoStr += "Refreshing now!"
            regularpasses=refresh_passes(False)
            last_regular_get_unix_time=currenttime
        else:
            infoStr += 'But not enough time has passed since last get from heavens-above.com :('
        print(infoStr)
        #this means that there will be no data in the lidt, and that the bad data string will be sent if asked.


    if (data.rstrip() == b'iss?'):
        try:
            print("Checking for passes.")

            next_visible_pass = getnextpass(visiblepasses)
            next_regular_pass = getnextpass(regularpasses)
            next_pass = which_pass_is_next(next_visible_pass,next_regular_pass)

            print('The next pass of the ISS above %s, %s is:' % (lat,lon))
            #next_pass[9] is magnitude, which is 'None' if it's not a visible pass...
            if next_pass[9] is None:

                print("Not visible, and will start in %s seconds @ %s" %(next_pass[6]-currenttime, next_pass[0].strftime('%d/%m %H:%M:%S')))
                logging.info('Next pass is visible and will occur at '+ str(next_pass[0].strftime('%d/%m %H:%M:%S')))
                logging.info('TX: R' + str(DST) + str(next_pass[6]) + str(next_pass[3]) + str(next_pass[7]) + str(next_pass[4]) + str(next_pass[8]) + str(next_pass[5]))
                MESSAGE='R\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (DST, next_pass[6],next_pass[3],next_pass[7],next_pass[4],next_pass[8],next_pass[5])
            else:
                print("VISIBLE! It will start in %s seconds @ %s" %(next_pass[6]-currenttime, next_pass[0].strftime('%d/%m %H:%M:%S')))
                logging.info('Next pass is visible and will occur at '+ str(next_pass[0].strftime('%d/%m %H:%M:%S')))
                logging.info('TX: V' + str(DST) + str(next_pass[9]) + str(next_pass[6]) + str(next_pass[3]) + str(next_pass[7]) + str(next_pass[4]) + str(next_pass[8]) + str(next_pass[5]))
                MESSAGE='V\0%s\0%s\0%s\0%s\0%s\0%s\0%s\0%s' % (DST, next_pass[9], next_pass[6],next_pass[3],next_pass[7],next_pass[4],next_pass[8],next_pass[5])

                #    (DST, V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)



        except:
            MESSAGE='fail at this end, sorry'
            logging.warning('parsing of data failed.')

        UDPSock.sendto(MESSAGE.encode('ASCII'), (remoteIP, remotePort))
        print()
        print(' TX: %s' % (MESSAGE))
        print('--------------------------------')
        print()

