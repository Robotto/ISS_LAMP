ISS_LAMP
========

My take on the ISS lamp, now based on an ESP8266 (wemos D1 mini). I did a writeup with nice pictures for [the hackaday challenge in 2014](http://hackaday.io/project/2026-iss-lamp-ntp-clock-artpiece)

The idea is to have a "stand-alone" unit which only needs power and wifi to work

The 'client' is an ESP8266 (wemos D1 mini) which uses an NTP lookup and a UDP connection to get data about the International Space Station,
which is parsed (upon request) from heavens-above.com by the 'Server' side python script

Both visible and invisible passes are parsed:
Visible passes have priority. (visible passes are also listed in the "all passes" table, but this is accounted for)

The server side script is meant to be running in eg. a screen session on a linux machine.
Since heavens-above.com started to limit page hits per hour pr device, a caching version that spares the ressources of the site considerably has been implemented. This was my chance to clean up the python a bit, so it's actually much nicer now.

The IPv4 address of the server (and GMT offset for ntp time) is hardcoded into the arduino code. And easily changeable there.

The location (LATTITUDE, LONGTITUDE, TIMEZONE), that the ISS data lookups are based on, are no longer hardcoded into the python script, but managed by doing a lookup in the [maxmind geolite database](http://dev.maxmind.com/geoip/legacy/geolite/) .

I use an old VFD display as my debug window on the arduino. It should be quite easy to change it to the serial interface, but beware the hardware.

This is still very much a prototype, A work in progress, an ongoing thing. It is not, nor will it ever, be "done". The latest addition is the option to upload ned code to the client hardware over-the-air, when connected to the same wifi.

Some of the code in this project has been copied from all over the interwebs, i'm sorry, but i haven't kept track of my sources. If you feel wronged, drop me a line and i'll give credit where it's due.

I will however take full credit for the VFD code, which has been libraryfied and has its own .h and .cpp files.

By take credit i mean that i wrote it, and hereby disclaim all ownership of it. feel free to use this code and edit, sell, buy, republish-taking-credit or print-out-and-eat it.

Prerequisites for the server:

```bash
sudo apt install python-mechanize python-geoip python-ipy python-dateutil python-beautifulsoup 
```

Happy hacking!
~Robotto
