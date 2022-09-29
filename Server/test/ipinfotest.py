#!/usr/bin/env python
import ipinfo
handler = ipinfo.getHandler()
details = handler.getDetails('62.107.0.140')

print 'location: ', details.loc
print 'timezone: ', details.timezone

