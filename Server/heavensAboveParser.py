from time import time
import mechanize
from bs4 import BeautifulSoup

from issPassClass import IssPass

class HeavensAboveDataStore:

    quarantineDuration = 86400

    def __init__(self,sourceUrl):
        self.quarantineUntil=0
        self.sourceUrl=sourceUrl
        self.passes = []

    def refreshPasses(self):
        if not self.quarantined():
            rows = self.getHtmlAndReturnRows()
            self.quarantineUntil = int(time()) + self.quarantineDuration

            for row in rows:
                self.passes.append(IssPass(row))
            print(f'Parsed {len(rows)} rows, and added {len(self.passes)} passes')
        else:
            print('Error! unable to refresh passes! Quarantine active!')

    def quarantined(self):
        return int(time()) > self.quarantineUntil

    def getHtmlAndReturnRows(self):
        br = mechanize.Browser()
        br.set_handle_robots(False)
        print(f'Retrieving list of passes from {self.sourceUrl}')
        html = br.open(self.sourceUrl).read()
        data = html.decode('UTF-8')

        Soup = BeautifulSoup(data,features="html5lib")
        Rows = Soup.findAll('tr', {"class": "clickableRow"})
        return (Rows)

    def getNextPass(self):
        for passObject in self.passes:
            if passObject.startTimeUnix > int(time()):
                return passObject

    #TODO: finish this.
    def removeOldPasses(self):
        pass



class IssDataGetterParserStorer:
    def __init__(self):
        pass
#TODO: Make two instances of the above, have them refresh their stuff, ask them both for their next pass, and choose the earliest one from that.


class IssDataServeer:
    def __init__(self):
        pass
#TODO: open connection, listen for request, answer politely.