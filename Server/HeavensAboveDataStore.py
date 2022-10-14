from issPassClass import IssPassUtil
import datetime
import time
import logging
from dateutil import tz

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

        self.timezone = IssPassUtil.getTZfromLatLon(lat, lon) #computationally intensive!

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
        # if datastore hasn't been asked for a pass in n days, it's stale and should probably be discarded

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

    def refreshPasses(self):

        #remove passes that ended in the past
        if len(self.passList) > 0: #list will be empty on first run.
            for issPass in self.passList:
                if not issPass.startsInTheFuture(): #has this pass already started?
                    logging.info(f'REMOVING old pass: {issPass}')
                    self.passList.remove(issPass)

        #Empty list?
        if len(self.passList) < 1:
            if datetime.datetime.now() < self.quarantineUntil: #if pass list is empty, but quarantine is active
                print(f'WARNING! pass list for url: {self.passURL} is empty, but not enough time has passed since last query!')
                logging.warning(f'Pass list for {self.passURL} is empty, but quarantine does not end before {self.quarantineUntil} (Timedelta: {self.quarantineUntil-datetime.datetime.now()})')
                return False
            else:
                for newPass in IssPassUtil.getPassesFromUrl(self.passURL):
                    if newPass.tStart > int(time.time()): #Does this newly parsed pass start in the future?
                        self.passList.append(newPass)
                self.quarantineUntil = datetime.datetime.now() + datetime.timedelta(days=1)
                logging.info(f'Quarantine for {f"Visible passes" if "showAll=f" in self.passURL else "Regular passes"}, (url: {self.passURL}) is now active for 24 Hours.')


        # check to see if the refresh actually got passes in the future.
        if len(self.passList)>0:
            return True
        else:
            logging.error(f"Pass list for {self.passURL} is empty. And Refreshing it didn't work...")
            return False

    def getNextPass(self):

        logging.debug(f"All passes in store (before refresh): {len(self.passList)}")
        for index,isspass in enumerate(self.passList, start=1):
            logging.debug(f"#{index}: {isspass}")

        self.lastCallWasAt = datetime.datetime.now()

        if self.refreshPasses(): #if datastore contains passes.
            return self.passList[0] #return first pass in list. Assuming that they are sorted chronologically.
        else:
            return False

