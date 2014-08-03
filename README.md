ISS_LAMP
========

My take on the ISS lamp based on the arduino uno hardware, with a POE enabled ethernet shield.

The idea is to have a "stand-alone" unit which only needs the POE injected CAT5e cable to work.

The 'client' is an arduino (uno) which uses an NTP lookup and a UDP connection to get data about the International Space Station,
which is parsed (upon request) from heavens-above.com by the 'Server' side python script

Both visible and invisible passes are parsed, visible passes have priority. (visible passes are also listed in the "all passes" table, this is accounted for)

The server side script is meant to be running in eg. a screen session on a linux machine.
A caching version that spares the ressources of heavens-above.com considerably, has been implemented, since HA started to limit page hits per hour pr device.
This was my chance to clean upthe python a bit, so it's actually much nicer now.

The IPv4 address of the server (and GMT offset for ntp time) is hardcoded into the arduino code. but it's easily changeable.

The location (LATTITUDE, LONGTITUDE, TIMEZONE), that the ISS data lookups are based on, are no longer hardcoded into the python script, but managed by doing a lookup in the [maxmind geolite database](http://dev.maxmind.com/geoip/legacy/geolite/) .

I use an old VFD display as my debug window on the arduino. It should be quite easy to change it to the serial interface, but beware the hardware.

This is still very much a prototype, A work in progress, an ongoing thing. It is, nor will it ever, be "done".

The arduino code resides in an ino project. If you don't know ino, [have a look at it right away!](http://inotool.org/) , I use it together with the [ST2-Arduino](https://github.com/geetarista/ST2-Arduino) plugin for [sublime text 3](http://www.sublimetext.com/) for a nice alternative to the arduino IDE.

Since ino is still in beta, a bug prohibits it from compiling any code using the ethernet library. fortunately there is a really easy workaround [documented here](https://github.com/amperka/ino/issues/55#issuecomment-47021016).

Some of the code in this project has been copied from all over the interwebs, i'm sorry, but i haven't kept track of my sources. If you feel wronged, drop me a line and i'll give credit where it's due.

I will however take full credit for the VFD code, which sits at the bottom of the giant pile of shitty code that i call an arduino project.

By take credit i mean that i wrote it, and hereby disclaim all ownership of it. feel free to use this code and edit, sell, buy, republish-taking-credit or print-out-and-eat it.

~Robotto
