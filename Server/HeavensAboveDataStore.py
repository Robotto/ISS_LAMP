from issPassClass import IssPassUtil
import datetime
import time



class locationSpecificISSpassStorage:

    def __init__(self, lat, lon):
        # not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
        self.visiblePassesURL = 'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat=%s&lng=%s&alt=12' % (lat, lon)
        self.allPassesURL = 'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat=%s&lng=%s&alt=12' % (lat, lon)

        self.visiblePasses = URLSpecificPassDataStore(self.visiblePassesURL)
        self.regularPasses = URLSpecificPassDataStore(self.allPassesURL)

    def getNextPass(self):
        nextVisible = self.visiblePasses.getNextPass()
        nextRegular = self.regularPasses.getNextPass()
        return min([nextVisible, nextRegular])


''' Stores passes from one specific heavens-above url
    Automagically refreshes passes (if needed) when asked for next pass. 
'''
class URLSpecificPassDataStore:
    def __init__(self,url):
        self.quarantineUntil = 0
        self.passURL = url
        self.passList = []
        self.refreshPasses()

    def refreshPasses(self):
        #remove passes that ended in the past
        if len(self.passList) > 0: #list will be empty on first run.
            for issPass in self.passList:
                if issPass.tEnd > int(time.time()):
                    self.passList.remove(issPass) #this might break

        if len(self.passList)<1:
            if datetime.datetime.now() < self.quarantineUntil: #if pass list is empty, but quarantine is active
                print(f'Error! pass list for url: {self.passURL} is empty, but not enough time has passed since last query!')
                return False
            else:
                for row in IssPassUtil.get_html_return_rows(self.passURL):
                    newPass = IssPassUtil.getPassFromRow(row)
                    if newPass.tEnd > int(time.time()):
                        self.passList.append(newPass)
                self.quarantineUntil = datetime.datetime.now() + datetime.timedelta(days=1)
        if len(self.passList)>0: #check to see if the refresh actually got passes in the future.
            return True
        else:
            return False

    def getNextPass(self):
        if self.refreshPasses(): #if datastore now contains passes.
            return self.passList[0] #return first pass in list. Assuming that they are sorted chronologically.
        else:
            return False


