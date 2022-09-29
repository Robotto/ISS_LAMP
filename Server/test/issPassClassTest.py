


lat = "56.1609"
lon = "10.2042"

#not providing heavens-above with a tz gives you the data in utc time.. which is what you want. :)
VisibleURL = f'http://heavens-above.com/PassSummary.aspx?showAll=f&satid=25544&lat={lat}&lng={lon}&alt=12'
AllURL = f'http://heavens-above.com/PassSummary.aspx?showAll=t&satid=25544&lat={lat}&lng={lon}&alt=12'
VisibleURL = 'https://sardukar.moore.dk/visible.html'
AllURL = 'https://sardukar.moore.dk/regular.html'


from issPassClass import IssPassUtil

visiblePasses = []
for row in IssPassUtil.get_html_return_rows(VisibleURL):
    newPass = IssPassUtil.getPassFromRow(row)
    visiblePasses.append(newPass)

allpasses = []
for row in IssPassUtil.get_html_return_rows(AllURL):
    newPass = IssPassUtil.getPassFromRow(row)
    allpasses.append(newPass)

Regular = allpasses[1]
Visible = visiblePasses[1]

print(f'Regular __str__(): {Regular}')
print(f'Visible __str__(): {Visible}')


print(f'is Visible visible? {Visible.isVisible()}')
assert Visible.isVisible()

print(f'is Regular visible? {Regular.isVisible()}')
assert not Regular.isVisible()

print(f'Is regular pass ({Regular.tStart}) earlier than visible pass ({Visible.tStart}) (regular-visible={Regular.tStart-Visible.tStart})? {Regular<Visible}')
print(f'Is regular pass later than visible pass? {Regular>Visible}')
assert Regular < Visible
assert Visible > Regular

print(f'Are they the same? {Regular==Visible}')
assert Regular != Visible

print(f'is regular the same as regular?? {Regular==Regular}')
assert Regular == Regular

print(f'is Visible the same as Visible?? {Visible==Visible}')
assert Visible == Visible

