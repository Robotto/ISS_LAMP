import mechanize

from bs4 import BeautifulSoup




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

