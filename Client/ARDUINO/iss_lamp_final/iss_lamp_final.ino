/*
ISS LAMP

This sketch gets ISS data from a python script on robottobox
using an Arduino Wiznet Ethernet shield.

and ALSO NTP time from the danish NTP pool.

and now a check to see if daylight savings time is active

Also: VFD!
And Also: PWM!
and now: DNS LOOKUP for the NTP!

Todo:
VFD to class .. or not.. 800 lines of code isn't that bad.. is it?

Before shipping:
-Internalize heavens-above html scrape? .. probably not
*/

#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>


byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0x34, 0xFC }; //MAC address of the Ethernet shield
IPAddress robottobox(62,212,66,171); //IP address constructor
IPAddress DNS_IP(8,8,8,8); //Google DNS.
char NTP_hostName[] = "dk.pool.ntp.org"; //should init with a null-char termination.
IPAddress timeServer;//init empty IP adress container

// Initialize the Ethernet client libraries
EthernetClient client;
DNSClient my_dns; //for dns lookup of NTP server
EthernetUDP Udp; // A UDP instance to let us send and receive packets over UDP


//UDP (robottobox) stuff:
const unsigned int localPort = 1337;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE=128; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
int UDPretryDelay = 0;
int UDPretries = 0;


//Timekeeper stuff:
boolean DST = false; //Daylight savings? (summertime)
int GMT_plus = 1; //timezone offset from GMT
int timekeeper_standalone_seconds=0; //how long since last sync with ntp server
int ntp_ip_refresh_seconds=0; //how long since last refresh of ntp server ip

unsigned long lastmillis; //how long since last run of timekeeper()
unsigned long currentEpoch;

//these contain details about next pass:
unsigned long passStartEpoch;
unsigned long passMaxEpoch;
unsigned long passEndEpoch;
unsigned long secs_to_next_pass;

//in clock format:
unsigned long hh_to_next_pass;
unsigned long mm_to_next_pass;
unsigned long ss_to_next_pass;

boolean passVisible;
String passMagnitude;
String passStartDir;
String passMaxDir;
String passEndDir;

//Statemachine booleans:
unsigned int state=1; //start at 1 since first pass-get (state 0) is done in setup



//hardware setup:
int PWM_PIN=9;

//VFD IO STUFF:
int T0 = A0;
int CS = A1;
int RD = A2;
int RESET = A3;
int A_0 = A4;
int WR = A5;
byte customcharposition = 0xa0;
unsigned char VFD_data_pins[8];
int temp_vfd_position = 0;





/*
   SSSSSSSSSSSSSSS                              tttt
 SS:::::::::::::::S                          ttt:::t
S:::::SSSSSS::::::S                          t:::::t
S:::::S     SSSSSSS                          t:::::t
S:::::S                eeeeeeeeeeee    ttttttt:::::ttttttt    uuuuuu    uuuuuu ppppp   ppppppppp
S:::::S              ee::::::::::::ee  t:::::::::::::::::t    u::::u    u::::u p::::ppp:::::::::p   ::::::
 S::::SSSS          e::::::eeeee:::::eet:::::::::::::::::t    u::::u    u::::u p:::::::::::::::::p  ::::::
  SS::::::SSSSS    e::::::e     e:::::etttttt:::::::tttttt    u::::u    u::::u pp::::::ppppp::::::p ::::::
    SSS::::::::SS  e:::::::eeeee::::::e      t:::::t          u::::u    u::::u  p:::::p     p:::::p
       SSSSSS::::S e:::::::::::::::::e       t:::::t          u::::u    u::::u  p:::::p     p:::::p
            S:::::Se::::::eeeeeeeeeee        t:::::t          u::::u    u::::u  p:::::p     p:::::p
            S:::::Se:::::::e                 t:::::t    ttttttu:::::uuuu:::::u  p:::::p    p::::::p ::::::
SSSSSSS     S:::::Se::::::::e                t::::::tttt:::::tu:::::::::::::::uup:::::ppppp:::::::p ::::::
S::::::SSSSSS:::::S e::::::::eeeeeeee        tt::::::::::::::t u:::::::::::::::up::::::::::::::::p  ::::::
S:::::::::::::::SS   ee:::::::::::::e          tt:::::::::::tt  uu::::::::uu:::up::::::::::::::pp
 SSSSSSSSSSSSSSS       eeeeeeeeeeeeee            ttttttttttt      uuuuuuuu  uuuup::::::pppppppp
                                                                                p:::::p
                                                                                p:::::p
                                                                               p:::::::p
                                                                               p:::::::p
                                                                               p:::::::p
                                                                               ppppppppp
*/

void setup() {

    VFDsetup();

    VFDclear();

    PWM_ramp(true);


    // start the Ethernet connection:
    if (Ethernet.begin(mac) == 0)
    {
        VFDstring("Failed to configure Ethernet using DHCP!");
        PWM_ramp(false);
        while(1); //dead end.
    }

    else
    {
        VFDclear();
        VFDstring("Ethernet up! - IP: ");

        String localIpString="  "; //really weird.. string needs to be initialized with something in it to work properly...
        localIpString=String(Ethernet.localIP()[0]); //first byte

        //formatting for print:
        for(byte ip=1;ip<4;ip++){localIpString+='.'; localIpString+=String(Ethernet.localIP()[ip]);} //last 3 bytes
        VFDstring(localIpString);
        delay(1500);
    }

    Udp.begin(localPort);

    delay(1000); //give the ethernet shield some time.

    VFDclear();
    lookup_ntp_ip();
    VFDstring("DNS resolve: dk.pool.ntp.org");

    delay(1000);

    //transform IP byte array to string so that the debug window can tell user what ip is being used:
    static char ipstringbuf[16];
    sprintf(ipstringbuf, "%d.%d.%d.%d\0", timeServer[0], timeServer[1], timeServer[2], timeServer[3]);
    VFDclear();
    VFDstring("NTP IP resolved: ");
    VFDstring(ipstringbuf);

    delay(1000);

    ///////////////// QUERY NTP //////////////
    VFDstring("   UDP TX -> NTP");
    sendNTPpacket(timeServer);
    UDPwait(false); //false = NTP
    VFDstring("  NTP RX!!");
    handle_ntp();


    //////////////// ISS ///////////////
    delay(500);

    VFDclear();
    sendISSpacket(robottobox); //ask robottobox for iss data
    VFDstring("ISS TX -> Robottobox");
    UDPwait(true);
    VFDstring("  ISS RX!!");
    handle_ISS_udp();
    VFDchar(0,0x16); //cursor off.

    //PWM_ramp(false); //lights ramp down
    digitalWrite(PWM_PIN,true);
}


/*
LLLLLLLLLLL
L:::::::::L
L:::::::::L
LL:::::::LL
  L:::::L                  ooooooooooo      ooooooooooo   ppppp   ppppppppp
  L:::::L                oo:::::::::::oo  oo:::::::::::oo p::::ppp:::::::::p   ::::::
  L:::::L               o:::::::::::::::oo:::::::::::::::op:::::::::::::::::p  ::::::
  L:::::L               o:::::ooooo:::::oo:::::ooooo:::::opp::::::ppppp::::::p ::::::
  L:::::L               o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
  L:::::L               o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
  L:::::L               o::::o     o::::oo::::o     o::::o p:::::p     p:::::p
  L:::::L         LLLLLLo::::o     o::::oo::::o     o::::o p:::::p    p::::::p ::::::
LL:::::::LLLLLLLLL:::::Lo:::::ooooo:::::oo:::::ooooo:::::o p:::::ppppp:::::::p ::::::
L::::::::::::::::::::::Lo:::::::::::::::oo:::::::::::::::o p::::::::::::::::p  ::::::
L::::::::::::::::::::::L oo:::::::::::oo  oo:::::::::::oo  p::::::::::::::pp
LLLLLLLLLLLLLLLLLLLLLLLL   ooooooooooo      ooooooooooo    p::::::pppppppp
                                                           p:::::p
                                                           p:::::p
                                                          p:::::::p
                                                          p:::::::p
                                                          p:::::::p
                                                          ppppppppp
*/

void loop()
{

timekeeper();


switch(state)
  {
  case 0:
        sendISSpacket(robottobox); //ask robottobox for new iss data
        UDPwait(true);
        handle_ISS_udp();
        //state=1;
        break;

  case 1:
        VFDclear();
        if (passVisible) VFDstring("Visible pass T-LOADING        LOADING"); //15
        else VFDstring("Regular pass    T-LOADING        LOADING");
        //state=2;
        break;

  case 2:       //probably could be called default state..
        if(passVisible) VFDsetpos(15);
        else VFDsetpos(18);
        if(hh_to_next_pass<10) VFDchar(0,'0');
        VFDstring(String(hh_to_next_pass));
        VFDchar(0,':');
        if(mm_to_next_pass<10) VFDchar(0,'0');
        VFDstring(String(mm_to_next_pass));
        VFDchar(0,':');
        if(ss_to_next_pass<10) VFDchar(0,'0');
        VFDstring(String(ss_to_next_pass));
        if (passVisible)
        {
            VFDstring(" M:");
            VFDstring(passMagnitude);
        }
        clock();
        break;

  case 3: //regular  pass
        VFDclear();
        PWM_ramp(true); //lights fade on
        VFDstring("Non-visible pass in progress.");
        clock();
        break;

  case 4: //visible pass
        VFDclear();
        PWM_ramp(true); //lights fade on

        VFDscrollMode(true);

        VFDstring("VISIBLE PASS STARTING!");
        delay(500);

        VFDstring("      Magnitude: ");
        VFDstring(passMagnitude);

        delay(500);

        VFDstring("      Direction: ");
        VFDstring(passStartDir);

        delay(500);

        VFDstring("      Duration: ");
        VFDstring(String((int)(passEndEpoch-passStartEpoch)));
        VFDstring(" seconds...");

        delay(1500);

        VFDscrollMode(false);

        //print info about upcoming pass max
        VFDclear();
        VFDstring("Visible pass max @");
        VFDstring(passMaxDir);
        VFDstring(" in T-");
        temp_vfd_position=24+passMaxDir.length();
        break;

  case 5: //visible pass countdown to max
        VFDsetpos(temp_vfd_position);
        VFDstring(String((int)(passMaxEpoch-currentEpoch)));
        VFDstring(" seconds ");
        break;

  case 6: //visible pass print end info
        VFDclear();
        VFDstring("Visible pass end @");  //18
        VFDstring(passEndDir);            //4-6
        VFDstring(" in T-");              //6
        temp_vfd_position=24+passEndDir.length();
        break;

  case 7: //visible pass countdown to end
        VFDsetpos(temp_vfd_position);
        VFDstring(String((int)(passEndEpoch-currentEpoch))); //1-3
        VFDstring(" seconds ");
        break;

  case 8: //pass ended
        VFDclear();
        VFDstring("End of pass.");
        PWM_ramp(false); //lights fade off
        VFDclear();
        break;
  }

state_update();

}


/*
                         tttt            iiii  lllllll
                      ttt:::t           i::::i l:::::l
                      t:::::t            iiii  l:::::l
                      t:::::t                  l:::::l
uuuuuu    uuuuuuttttttt:::::ttttttt    iiiiiii  l::::l     ssssssssss
u::::u    u::::ut:::::::::::::::::t    i:::::i  l::::l   ss::::::::::s   ::::::
u::::u    u::::ut:::::::::::::::::t     i::::i  l::::l ss:::::::::::::s  ::::::
u::::u    u::::utttttt:::::::tttttt     i::::i  l::::l s::::::ssss:::::s ::::::
u::::u    u::::u      t:::::t           i::::i  l::::l  s:::::s  ssssss
u::::u    u::::u      t:::::t           i::::i  l::::l    s::::::s
u::::u    u::::u      t:::::t           i::::i  l::::l       s::::::s
u:::::uuuu:::::u      t:::::t    tttttt i::::i  l::::l ssssss   s:::::s  ::::::
u:::::::::::::::uu    t::::::tttt:::::ti::::::il::::::ls:::::ssss::::::s ::::::
 u:::::::::::::::u    tt::::::::::::::ti::::::il::::::ls::::::::::::::s  ::::::
  uu::::::::uu:::u      tt:::::::::::tti::::::il::::::l s:::::::::::ss
    uuuuuuuu  uuuu        ttttttttttt  iiiiiiiillllllll  sssssssssss
*/

void timekeeper(void)
{

    while(millis()<lastmillis+1000) {} //WAIT FOR ABOUT A SECOND

    currentEpoch+=((millis()-lastmillis)/1000); //add  a second or more to the current epoch
    lastmillis=millis();
    timekeeper_standalone_seconds++;
    ntp_ip_refresh_seconds++;
    secs_to_next_pass=passStartEpoch-currentEpoch;

    if(timekeeper_standalone_seconds>=30)
        {
            sendNTPpacket(timeServer);
            UDPwait(false);
            handle_ntp();
            currentEpoch++; // meh.. calibration...
            timekeeper_standalone_seconds=0;
        }
    if(ntp_ip_refresh_seconds>3600)
        {
            ntp_ip_refresh_seconds=0;
            lookup_ntp_ip(); //for every hour.
        }

    hh_to_next_pass=(secs_to_next_pass  % 86400L) / 3600;
    mm_to_next_pass=(secs_to_next_pass % 3600) / 60;
    ss_to_next_pass=secs_to_next_pass % 60;
}

void state_update()
{
    switch(state)
    {
        case 0:
            state=1;
            break;
        case 1:
            state=2;
            break;
        case 2:
            if(currentEpoch>=passStartEpoch)
                {
                if(!passVisible) state=3;
                else state=4;
                }
            break;
        case 3:
            if(currentEpoch>=passEndEpoch) state=8;
            break;
        case 4:
            state = 5;
            break;
        case 5:
            if(currentEpoch>=passMaxEpoch) state=6;
            break;
        case 6:
            state=7;
            break;
        case 7:
            if (currentEpoch>=passEndEpoch) state=8;
            break;

        case 8:
            state = 0;
            break;

        default:
            VFDclear();
            VFDstring("statemachine b0rked!");
            errorclock();
            break;

    }
}

void lookup_ntp_ip(void)
{
    ///////////////// LOOKUP NTP IP //////////////
    //EthernetDNS.setDNSServer(dnsServerIp);
    my_dns.begin(DNS_IP);
    if(my_dns.getHostByName(NTP_hostName, timeServer) !=1)
    {
        VFDclear();
        VFDstring("NTP DNS lookup failed");
        delay(1000);
        errorclock(); //if it returns something other than 1 we're in trouble.
    }


}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void errorclock(void)
{
  //PWM_ramp(false); //lights off
  digitalWrite(PWM_PIN,true);
  unsigned int error_seconds=0;
  VFDclear();
  VFDchar(1,1); //set VFD position.
  VFDchar(0,'!'); //print error indicator
  //VFDchar(0,0x16); //cursor off
  VFDcursor(false);

  while(1)
  {
    while(millis()<lastmillis+1000) {} //WAIT FOR ABOUT A SECOND
    currentEpoch+=((millis()-lastmillis)/1000); //add  a second or more to the current epoch
    lastmillis=millis();
    error_seconds++;

    clock();

    if (error_seconds>120) resetFunc();  //call reset after 2 minutes
  }
}

/*
PPPPPPPPPPPPPPPPP   WWWWWWWW                           WWWWWWWWMMMMMMMM               MMMMMMMM
P::::::::::::::::P  W::::::W                           W::::::WM:::::::M             M:::::::M
P::::::PPPPPP:::::P W::::::W                           W::::::WM::::::::M           M::::::::M
PP:::::P     P:::::PW::::::W                           W::::::WM:::::::::M         M:::::::::M
  P::::P     P:::::P W:::::W           WWWWW           W:::::W M::::::::::M       M::::::::::M
  P::::P     P:::::P  W:::::W         W:::::W         W:::::W  M:::::::::::M     M:::::::::::M ::::::
  P::::PPPPPP:::::P    W:::::W       W:::::::W       W:::::W   M:::::::M::::M   M::::M:::::::M ::::::
  P:::::::::::::PP      W:::::W     W:::::::::W     W:::::W    M::::::M M::::M M::::M M::::::M ::::::
  P::::PPPPPPPPP         W:::::W   W:::::W:::::W   W:::::W     M::::::M  M::::M::::M  M::::::M
  P::::P                  W:::::W W:::::W W:::::W W:::::W      M::::::M   M:::::::M   M::::::M
  P::::P                   W:::::W:::::W   W:::::W:::::W       M::::::M    M:::::M    M::::::M
  P::::P                    W:::::::::W     W:::::::::W        M::::::M     MMMMM     M::::::M ::::::
PP::::::PP                   W:::::::W       W:::::::W         M::::::M               M::::::M ::::::
P::::::::P                    W:::::W         W:::::W          M::::::M               M::::::M ::::::
P::::::::P                     W:::W           W:::W           M::::::M               M::::::M
PPPPPPPPPP                      WWW             WWW            MMMMMMMM               MMMMMMMM
*/
/*
void PWM_ramp(boolean direction, unsigned long duration_ms) //true=up/false=down , duration in millisecs
{
  //this may seem reversed, but the hardware that drives the LED's inverts the PWM, so that 100% is (almost) full off.
  if(direction) for (int i = 255; i > 0; i--) {analogWrite(PWM_PIN,i); delay(duration_ms>>8);} //(duration_m/255)
  else for (int i = 0; i < 255; i++) {analogWrite(PWM_PIN,i); delay(duration_ms>>8);}
}
*/

void PWM_ramp(boolean direction) //true=up/false=down , duration in millisecs
{
  //this may seem reversed, but the hardware that drives the LED's inverts the PWM, so that 100% is (almost) full off.
  int i;
  if(direction) for (i = 255; i > 0; i--) {analogWrite(PWM_PIN,i); delay(15);} //(duration_m/255)
  else for (i = 0; i < 255; i++) {analogWrite(PWM_PIN,i); delay(15);}
}

/*
        CCCCCCCCCCCCCLLLLLLLLLLL                  OOOOOOOOO             CCCCCCCCCCCCCKKKKKKKKK    KKKKKKK
     CCC::::::::::::CL:::::::::L                OO:::::::::OO        CCC::::::::::::CK:::::::K    K:::::K
   CC:::::::::::::::CL:::::::::L              OO:::::::::::::OO    CC:::::::::::::::CK:::::::K    K:::::K
  C:::::CCCCCCCC::::CLL:::::::LL             O:::::::OOO:::::::O  C:::::CCCCCCCC::::CK:::::::K   K::::::K
 C:::::C       CCCCCC  L:::::L               O::::::O   O::::::O C:::::C       CCCCCCKK::::::K  K:::::KKK
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K:::::K K:::::K    ::::::
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K::::::K:::::K     ::::::
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K:::::::::::K      ::::::
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K:::::::::::K
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K::::::K:::::K
C:::::C                L:::::L               O:::::O     O:::::OC:::::C                K:::::K K:::::K
 C:::::C       CCCCCC  L:::::L         LLLLLLO::::::O   O::::::O C:::::C       CCCCCCKK::::::K  K:::::KKK ::::::
  C:::::CCCCCCCC::::CLL:::::::LLLLLLLLL:::::LO:::::::OOO:::::::O  C:::::CCCCCCCC::::CK:::::::K   K::::::K ::::::
   CC:::::::::::::::CL::::::::::::::::::::::L OO:::::::::::::OO    CC:::::::::::::::CK:::::::K    K:::::K ::::::
     CCC::::::::::::CL::::::::::::::::::::::L   OO:::::::::OO        CCC::::::::::::CK:::::::K    K:::::K
        CCCCCCCCCCCCCLLLLLLLLLLLLLLLLLLLLLLLL     OOOOOOOOO             CCCCCCCCCCCCCKKKKKKKKK    KKKKKKK
*/

void clock()
{

                              //TODO: should check if time has changed befgore printing!

  //VFDchar(1,30); //set VFD position.
  VFDsetpos(32);

   // print the hour, minute and second:

    unsigned long hours=((currentEpoch  % 86400L) / 3600)+GMT_plus; //calc the hour (86400 equals secs per day)
    if (DST) hours++; //daylight savings boolean is checked
    unsigned long minutes=((currentEpoch % 3600) / 60);
    unsigned long seconds= (currentEpoch % 60);

    if (hours>23) hours=hours-24; //offset check since GMT and DST offsets are added after modulo

    if(hours<10) VFDchar(0,'0'); //add leading '0' to hours lower than 10

    VFDstring(String(hours)); // print the hour

    VFDchar(0,':');


    if ( minutes < 10 ) VFDchar(0,'0'); //add leading '0' to minutes lower than 10
    VFDstring(String(minutes)); // print the minute (3600 equals secs per minute)

    VFDchar(0,':');


    if ( seconds < 10 ) VFDchar(0,'0'); //add leading '0' to seconds lower than 10
    VFDstring(String(seconds)); // print the second
}


/*
UUUUUUUU     UUUUUUUUDDDDDDDDDDDDD      PPPPPPPPPPPPPPPPP                                                                   iiii          tttt
U::::::U     U::::::UD::::::::::::DDD   P::::::::::::::::P                                                                 i::::i      ttt:::t
U::::::U     U::::::UD:::::::::::::::DD P::::::PPPPPP:::::P                                                                 iiii       t:::::t
UU:::::U     U:::::UUDDD:::::DDDDD:::::DPP:::::P     P:::::P                                                                           t:::::t
 U:::::U     U:::::U   D:::::D    D:::::D P::::P     P:::::P     wwwwwww           wwwww           wwwwwwwaaaaaaaaaaaaa   iiiiiiittttttt:::::ttttttt
 U:::::D     D:::::U   D:::::D     D:::::DP::::P     P:::::P      w:::::w         w:::::w         w:::::w a::::::::::::a  i:::::it:::::::::::::::::t     ::::::
 U:::::D     D:::::U   D:::::D     D:::::DP::::PPPPPP:::::P        w:::::w       w:::::::w       w:::::w  aaaaaaaaa:::::a  i::::it:::::::::::::::::t     ::::::
 U:::::D     D:::::U   D:::::D     D:::::DP:::::::::::::PP          w:::::w     w:::::::::w     w:::::w            a::::a  i::::itttttt:::::::tttttt     ::::::
 U:::::D     D:::::U   D:::::D     D:::::DP::::PPPPPPPPP             w:::::w   w:::::w:::::w   w:::::w      aaaaaaa:::::a  i::::i      t:::::t
 U:::::D     D:::::U   D:::::D     D:::::DP::::P                      w:::::w w:::::w w:::::w w:::::w     aa::::::::::::a  i::::i      t:::::t
 U:::::D     D:::::U   D:::::D     D:::::DP::::P                       w:::::w:::::w   w:::::w:::::w     a::::aaaa::::::a  i::::i      t:::::t
 U::::::U   U::::::U   D:::::D    D:::::D P::::P                        w:::::::::w     w:::::::::w     a::::a    a:::::a  i::::i      t:::::t    tttttt ::::::
 U:::::::UUU:::::::U DDD:::::DDDDD:::::DPP::::::PP                       w:::::::w       w:::::::w      a::::a    a:::::a i::::::i     t::::::tttt:::::t ::::::
  UU:::::::::::::UU  D:::::::::::::::DD P::::::::P                        w:::::w         w:::::w       a:::::aaaa::::::a i::::::i     tt::::::::::::::t ::::::
    UU:::::::::UU    D::::::::::::DDD   P::::::::P                         w:::w           w:::w         a::::::::::aa:::ai::::::i       tt:::::::::::tt
      UUUUUUUUU      DDDDDDDDDDDDD      PPPPPPPPPP                          www             www           aaaaaaaaaa  aaaaiiiiiiii         ttttttttttt
*/

void UDPwait(boolean ISSorNTP) //true if ISS, false if NTP.
{
while (!Udp.parsePacket())
  {
  delay(50);
  UDPretryDelay++;
  if (UDPretryDelay==100)  //if 5 seconds has passed without an answer
    {
      if(ISSorNTP) sendISSpacket(robottobox);
      else sendNTPpacket(timeServer);
      UDPretries++;
      UDPretryDelay=0;
//      VFDchar(0,'.');
    }
  if(UDPretries==10)
    {
      VFDclear();
      VFDstring("No UDP RX for 50+sec, giving up.");
      errorclock();
    }
  }
UDPretries=0;
UDPretryDelay=0;
}


/*
IIIIIIIIII   SSSSSSSSSSSSSSS    SSSSSSSSSSSSSSS      TTTTTTTTTTTTTTTTTTTTTTTXXXXXXX       XXXXXXX
I::::::::I SS:::::::::::::::S SS:::::::::::::::S     T:::::::::::::::::::::TX:::::X       X:::::X
I::::::::IS:::::SSSSSS::::::SS:::::SSSSSS::::::S     T:::::::::::::::::::::TX:::::X       X:::::X
II::::::IIS:::::S     SSSSSSSS:::::S     SSSSSSS     T:::::TT:::::::TT:::::TX::::::X     X::::::X
  I::::I  S:::::S            S:::::S                 TTTTTT  T:::::T  TTTTTTXXX:::::X   X:::::XXX
  I::::I  S:::::S            S:::::S                         T:::::T           X:::::X X:::::X    ::::::
  I::::I   S::::SSSS          S::::SSSS                      T:::::T            X:::::X:::::X     ::::::
  I::::I    SS::::::SSSSS      SS::::::SSSSS                 T:::::T             X:::::::::X      ::::::
  I::::I      SSS::::::::SS      SSS::::::::SS               T:::::T             X:::::::::X
  I::::I         SSSSSS::::S        SSSSSS::::S              T:::::T            X:::::X:::::X
  I::::I              S:::::S            S:::::S             T:::::T           X:::::X X:::::X
  I::::I              S:::::S            S:::::S             T:::::T        XXX:::::X   X:::::XXX ::::::
II::::::IISSSSSSS     S:::::SSSSSSSS     S:::::S           TT:::::::TT      X::::::X     X::::::X ::::::
I::::::::IS::::::SSSSSS:::::SS::::::SSSSSS:::::S           T:::::::::T      X:::::X       X:::::X ::::::
I::::::::IS:::::::::::::::SS S:::::::::::::::SS            T:::::::::T      X:::::X       X:::::X
IIIIIIIIII SSSSSSSSSSSSSSS    SSSSSSSSSSSSSSS              TTTTTTTTTTT      XXXXXXX       XXXXXXX
*/

unsigned long sendISSpacket(IPAddress& address)
{
  // set 4 bytes in the buffer to 0

  memset(packetBuffer, 0, 4);

  packetBuffer[0] = 'i';
  packetBuffer[1] = 's';
  packetBuffer[2] = 's';
  packetBuffer[3] = '?';

  Udp.beginPacket(address, 1337); //remote port: 1337
  Udp.write(packetBuffer,4); //push the 4 bytes
  Udp.endPacket();
}

/*
IIIIIIIIII   SSSSSSSSSSSSSSS    SSSSSSSSSSSSSSS      RRRRRRRRRRRRRRRRR   XXXXXXX       XXXXXXX
I::::::::I SS:::::::::::::::S SS:::::::::::::::S     R::::::::::::::::R  X:::::X       X:::::X
I::::::::IS:::::SSSSSS::::::SS:::::SSSSSS::::::S     R::::::RRRRRR:::::R X:::::X       X:::::X
II::::::IIS:::::S     SSSSSSSS:::::S     SSSSSSS     RR:::::R     R:::::RX::::::X     X::::::X
  I::::I  S:::::S            S:::::S                   R::::R     R:::::RXXX:::::X   X:::::XXX
  I::::I  S:::::S            S:::::S                   R::::R     R:::::R   X:::::X X:::::X    ::::::
  I::::I   S::::SSSS          S::::SSSS                R::::RRRRRR:::::R     X:::::X:::::X     ::::::
  I::::I    SS::::::SSSSS      SS::::::SSSSS           R:::::::::::::RR       X:::::::::X      ::::::
  I::::I      SSS::::::::SS      SSS::::::::SS         R::::RRRRRR:::::R      X:::::::::X
  I::::I         SSSSSS::::S        SSSSSS::::S        R::::R     R:::::R    X:::::X:::::X
  I::::I              S:::::S            S:::::S       R::::R     R:::::R   X:::::X X:::::X
  I::::I              S:::::S            S:::::S       R::::R     R:::::RXXX:::::X   X:::::XXX ::::::
II::::::IISSSSSSS     S:::::SSSSSSSS     S:::::S     RR:::::R     R:::::RX::::::X     X::::::X ::::::
I::::::::IS::::::SSSSSS:::::SS::::::SSSSSS:::::S     R::::::R     R:::::RX:::::X       X:::::X ::::::
I::::::::IS:::::::::::::::SS S:::::::::::::::SS      R::::::R     R:::::RX:::::X       X:::::X
IIIIIIIIII SSSSSSSSSSSSSSS    SSSSSSSSSSSSSSS        RRRRRRRR     RRRRRRRXXXXXXX       XXXXXXX
*/

// V \ 1 \ -2.8 \ 1406937446 \ SSE-37 \ 1406937446 \ SSE-37 \ 1406851222 \ ESE-10

void handle_ISS_udp()
{
// We've received a packet, read the data from it
    memset(packetBuffer, 0, NTP_PACKET_SIZE); //reset packet buffer
    int read_bytes=Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
//    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
//    VFDstring(" Read bytes: ");
//    VFDstring(String(read_bytes));
//    delay(500);
//    VFDclear();


if (packetBuffer[0]=='V') passVisible=true; //stringcount=7; //VISIBLE PASS
else if (packetBuffer[0]=='R') passVisible=false; //stringcount=6; //REGULAR PASS

else
  {
  VFDstring("Bad data from robottobox, aborting :(");
  errorclock();
  }

if (packetBuffer[2]=='1') DST = true;
else if (packetBuffer[2]=='0') DST = false;


byte *startp=packetBuffer;

//jump the visibility and DST indicator bytes and their respective \null chars

startp+=4;

char Epoch_TEMP[12]; //holds strings so they can be converted to a number

if (passVisible)
  {
    VFDclear();
    VFDstring("NEXT PASS IS VISIBLE!");
    delay(1500);
    VFDclear();

   //MAGNITUDE
   passMagnitude=String((char *)startp);
   while(*startp) startp++; //jump to the next string in the UDP packet
   startp++; //jump past '\0'

  }
else
  {
    VFDclear();
    VFDstring("Next pass not visible. ");
    delay(1500);
    VFDclear();
  }
    //START TIME:

    String((char *)startp).toCharArray(Epoch_TEMP, 11) ;
    passStartEpoch=(unsigned long)atol(Epoch_TEMP);


   // Serial.println(passStartEpoch);

    while(*startp) startp++; //jump to the next string in the UDP packet
    startp++;

    //START DIR:
    passStartDir=String((char *)startp);

    //PRINT
    VFDstring("Next pass start direction: ");
    VFDstring(passStartDir);
    delay(1000);
    VFDclear();


    while(*startp) startp++; //jump to the next string in the UDP packet
    startp++;

    //MAX TIME:
    String((char *)startp).toCharArray(Epoch_TEMP, 11) ;
    passMaxEpoch=(unsigned long)atol(Epoch_TEMP);

    while(*startp) startp++; //jump to the next string in the UDP packet
    startp++;

    //MAX DIR:
    passMaxDir=String((char *)startp);

    //PRINT:
    VFDstring("Next pass MAX direction: ");
    VFDstring(passMaxDir);
    delay(1000);
    VFDclear();


    while(*startp) startp++; //jump to the next string in the UDP packet
    startp++;

    //END TIME:
    String((char *)startp).toCharArray(Epoch_TEMP, 11) ;
    passEndEpoch=(unsigned long)atol(Epoch_TEMP);


    while(*startp) startp++; //jump to the next string in the UDP packet
    startp++;

    //END DIR:
    passEndDir=String((char *)startp);
    //PRINT:
    VFDstring("Next pass END direction: ");
    VFDstring(passEndDir);
    delay(1000);
    VFDclear();


    //SECS TO PASS:
    secs_to_next_pass=passStartEpoch-currentEpoch;
    VFDstring("SECONDS TO NEXT PASS: ");
    VFDstring(String(secs_to_next_pass));
    delay(2500);
    VFDclear();

    VFDstring("Duration: ");
    VFDstring(String((int)(passEndEpoch-passStartEpoch)));
    VFDstring(" seconds...");
    delay(2500);
    VFDclear();

    /*
    VFDstring("  SECONDS TIL PASS MAX: ");
    VFDstring(String((int)(passMaxEpoch-currentEpoch)));

    delay(300);
    VFDclear();

    VFDstring("  SECONDS TIL PASS END: ");
    VFDstring(String((int)(passEndEpoch-currentEpoch)));


    delay(300);
    VFDclear();
    */
  //memset(packetBuffer, 0, NTP_PACKET_SIZE); //reset buffer
}

/*
NNNNNNNN        NNNNNNNNTTTTTTTTTTTTTTTTTTTTTTTPPPPPPPPPPPPPPPPP        TTTTTTTTTTTTTTTTTTTTTTTXXXXXXX       XXXXXXX
N:::::::N       N::::::NT:::::::::::::::::::::TP::::::::::::::::P       T:::::::::::::::::::::TX:::::X       X:::::X
N::::::::N      N::::::NT:::::::::::::::::::::TP::::::PPPPPP:::::P      T:::::::::::::::::::::TX:::::X       X:::::X
N:::::::::N     N::::::NT:::::TT:::::::TT:::::TPP:::::P     P:::::P     T:::::TT:::::::TT:::::TX::::::X     X::::::X
N::::::::::N    N::::::NTTTTTT  T:::::T  TTTTTT  P::::P     P:::::P     TTTTTT  T:::::T  TTTTTTXXX:::::X   X:::::XXX
N:::::::::::N   N::::::N        T:::::T          P::::P     P:::::P             T:::::T           X:::::X X:::::X    ::::::
N:::::::N::::N  N::::::N        T:::::T          P::::PPPPPP:::::P              T:::::T            X:::::X:::::X     ::::::
N::::::N N::::N N::::::N        T:::::T          P:::::::::::::PP               T:::::T             X:::::::::X      ::::::
N::::::N  N::::N:::::::N        T:::::T          P::::PPPPPPPPP                 T:::::T             X:::::::::X
N::::::N   N:::::::::::N        T:::::T          P::::P                         T:::::T            X:::::X:::::X
N::::::N    N::::::::::N        T:::::T          P::::P                         T:::::T           X:::::X X:::::X
N::::::N     N:::::::::N        T:::::T          P::::P                         T:::::T        XXX:::::X   X:::::XXX ::::::
N::::::N      N::::::::N      TT:::::::TT      PP::::::PP                     TT:::::::TT      X::::::X     X::::::X ::::::
N::::::N       N:::::::N      T:::::::::T      P::::::::P                     T:::::::::T      X:::::X       X:::::X ::::::
N::::::N        N::::::N      T:::::::::T      P::::::::P                     T:::::::::T      X:::::X       X:::::X
NNNNNNNN         NNNNNNN      TTTTTTTTTTT      PPPPPPPPPP                     TTTTTTTTTTT      XXXXXXX       XXXXXXX
*/

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0

  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:

  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,48);
  Udp.endPacket();
}

/*
NNNNNNNN        NNNNNNNNTTTTTTTTTTTTTTTTTTTTTTTPPPPPPPPPPPPPPPPP        RRRRRRRRRRRRRRRRR   XXXXXXX       XXXXXXX
N:::::::N       N::::::NT:::::::::::::::::::::TP::::::::::::::::P       R::::::::::::::::R  X:::::X       X:::::X
N::::::::N      N::::::NT:::::::::::::::::::::TP::::::PPPPPP:::::P      R::::::RRRRRR:::::R X:::::X       X:::::X
N:::::::::N     N::::::NT:::::TT:::::::TT:::::TPP:::::P     P:::::P     RR:::::R     R:::::RX::::::X     X::::::X
N::::::::::N    N::::::NTTTTTT  T:::::T  TTTTTT  P::::P     P:::::P       R::::R     R:::::RXXX:::::X   X:::::XXX
N:::::::::::N   N::::::N        T:::::T          P::::P     P:::::P       R::::R     R:::::R   X:::::X X:::::X    ::::::
N:::::::N::::N  N::::::N        T:::::T          P::::PPPPPP:::::P        R::::RRRRRR:::::R     X:::::X:::::X     ::::::
N::::::N N::::N N::::::N        T:::::T          P:::::::::::::PP         R:::::::::::::RR       X:::::::::X      ::::::
N::::::N  N::::N:::::::N        T:::::T          P::::PPPPPPPPP           R::::RRRRRR:::::R      X:::::::::X
N::::::N   N:::::::::::N        T:::::T          P::::P                   R::::R     R:::::R    X:::::X:::::X
N::::::N    N::::::::::N        T:::::T          P::::P                   R::::R     R:::::R   X:::::X X:::::X
N::::::N     N:::::::::N        T:::::T          P::::P                   R::::R     R:::::RXXX:::::X   X:::::XXX ::::::
N::::::N      N::::::::N      TT:::::::TT      PP::::::PP               RR:::::R     R:::::RX::::::X     X::::::X ::::::
N::::::N       N:::::::N      T:::::::::T      P::::::::P               R::::::R     R:::::RX:::::X       X:::::X ::::::
N::::::N        N::::::N      T:::::::::T      P::::::::P               R::::::R     R:::::RX:::::X       X:::::X
NNNNNNNN         NNNNNNN      TTTTTTTTTTT      PPPPPPPPPP               RRRRRRRR     RRRRRRRXXXXXXX       XXXXXXX
*/
void handle_ntp()
{
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // VFDstring("Current unix time = ");

	// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;

    // subtract seventy years:
    currentEpoch = secsSince1900 - seventyYears;
    // print Unix time:

    lastmillis = millis();
    //Serial.println(currentEpoch);
    //VFDstring(String(currentEpoch));
}


/*
VVVVVVVV           VVVVVVVVFFFFFFFFFFFFFFFFFFFFFFDDDDDDDDDDDDD
V::::::V           V::::::VF::::::::::::::::::::FD::::::::::::DDD
V::::::V           V::::::VF::::::::::::::::::::FD:::::::::::::::DD
V::::::V           V::::::VFF::::::FFFFFFFFF::::FDDD:::::DDDDD:::::D
 V:::::V           V:::::V   F:::::F       FFFFFF  D:::::D    D:::::D
  V:::::V         V:::::V    F:::::F               D:::::D     D:::::D ::::::
   V:::::V       V:::::V     F::::::FFFFFFFFFF     D:::::D     D:::::D ::::::
    V:::::V     V:::::V      F:::::::::::::::F     D:::::D     D:::::D ::::::
     V:::::V   V:::::V       F:::::::::::::::F     D:::::D     D:::::D
      V:::::V V:::::V        F::::::FFFFFFFFFF     D:::::D     D:::::D
       V:::::V:::::V         F:::::F               D:::::D     D:::::D
        V:::::::::V          F:::::F               D:::::D    D:::::D  ::::::
         V:::::::V         FF:::::::FF           DDD:::::DDDDD:::::D   ::::::
          V:::::V          F::::::::FF           D:::::::::::::::DD    ::::::
           V:::V           F::::::::FF           D::::::::::::DDD
            VVV            FFFFFFFFFFF           DDDDDDDDDDDDD
*/

void VFDsetup()
{
VFD_data_pins[0] = 1; //D7 - used to be 9
VFD_data_pins[1] = 0; //D6 - used to be 8
VFD_data_pins[2] = 7; //D5
VFD_data_pins[3] = 6; //D4
VFD_data_pins[4] = 5; //D3
VFD_data_pins[5] = 4; //D2
VFD_data_pins[6] = 3; //D1
VFD_data_pins[7] = 2; //D0


//DATA PORT:

unsigned char pin;
for (pin=0; pin < 8; pin++)
{
pinMode (VFD_data_pins[pin], OUTPUT);
digitalWrite (VFD_data_pins[pin], LOW);
}

//CONTROL PINS
pinMode(WR, OUTPUT); //!WR
pinMode(A_0, OUTPUT); //A0
pinMode(RESET, OUTPUT); //RESET
pinMode(RD, OUTPUT); //!RD
pinMode(CS, OUTPUT); //!CS
pinMode(T0, OUTPUT); //T0

digitalWrite(WR, LOW);
digitalWrite(A_0, LOW);
digitalWrite(RESET, LOW);
digitalWrite(RD, HIGH);
digitalWrite(CS, LOW);
digitalWrite(T0, HIGH);
digitalWrite(WR,HIGH);

VFDreset();

VFDcursor(true);

VFDscrollMode(true);
}

void VFDreset()
{
  digitalWrite(RESET, HIGH);
  delay(100);
  digitalWrite(RESET, LOW);
  delay(500);
}

void VFDclear()
{
  VFDchar(0,'\r');
  VFDchar(0,'\n');
}

void VFDscrollMode(boolean onoff)
{
 if(onoff)  VFDchar(0,0x13); else VFDchar(0,0x11);
}

void VFDcursor(boolean onoff)
{
  if(onoff) VFDchar(0,0x17); //flashing carriage
  else VFDchar(0,0x16); //cursor off
}

void VFDsetpos(byte position) //0-40 decimal
{
 VFDchar(1,position);
}
/*
void VFDsmileyMake()
{
  VFDchar(0,0x1b); //ESC
  VFDchar(0,customcharposition);
  VFDchar(0,0b00011000);
  VFDchar(0,0b00010001);
  VFDchar(0,0b00010000);
  VFDchar(0,0b10000000);
  VFDchar(0,0b00111000);
//  VFDchar(0,customcharposition);  //print
}
*/

void VFDchar(int isCommand, unsigned char databyte)
{
  if(isCommand==1) digitalWrite(A_0,HIGH); else digitalWrite(A_0,LOW);
  digitalWrite(WR,LOW);
  delay(1);
  VFDsetDataport(databyte);
  delay(1);
  digitalWrite(WR,HIGH);
  delay(1);
}
/*
void VFDflashyString(String inputstring)
{
 VFDchar(0,0x06); //start of flashy string
 VFDstring(inputstring);
 VFDchar(0,0x07); //end of flashy string
}
*/

void VFDstring(String inputstring)
{
  int i=0;
  while (i<=inputstring.length())
  {
  byte checkbyte=inputstring[i+1]; //needs to be a byte to see non ascii unsigned stuff.
                                   //also skip the strange non ascii identifyer byte.
  switch (checkbyte)
    {
    case 166: //æ
      VFDchar(0,0x1c);
      VFDchar(0,0x7b);
      i++;
    break;

    case 184: //ø
      VFDchar(0,0x1c);
      VFDchar(0,0x7c);
      i++;
    break;

    case 165: //å
      VFDchar(0,0x1c);
      VFDchar(0,0x7d);
      i++;
    break;

    case 134: //Æ
      VFDchar(0,0x1c);
      VFDchar(0,0x5b);
      i++;
    break;

    case 152: //Ø
      VFDchar(0,0x1c);
      VFDchar(0,0x5c);
      i++;
    break;

    case 133: //Å
      VFDchar(0,0x1c);
      VFDchar(0,0x5d);
      i++;
    break;

    default:
      VFDchar(0,inputstring[i]);
    break;
    }
    i++;

  }
}

void VFDsetDataport(unsigned char byte_of_doom)
{
    for (unsigned char i = 0; i < 8; i++)
    {
     digitalWrite(VFD_data_pins[i], (byte_of_doom >> i) & 0x01);
    }
}
/*
void VFDdancingSmileyForever()
{
    VFDcursor(false);
    VFDsmileyMake();

    while(1)
    {
    for(int i=0;i<39;i++) { VFDchar(1,i); VFDchar(0,customcharposition); VFDchar(0,0x08); VFDchar(0,' ');}
    for(int i=41;i!=0;i--) { VFDchar(1,i); VFDchar(0,customcharposition); VFDchar(0,0x08); VFDchar(0,' ');}
    }

}
*/