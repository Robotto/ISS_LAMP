import mechanize
import ipinfo
import logging

from bs4 import BeautifulSoup

from datetime import datetime, date
from dateutil import tz

from time import strftime, strptime, mktime, time, ctime, localtime





def get_html(isvisible):

    #not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
    VisibleURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)
    AllURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=12' %(lat, lon)

    VisibleURL = 'https://sardukar.moore.dk/visible.html'
    AllURL = 'https://sardukar.moore.dk/regular.html'
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

lat = "56.1609"
lon = "10.2042"


from issPassClass import IssPass


html = get_html(True)
rows = html_to_rows(html)

passes = []
for row in rows:
    newPass = IssPass(row)
    passes.append(newPass)

html = get_html(False)
rows = html_to_rows(html)

allpasses = []
for row in rows:
    newPass = IssPass(row)
    allpasses.append(newPass)

Regular = allpasses[2]
Visible = passes[1]

print(Regular)
print(Visible)

print(f'Is regular pass ({Regular.startTimeUnix}) earlier than visible pass ({Visible.startTimeUnix})? {Regular<Visible}')
print(f'Is regular pass later than visible pass? {Regular>Visible}')
print(f'Are they the same? {Regular==Visible}')

print(f'is regular the same as regular?? {Regular==Regular}')

print(f'is Visible the same as Visible?? {Visible==Visible}')

