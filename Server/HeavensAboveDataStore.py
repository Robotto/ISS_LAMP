import datetime
import logging
from dateutil import tz
from timezonefinder import TimezoneFinder
import mechanize
from bs4 import BeautifulSoup
from issPassClass import IssPass


'''
locationSpecificISSpassStorage - A store for each client location.
Stores two instances of the URLspecificPassDataStore; One for visible passes and one for regular passes
'''
class locationSpecificISSpassStorage:

    def __init__(self, lat, lon):
        # not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
        self.visiblePassesURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=12' % (lat, lon)
        self.allPassesURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=12' % (lat, lon)

        self.visiblePasses = URLSpecificPassDataStore(self.visiblePassesURL)
        self.regularPasses = URLSpecificPassDataStore(self.allPassesURL)

        # https://koalatea.io/timezone-from-location/
        tf = TimezoneFinder()
        self.timezone = tf.timezone_at(lng=lon, lat=lat)

        self.lastCallWasAt = datetime.datetime.now()

    def getNextPass(self):
        self.lastCallWasAt = datetime.datetime.now()
        nextVisible = self.visiblePasses.getNextPass()
        nextRegular = self.regularPasses.getNextPass()

        if nextVisible is not False and nextRegular is not False:
            if nextVisible == nextRegular: #maybe it's the same pass?, then prefer the visible one!
                return nextVisible
            else:
                return min([nextVisible, nextRegular])
        #todo: is this really the most elegant solution?
        elif nextVisible is False and nextRegular is False:
            logging.error(f"Both localised datastores are empty and cannot return passes for comparison...")
            return False
        elif nextRegular is False:
            logging.warning(f"nextRegular doesn't exist, but nextVisible does.. Probably due to quarantine.")
            return nextVisible
        else:
            return nextRegular

    '''returns "1" or "0"'''
    def isDstAtClientLocation(self):
        return str(int(datetime.datetime.now(tz.gettz(self.timezone)).dst().seconds / 3600))

    # if datastore hasn't been asked for a pass in 7 days, it's stale and should probably be discarded
    def isStale(self):
        return datetime.datetime.now() - self.lastCallWasAt > datetime.timedelta(days=7)


''' 
URLSpecificPassDataStore - Stores passes from one specific heavens-above url
Automagically refreshes passes (if needed) when asked for next pass. 
'''
class URLSpecificPassDataStore:
    def __init__(self,url):
        self.quarantineUntil = datetime.datetime.now()
        self.passURL = url
        self.passList = []
        self.refreshPasses()

    def log_datastore(self,info):
        logging.debug(f"All passes in store ({info}): {len(self.passList)}")
        for index,isspass in enumerate(self.passList, start=1):
            if not isspass.startsInTheFuture():
                logging.warning(f"#{index}: {isspass}")
            else:
                logging.debug(f"#{index}: {isspass}")

    def refreshPasses(self):

        #self.log_datastore("before refresh")

        #remove passes that ended in the past
        if len(self.passList) > 0: #list will be empty on first run.
            for issPass in self.passList[:]: #iterate through a copy of the list
                if not issPass.startsInTheFuture(): #has this pass already started?
                    logging.info(f'REMOVING old pass: {issPass}')
                    self.passList.remove(issPass)

        #Empty list?
        if len(self.passList) < 1:
            if datetime.datetime.now() < self.quarantineUntil: #if pass list is empty, but quarantine is active
                print(f'WARNING! pass list for url: {self.passURL} is empty, but not enough time has passed since last query!')
                logging.warning(f'Pass list for {self.passURL} is empty, but quarantine does not end before {self.quarantineUntil} (Timedelta: {self.quarantineUntil-datetime.datetime.now()})')
                self.log_datastore("after refresh")
                return False
            else:
                for row in URLSpecificPassDataStore.get_html_return_rows(self.passURL):
                    newPass = IssPass(row)
                    if newPass.startsInTheFuture():
                        self.passList.append(newPass)
                self.quarantineUntil = datetime.datetime.now() + datetime.timedelta(days=1)
                logging.info(f'Quarantine for {f"Visible passes" if "showAll=f" in self.passURL else "Regular passes"}, (url: {self.passURL}) is now active for 24 Hours.')

        #self.log_datastore("after refresh")

        # check to see if the refresh actually got passes in the future.
        if len(self.passList)>0:
            return True
        else:
            logging.error(f"Pass list for {self.passURL} is empty, but refreshing it didn't work...")
            return False





    def getNextPass(self):
        self.lastCallWasAt = datetime.datetime.now()
        if self.refreshPasses(): #if datastore contains passes.
            return self.passList[0] #return first pass in list. Assuming that they are sorted chronologically.
        else:
            return False

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







