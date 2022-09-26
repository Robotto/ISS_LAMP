
import datetime
from dateutil import parser, tz
import mechanize
from bs4 import BeautifulSoup
from ip2geotools.databases.noncommercial import DbIpCity
from timezonefinder import TimezoneFinder

# static methods, because utility functions don't really need an instance...

class IssPassUtil:
    @staticmethod
    def getLatLonFromIP(ipv4):
        # https://iplocation.io/
        # https://db-ip.com/<IPV4>
        # https://pypi.org/project/ip2geotools/
        #usage: IssPassUtil.getLatLonFromIP(ipv4)

        response = DbIpCity.get(ipv4, api_key='free')
        return [response.latitude,response.longitude]

    @staticmethod
    def getTZfromLatLon(lat,lon):
        # https://koalatea.io/timezone-from-location/
        tf = TimezoneFinder()
        return tf.timezone_at(lng=lon, lat=lat)

    '''returns "1" or "0"'''
    @staticmethod
    def getClientDSTstr(timezone,epoch):
        return str(int(datetime.datetime.fromtimestamp(epoch, tz.gettz(timezone)).dst().seconds / 3600))

    '''return a collection of parseable ISS-PASS rows from heavens above'''
    @staticmethod
    def get_html_return_rows(url):
        br = mechanize.Browser()
        br.set_handle_robots(False)
        # Get the ISS PASSES pages:
        print(f'Retrieving list of passes from {url}')
        html = br.open(url).read()

        soup = BeautifulSoup(html.decode('UTF-8'), features="html5lib")
        rows = soup.findAll('tr', {"class": "clickableRow"})
        # print(f"{len(rows)} rows in rows: {rows}")
        return rows

    '''Returns an instance of IssPass built from a row from heavens-above.com'''
    @staticmethod
    def getPassFromRow(row):
        p = IssPassUtil.rowParser(row) #ew...
        return IssPass(p["startAz"], p["maxAz"], p["endAz"], p["tStart"], p["tMax"], p["tEnd"], p["magnitude"])

    '''Parses rows of data from heavens-above.com'''
    @staticmethod
    def rowParser(row):

        cols = row.findAll('td')
        dStr = cols[0].a.string

        # visible passes have a magnitude, non-visible passes do not:
        try:
            mag = float(cols[1].string)
        except:
            mag = None

        t1Str = ':'.join(cols[2].string.split(':'))
        t2Str = ':'.join(cols[5].string.split(':'))
        t3Str = ':'.join(cols[8].string.split(':'))
        alt1 = cols[3].string.replace('\xB0', '')  # remove '°'
        az1 = cols[4].string
        alt2 = cols[6].string.replace('\xB0', '')
        az2 = cols[7].string
        alt3 = cols[9].string.replace('\xB0', '')
        az3 = cols[10].string

        azAlt1 = '%s-%s' % (az1, alt1)
        azAlt2 = '%s-%s' % (az2, alt2)
        azAlt3 = '%s-%s' % (az3, alt3)

        startUnix, maxUnix, endUnix = IssPassUtil.makeTime(dStr, t1Str, t2Str, t3Str)

        return {"startAz": azAlt1, "maxAz": azAlt2, "endAz": azAlt3, "tStart": startUnix, "tMax": maxUnix, "tEnd": endUnix, "magnitude": mag}

    '''Takes strings from the website about dates and times, and infers real time data'''
    @staticmethod
    def makeTime(dateString, timeStartStr, timeMaxStr, timeEndStr):

        '''check whether the pass occurs next year'''
        # if we are in december and dStr is a date in january, the pass is in the next year
        if 'Jan' in dateString and datetime.date.today().month == 12:
            inferredYear = datetime.date.today().year + 1
        else:
            inferredYear = datetime.date.today().year

        dtStr1 = f'{dateString} {inferredYear} {timeStartStr} UTC'
        dtStr2 = f'{dateString} {inferredYear} {timeMaxStr} UTC'
        dtStr3 = f'{dateString} {inferredYear} {timeEndStr} UTC'

        dt1 = parser.parse(dtStr1)
        dt2 = parser.parse(dtStr2)
        dt3 = parser.parse(dtStr3)

        '''
        Check whether midnight occurs during pass, since this would break using datestring to create all dt objects!
        if discrepancies are found, shift the parsed timestamps one day forward (they passed midnight)
        '''
        if dt2 < dt1:
            dt2 += datetime.timedelta(days=1)
        if dt3 < dt2:
            dt3 += datetime.timedelta(days=1)

        #Extra super special case: A pass occurs during midnight on new-years eve!!
        if dt2 < dt1:
            dt2.replace(year=dt2.year+1)
        if dt3 < dt2:
            dt3.replace(year=dt3.year+1)


        return int(dt1.timestamp()), int(dt2.timestamp()), int(dt3.timestamp())


    @staticmethod
    def message(issPass,DSTstr):
        #    (DST, V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)
        #    DST is added to from the caller, since it is dependent on client location.
        if issPass.magnitude:
            return f'V\0{DSTstr}\0{issPass.magnitude}\0{issPass.tStart}\0{issPass.startAz}\0{issPass.tMax}\0{issPass.maxAz}\0{issPass.tEnd}\0{issPass.endAz}'
        else:
            return f'R\0{DSTstr}\0{issPass.tStart}\0{issPass.startAz}\0{issPass.tMax}\0{issPass.maxAz}\0{issPass.tEnd}\0{issPass.endAz}'
class IssPass:

    def __init__(self, _startAz, _maxAz, _endAz, _tStart, _tMax, _tEnd, _magnitude=None):
        self.startAz = _startAz
        self.maxAz = _maxAz
        self.endAz = _endAz

        self.tStart = _tStart
        self.tMax = _tMax
        self.tEnd = _tEnd

        self.magnitude = _magnitude

    def isVisible(self):
        if self.magnitude:
            return True
        return False

    def __str__(self):
        return f'ISS pass data, for a {f"visible pass, with magnitude {self.magnitude}," if self.magnitude else "regular (non-visible) pass"} that starts on {datetime.datetime.fromtimestamp(self.tStart, tz=tz.UTC)}'

    '''
    Do a ten minute check to see if the visible pass isn't a delayed subset of the regular passes
    (regular passes always start and end at 10degrees elevation, visible passes sometimes start higher)
    These return values are meant to be counter intuitive...
    
    Regular pass list will contain ALL passes, including visible passes, making the visible passes a subset of the regular passes..
    
    Problem: Some visible passes are not visible for the entire pass, thus, in the list of all passes this pass will start earlier, 
    thus it will have an earlier timestamp and always be selected first.. i try to fix it by offsetting the compare by 10 minutes...
    '''
    def __lt__(self, other):
        return self.tStart + 600 < other.tStart  # if this pass is earlier than the other one. Even after adding ten minutes to own start time

    def __gt__(self, other):
        return other.tStart - 600 < self.tStart

    def __eq__(self, other):
        return abs(self.tStart-other.tStart) < 600  # They are the same pass.


