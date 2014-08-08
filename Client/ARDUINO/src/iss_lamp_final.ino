/*
ISS LAMP

This sketch gets ISS data from a python script on robottobox
using an Arduino Wiznet Ethernet shield.

and ALSO NTP time from the danish NTP pool, with dns lookup.

Todo:
VFD to class .. or not.. 800 lines of code isn't that bad.. is it?

*/

#define DISPLAY_SIZE 40

#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>
#include <VFD.h>



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
int GMT_plus = 1; //timezone offset from GMT - it should be possible to get the GMT offset from the server since it does geolocation anyways... hmm... nah.
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

//Statemachine state indicator:
unsigned int state=0;

//hardware setup:
int PWM_PIN=9;

//vfd constructor:
VFD VFD(A0,A1,A2,A3,A4,A5,1,0,7,6,5,4,3,2);

int temp_vfd_position = 0;
String displaybuffer="  ";





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


    VFD.begin();

    VFD.sendString("Power OK.");

    PWM_ramp(true);


    // start the Ethernet connection:
    if (Ethernet.begin(mac) == 0)
    {
        VFD.sendString("Failed to configure Ethernet using DHCP!");
        PWM_ramp(false);
        while(1); //dead end.
    }

    else
    {
        VFD.clear();
        VFD.sendString("Ethernet up! - IP: ");

        String localIpString="  "; //really weird.. string needs to be initialized with something in it to work properly...
        localIpString=String(Ethernet.localIP()[0]); //first byte

        //formatting for print:
        for(byte ip=1;ip<4;ip++){localIpString+='.'; localIpString+=String(Ethernet.localIP()[ip]);} //last 3 bytes
        VFD.sendString(localIpString);
        delay(1500);
    }

    Udp.begin(localPort);
    delay(1000); //give the ethernet shield some time.

    my_dns.begin(DNS_IP);

    VFD.clear();
    lookup_ntp_ip();
    VFD.sendString("DNS resolve: dk.pool.ntp.org");

    delay(1000);

    //transform IP byte array to string so that the debug window can tell user what ip is being used:
    static char ipstringbuf[16];
    sprintf(ipstringbuf, "%d.%d.%d.%d\0", timeServer[0], timeServer[1], timeServer[2], timeServer[3]);
    VFD.clear();
    VFD.sendString("NTP IP resolved: ");
    VFD.sendString(ipstringbuf);

    delay(1000);

    ///////////////// QUERY NTP //////////////
    VFD.sendString("   UDP TX -> NTP");
    sendNTPpacket(timeServer);
    UDPwait(false); //false = NTP
    VFD.sendString("  NTP RX!!");
    handle_ntp();

    delay(500);
    VFD.cursorMode(VFD_CURSOR_OFF);
    VFD.clear();

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
        VFD.sendString("ISS TX -> Robottobox");
        UDPwait(true);
        delay(500);
        VFD.sendString("  ISS RX!!");
        handle_ISS_udp();
        //state=1;
        break;

  case 1:
        VFD.clear();
        if (passVisible) VFD.sendString("Visible pass T-LOADING          LOADING"); //15
        else VFD.sendString("Regular pass    T-LOADING        LOADING");
        //state=2;
        break;

  case 2:       //probably could be called default state..
        if(passVisible) VFD.setPos(15);
        else VFD.setPos(18);
        displaybuffer ="";
        if(hh_to_next_pass<10) displaybuffer+="0";
        displaybuffer+=String(hh_to_next_pass);
        displaybuffer+=":";
        if(mm_to_next_pass<10) displaybuffer+="0";
        displaybuffer+=String(mm_to_next_pass);
        displaybuffer+=":";
        if(ss_to_next_pass<10) displaybuffer+="0";
        displaybuffer+=String(ss_to_next_pass);
        if (passVisible) displaybuffer+=" M:"+passMagnitude;

        VFD.sendString(displaybuffer);

        /*
        if(hh_to_next_pass<10) VFD.sendChar('0');
        VFD.sendString(String(hh_to_next_pass));
        VFD.sendChar(':');
        if(mm_to_next_pass<10) VFD.sendChar('0');
        VFD.sendString(String(mm_to_next_pass));
        VFD.sendChar(':');
        if(ss_to_next_pass<10) VFD.sendChar('0');
        VFD.sendString(String(ss_to_next_pass));
        if (passVisible)
        {
            VFD.sendString(" M:");
            VFD.sendString(passMagnitude);
        }
        */
        clock();
        break;

  case 3: //regular  pass
        VFD.clear();
        PWM_ramp(true); //lights fade on
        VFD.sendString("Non-visible pass in progress.");
        //MAYBE ADD COUNTDOWN?
        break;

  case 4:
        clock();
        break;

  case 5: //visible pass
        VFD.clear();

        VFD.scrollMode(true);

        VFD.sendString("VISIBLE PASS STARTING!");

        PWM_ramp(true); //lights fade on

        displaybuffer="      Magnitude: "+passMagnitude;
        VFD.sendString(displaybuffer);

        delay(500);

        displaybuffer="      Direction: "+passStartDir;
        VFD.sendString(displaybuffer);

        delay(500);

        displaybuffer="      Duration: "+String((int)(passEndEpoch-passStartEpoch))+" seconds...";
        VFD.sendString(displaybuffer);

        delay(1500);

        VFD.scrollMode(false);

        //print info about upcoming pass max
        VFD.clear();
        displaybuffer = "Visible pass max @" + passMaxDir + " in ";
        VFD.sendString(displaybuffer);
        temp_vfd_position=displaybuffer.length();
        break;

  case 6: //visible pass countdown to max
        VFD.setPos(temp_vfd_position);

        displaybuffer=String(int(passMaxEpoch-currentEpoch)) + " seconds"; //create a printable string from the time until pass max
        for(int i=DISPLAY_SIZE-(temp_vfd_position+displaybuffer.length());i>0;i--) displaybuffer+=" "; //append spaces to the string to match size of display
        VFD.sendString(displaybuffer);
        break;

  case 7: //visible pass print end info
        VFD.clear();
        displaybuffer = "Visible pass end @" + passEndDir + " in ";
        VFD.sendString(displaybuffer);
        temp_vfd_position=displaybuffer.length();
        break;

  case 8: //visible pass countdown to end
        VFD.setPos(temp_vfd_position);
        displaybuffer=String(int(passEndEpoch-currentEpoch)) + " seconds"; //create a printable string from the time until pass end
        for(int i=DISPLAY_SIZE-(temp_vfd_position+displaybuffer.length());i>0;i--) displaybuffer+=" "; //append spaces to the string to match size of display
        VFD.sendString(displaybuffer);
        break;

  case 9: //pass ended
        VFD.clear();
        VFD.sendString("End of pass.");
        PWM_ramp(false); //lights fade off
        VFD.clear();
        delay(1000);
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
        case 0: //get new pass
            state=1;
            break;

        case 1: //print T-
            state=2;
            break;

        case 2: //countdown and clock
            if(currentEpoch>=passStartEpoch)
                {
                if(!passVisible) state=3;
                else state=5;
                }
            break;

        case 3: //non visible pass start
            state=4;
            break;

        case 4: //non visible pass in progress
            if(currentEpoch>=passEndEpoch) state=8;
            break;

        case 5: //Visible pass start
            state = 6;
            break;

        case 6: //Visible pass in progress before max
            if(currentEpoch>=passMaxEpoch) state=7;
            break;

        case 7: //Visible pass at max (print end info)
            state=8;
            break;

        case 8: //Visible pass in progress intill end
            if (currentEpoch>=passEndEpoch) state=9;
            break;

        case 9: //End of pass
            state = 0;
            break;

        default:
            VFD.clear();
            VFD.sendString("statemachine b0rked!");
            errorclock();
            break;

    }
}

void lookup_ntp_ip(void)
{
    if(my_dns.getHostByName(NTP_hostName, timeServer) !=1)
    {
        VFD.clear();
        VFD.sendString("NTP DNS lookup failed");
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
  VFD.clear();
  VFD.setPos(1); //set VFD position.
  VFD.sendChar('!'); //print error indicator
  //VFD.sendChar(0x16); //cursor off
  VFD.cursorMode(VFD_CURSOR_OFF);

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

    VFD.setPos(DISPLAY_SIZE-8); //set at the far right end of display

    // print the hour, minute and second:

    unsigned long hours=((currentEpoch  % 86400L) / 3600)+GMT_plus; //calc the hour (86400 equals secs per day)
    if (DST) hours++; //daylight savings boolean is checked
    unsigned long minutes=((currentEpoch % 3600) / 60);
    unsigned long seconds= (currentEpoch % 60);

    if ( hours > 23 ) hours = hours-24; //offset check since GMT and DST offsets are added after modulo

    displaybuffer=""; //start out empty

    if( hours < 10 ) displaybuffer+="0"; //add leading '0' to hours lower than 10

    displaybuffer+=String(hours)+":";

    if ( minutes < 10 ) displaybuffer+="0"; //add leading '0' to minutes lower than 10

    displaybuffer+=String(minutes)+":";

    if ( seconds < 10 ) displaybuffer+="0"; //add leading '0' to seconds lower than 10

    displaybuffer+=String(seconds);

    VFD.sendString(displaybuffer);
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
//      VFD.sendChar('.');
    }
  if(UDPretries==10)
    {
      VFD.clear();
      VFD.sendString("No UDP RX for 50+sec, giving up.");
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
//    VFD.sendString(" Read bytes: ");
//    VFD.sendString(String(read_bytes));
//    delay(500);
//    VFD.clear();


if (packetBuffer[0]=='V') passVisible=true; //stringcount=7; //VISIBLE PASS
else if (packetBuffer[0]=='R') passVisible=false; //stringcount=6; //REGULAR PASS

else
  {
  VFD.sendString("Bad data from robottobox, aborting :(");
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
    VFD.clear();
    VFD.sendString("NEXT PASS IS VISIBLE!");
    delay(1500);
    VFD.clear();

   //MAGNITUDE
   passMagnitude=String((char *)startp);
   while(*startp) startp++; //jump to the next string in the UDP packet
   startp++; //jump past '\0'

  }
else
  {
    VFD.clear();
    VFD.sendString("Next pass not visible. ");
    delay(1500);
    VFD.clear();
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
    displaybuffer="Next pass start direction: "+passStartDir;
    VFD.sendString(displaybuffer);
    delay(1000);
    VFD.clear();


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
    displaybuffer="Next pass MAX direction: "+passMaxDir;
    VFD.sendString(displaybuffer);
    delay(1000);
    VFD.clear();


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
    displaybuffer="Next pass END direction: "+passEndDir;
    VFD.sendString(displaybuffer);
    delay(1000);
    VFD.clear();


    //SECS TO PASS:
    secs_to_next_pass=passStartEpoch-currentEpoch;
    displaybuffer="Seconds to next pass: "+String(secs_to_next_pass);
    VFD.sendString(displaybuffer);
    delay(1500);
    VFD.clear();

    displaybuffer="Duration: "+String((int)(passEndEpoch-passStartEpoch))+" seconds...";
    VFD.sendString(displaybuffer);
    delay(1500);
    VFD.clear();

    /*
    displaybuffer="  SECONDS TIL PASS MAX: "+String((int)(passMaxEpoch-currentEpoch));
    VFD.sendString(displaybuffer);

    delay(300);
    VFD.clear();

    displaybuffer="  SECONDS TIL PASS END: "+String((int)(passEndEpoch-currentEpoch));
    VFD.sendString(displaybuffer);


    delay(300);
    VFD.clear();
    */
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

	// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;

    // subtract seventy years:
    currentEpoch = secsSince1900 - seventyYears;

    lastmillis = millis();
}