
import datetime
from dateutil import tz
from dateutil import parser


class IssPass:

    #def __init__(self, _startAz, _maxAz, _endAz, _tStart, _tMax, _tEnd, _magnitude=None):
    def __init__(self, row):
        cols = row.findAll('td')
        dStr = cols[0].a.string
        # visible passes have a magnitude, non-visible passes do not:
        try:
            self.magnitude = float(cols[1].string)
        except:
            self.magnitude = None

        t1Str = ':'.join(cols[2].string.split(':'))
        t2Str = ':'.join(cols[5].string.split(':'))
        t3Str = ':'.join(cols[8].string.split(':'))
        alt1 = cols[3].string.replace('\xB0', '')  # remove '°'
        az1 = cols[4].string
        alt2 = cols[6].string.replace('\xB0', '')
        az2 = cols[7].string
        alt3 = cols[9].string.replace('\xB0', '')
        az3 = cols[10].string

        self.startAz = f'{az1}-{alt1}'
        self.maxAz = f'{az2}-{alt2}'
        self.endAz = f'{az3}-{alt3}'

        self.tStart, self.tMax, self.tEnd = IssPass.makeTime(dStr, t1Str, t2Str, t3Str)

    def isVisible(self):
        if self.magnitude:
            return True
        return False

    def getStartTimestampUTC(self):
        return datetime.datetime.fromtimestamp(self.tStart, tz=tz.UTC)

    def getStartTimedeltaUTC(self):
        return self.getStartTimestampUTC()-datetime.datetime.now(datetime.timezone.utc)

    def startsInTheFuture(self):
        if self.getStartTimedeltaUTC().total_seconds()>0:
            return True
        else:
            return False

    def constructMessage(self, DSTstr):
        #    (DST, V_mag, V_startUnix, V_loc1, V_maxUnix, V_loc2, V_endUnix, V_loc3)
        #    DST is added to from the caller, since it is dependent on client location.
        if self.magnitude:
            return f'V\0{DSTstr}\0{self.magnitude}\0{self.tStart}\0{self.startAz}\0{self.tMax}\0{self.maxAz}\0{self.tEnd}\0{self.endAz}'
        else:
            return f'R\0{DSTstr}\0{self.tStart}\0{self.startAz}\0{self.tMax}\0{self.maxAz}\0{self.tEnd}\0{self.endAz}'

    def __str__(self):
        return f'{f"Visible pass, with magnitude {self.magnitude}," if self.magnitude else "Regular (non-visible) pass"} that starts on {self.getStartTimestampUTC()} (timedelta: {self.getStartTimedeltaUTC()})'

    '''
    Do a ten minute check to see if the visible pass isn't a delayed subset of the regular passes
    (regular passes always start and end at 10degrees elevation, visible passes sometimes start higher)
    These return values are meant to be counter intuitive...
    
    Regular pass list will contain ALL passes, including visible passes, making the visible passes a subset of the regular passes..
    
    Problem: Some visible passes are not visible for the entire pass, thus, the same pass will have an earlier timestamp in the list of regular passes, 
    and always be selected first.. i try to fix it by offsetting the compare by 10 minutes... which is definitely not enough time orbit the planet, and be confused with another pass ;)
    '''
    def __lt__(self, other):
        return self.tStart + 600 < other.tStart  # if this pass is earlier than the other one. Even after adding ten minutes to own start time

    def __gt__(self, other):
        return other.tStart - 600 < self.tStart

    def __eq__(self, other):
        return abs(self.tStart-other.tStart) < 600  # They are the same pass.


    '''
    makeTime:
    Takes strings from the website about dates and times, and infers real time data -
    ASSUMES THAT HEAVENS ABOVE HAS RETURNED PASS DATA IN UTC
    '''
    @staticmethod
    def makeTime(dateString, timeStartStr, timeMaxStr, timeEndStr):

        inferredYear = datetime.date.today().year
        # check whether the pass occurs next year:
        # if we are in december and dStr is a date in january, the pass is in the next year
        if 'Jan' in dateString and datetime.date.today().month == 12:
            inferredYear += 1

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



