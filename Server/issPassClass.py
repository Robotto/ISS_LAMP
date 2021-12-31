
import datetime
from dateutil import parser, tz


class IssPass:

    def __init__(self, row):
        self.rowParser(row)

    def isVisible(self):
        if self.magnitude:
            return True
        return False

    '''Parses rows of data from heavens-above.com'''
    def rowParser(self, row):

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

        loc1 = '%s-%s' % (az1, alt1)
        loc2 = '%s-%s' % (az2, alt2)
        loc3 = '%s-%s' % (az3, alt3)

        # like this one: https://heavens-above.com/passdetails.aspx?lat=56.1609&lng=10.2042&loc=Unspecified&alt=12&tz=UCT&satid=25544&mjd=59465.9980749864&type=A

        startUnix, maxUnix, endUnix = self.makeTime(dStr, t1Str, t2Str, t3Str)

        # self.startTimeStr = start
        # self.maxTimeStr = max
        # self.endTimeStr = end
        self.startAzStr = loc1
        self.maxAzStr = loc2
        self.endAzStr = loc3

        self.startTimeUnix = startUnix
        self.maxTimeUnix = maxUnix
        self.endTimeUnix = endUnix

        self.magnitude = mag

    '''Takes strings from the website about dates and times, and infers real time data'''
    def makeTime(self, dateString, timeStartStr, timeMaxStr, timeEndStr):

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

        # TODO: Extra super special case: A pass occurs during midnight on new-years eve????!?!

        return int(dt1.timestamp()), int(dt2.timestamp()), int(dt3.timestamp())

    def __str__(self):
        return f'ISS pass data, for a {f"visible pass, with magnitude {self.magnitude}," if self.magnitude else "regular (non-visible) pass"} that starts on {datetime.datetime.fromtimestamp(self.startTimeUnix, tz=tz.UTC)}'


    '''
    Do a ten minute check to see if the visible pass isn't a delayed subset of the regular passes
    (regular passes always start and end at 10degrees elevation, visible passes sometimes start higher)
    These return values are meant to be counter intuitive...
    
    Regular pass list will contain ALL passes, including visible passes, making the visible passes a subset of the regular passes..
    
    Problem: Some visible passes are not visible for the entire pass, thus, in the list of all passes this pass will start earlier, 
    thus it will have an earlier timestamp and always be selected first.. i try to fix it by offsetting the compare by 10 minutes...
    '''
    def __lt__(self, other):
        return self.startTimeUnix + 600 < other.startTimeUnix  # if this pass is earlier than the other one. Even after adding ten minutes to own start time

    def __gt__(self, other):
        return other.startTimeUnix - 600 < self.startTimeUnix

    def __eq__(self, other):
        return abs(self.startTimeUnix-other.startTimeUnix)<600  # They are the same pass.


