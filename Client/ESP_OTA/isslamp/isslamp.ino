#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//webserver for control messages
//'C' -> clock_only_control_state -> operate with clock only until 7am
//'B' -> blank_control_state -> operate with no display until 7am
//'R' -> reset -> resets device, thus enabling normal_operations_control_state

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

//OTA:
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include "VFD.h"
#define DISPLAY_SIZE 40
VFD VFD(D10,D9,D0,D1,D2,D3,D4,D5,D6,D7);

#define PIXEL_PIN   D8    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 2
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

//Neopixel stuff:
uint32_t Wheel(byte WheelPos);
void rainbowCycle(uint8_t wait);
void fade(bool upDown, uint8_t wait);


IPAddress robottobox(5,79,74,16); //IP address constructor
const char* NTP_hostName = "dk.pool.ntp.org"; //should init with a null-char termination.
IPAddress timeServer;//init empty IP adress container

// Initialize the wifi client libraries
//WiFiClient client;
WiFiUDP Udp;

//UDP (robottobox) stuff:
const unsigned int localPort = 1337;    // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 128; 		// NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE ]; //buffer to hold incoming and outgoing packets

//States enum:
enum machine_states { //default state should be "get_data"
	get_data,
	display_setup,
	countdown,
	regular_start,
	regular_underway,
	visible_start,
	visible_before_max,
	print_end_info,
	visible_after_max,
	end_of_pass,
  override_state, //the override state happens when not in normal_operations_control_state 
  overridden_state
};


machine_states state; //init the enum with the pass states.
boolean showClock = true;

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
unsigned int hh_to_next_pass;
unsigned int mm_to_next_pass;
unsigned int ss_to_next_pass;

boolean passVisible;
String passMagnitude;
String passStartDir;
String passMaxDir;
String passEndDir;

String displaybuffer="  ";
int temp_vfd_position;


void configModeCallback (WiFiManager *myWiFiManager) {
  VFD.clear();
  VFD.sendString("AP@");
  String ipString=String(WiFi.softAPIP()[0]) + "." + String(WiFi.softAPIP()[1]) + "." + String(WiFi.softAPIP()[2]) + "." + String(WiFi.softAPIP()[3]);
  VFD.sendString(ipString);

  
  /*
  display.clear();
  display.display();
  display.drawString(0, 10, "Connection failed");
  display.drawString(0, 20, "Creating accesspoint: ");
  display.drawString(0, 30, myWiFiManager->getConfigPortalSSID());
  display.drawString(0, 40, String(WiFi.softAPIP()));
  */
  }


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
  //Serial.begin(115200);
  WiFi.hostname("ISS_LAMP");
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  strip.begin();
  VFD.begin();
  VFD.scrollMode(false);

  delay(10);

    VFD.sendString("Reactor online.");
    delay(1000);
    VFD.clear();
    VFD.sendString("Sensors online.");
    delay(1000);
    VFD.clear();
    VFD.sendString("Weapons online.");
    delay(1000);
    VFD.clear();
    VFD.sendString("All systems nominal...");

  rainbowCycle(1);
  strip.setPixelColor(1, 0xFFFFFF);
  strip.setPixelColor(0, 0xFFFFFF);
  strip.show();

  //  PWM_ramp(true);
    VFD.clear();
    VFD.sendString("Awaiting wifi connection...");

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
    wifiManager.setConnectTimeout(10); //try to connect to known wifis ten seconds 
  
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "ISS_LAMP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("ISS_LAMP")) {
      VFD.clear();
      VFD.sendString("Wifi setup failed :(");
      delay(500);
      reset();
  
    }

  String ipString=String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
  VFD.clear();
  VFD.sendString("WiFi up!");
  VFD.sendString("  IPv4: ");
  VFD.sendString(ipString);
  //VFD.sendString(String(WiFi.localIP()));
  
  //OTA:
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("ISS_LAMP");
  
  ArduinoOTA.onStart([]() {
    VFD.clear();
    VFD.sendString("OTA Start");
    delay(500);
    VFD.clear();
    VFD.sendString("OTA Progress: ");
  });

  ArduinoOTA.onEnd([]() {
  	VFD.clear();
  	VFD.sendString("OTA End");
  	reset();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    VFD.setPos(14);
    VFD.sendString(String(progress / (total / 100)) + String("%"));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    VFD.clear();
    VFD.sendString( String("Error[") + String(error) + String("]: ") );
    if (error == OTA_AUTH_ERROR) VFD.sendString("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) VFD.sendString("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) VFD.sendString("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) VFD.sendString("Receive Failed");
    else if (error == OTA_END_ERROR) VFD.sendString("End Failed");
  });
  ArduinoOTA.begin();

  delay(1000);
  VFD.clear();
  VFD.sendString("HTTP server setup..");
  server.on("/", http_handle_root);
  server.on("/setmode", http_set_mode);
  server.onNotFound(http_handle_not_found);

  server.begin();
  delay(500);
  VFD.clear();
  VFD.sendString("HTTP server started.");


  Udp.begin(localPort);
  delay(2000); //give them a moment, for pity's sake

    VFD.clear();

    VFD.sendString("DNS resolve: dk.pool.ntp.org");
    lookup_ntp_ip();
    delay(1500);

    VFD.clear();
    VFD.sendString("NTP IP: ");
    VFD.sendString(String(timeServer[0])+"."+String(timeServer[1])+"."+String(timeServer[2])+"."+String(timeServer[3]));

    delay(1500);

    VFD.clear();
    ///////////////// QUERY NTP //////////////
    VFD.sendString("UDP TX -> NTP");
    sendNTPpacket(timeServer);
    UDPwait(false); //false = NTP
    VFD.sendString("  NTP RX!!");
    handle_ntp();

    delay(500);
    //PWM_ramp(false); //lights ramp down
    //digitalWrite(PWM_PIN,true);
    state=get_data;

    fade(false,10);


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
void loop() {

timekeeper();

statemachine();

ArduinoOTA.handle();

server.handleClient();


}

void statemachine(){


 switch(state)
  {
  case get_data:
        VFD.clear();
        delay(1000);
        sendISSpacket(robottobox); //ask robottobox for new iss data
        VFD.sendString("ISS TX -> Sardukar");
        UDPwait(true);
        delay(500);
        VFD.sendString("   RX! :D");
        delay(500);
        handle_ISS_udp();
        state=display_setup;
        break;

  case display_setup:
        VFD.clear();
        if (passVisible)
          {
            VFD.sendString("Visible pass T-"); //15
            VFD.flashyString("LOADING          LOADING");
          }
        else
      {
        //VFD.sendString("Next pass       T-");
        VFD.sendString("Next pass in:   T-"); //18
        VFD.flashyString("LOADING        LOADING");
      }

        state=countdown;
        break;

  case countdown:       //probably could be called default state..

        if(passVisible) VFD.setPos(15); //right after T-
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

        clock();

        if(currentEpoch+1>=passStartEpoch)
            {
                if(!passVisible) state=regular_start;
                else state=visible_start;
            }
        break;

  case regular_start: //regular pass start
        VFD.clear();
        VFD.sendString("Non-visible pass in progress.");
        fade(true,5);
        //PWM_ramp(true); //lights fade on
        //MAYBE ADD COUNTDOWN?

        state=regular_underway;
        break;

  case regular_underway: //regular pass in progress
        clock();
        if(currentEpoch+1>=passEndEpoch) state=end_of_pass;
        break;

  case visible_start: //visible pass start
        VFD.clear();
        VFD.scrollMode(true);
        VFD.sendString("VISIBLE PASS STARTING!");
        //PWM_ramp(true); //lights fade on
        rainbowCycle(1);
        fade(true,1);
        VFD.sendString(" M:"+passMagnitude);
        delay(500);
        VFD.sendString(" @"+passStartDir);
        delay(1500);
        VFD.scrollMode(false);
        //print info about upcoming pass max
        VFD.clear();
        displaybuffer = "Visible pass max @" + passMaxDir + " in ";
        VFD.sendString(displaybuffer);
        temp_vfd_position=displaybuffer.length();

        state = visible_before_max;
        break;

  case visible_before_max: //visible pass countdown to max
        VFD.setPos(temp_vfd_position);
        displaybuffer=String(int(passMaxEpoch-currentEpoch)) + " seconds"; //create a printable string from the time until pass max
        for(int i=DISPLAY_SIZE-(temp_vfd_position+displaybuffer.length());i>0;i--) displaybuffer+=" "; //append spaces to the string to match size of display
        VFD.sendString(displaybuffer);

        if(currentEpoch+1>=passMaxEpoch) state=print_end_info;
        break;

  case print_end_info: //visible pass print end info
        VFD.clear();
        displaybuffer = "Visible pass end @" + passEndDir + " in ";
        VFD.sendString(displaybuffer);
        temp_vfd_position=displaybuffer.length();

        state=visible_after_max;
        break;

  case visible_after_max: //visible pass countdown to end
        VFD.setPos(temp_vfd_position);
        displaybuffer=String(int(passEndEpoch-currentEpoch)) + " seconds"; //create a printable string from the time until pass end
        for(int i=DISPLAY_SIZE-(temp_vfd_position+displaybuffer.length());i>0;i--) displaybuffer+=" "; //append spaces to the string to match size of display
        VFD.sendString(displaybuffer);

        if (currentEpoch+1>=passEndEpoch) state=end_of_pass;
        break;

  case end_of_pass: //pass ended
        VFD.clear();
        VFD.sendString("End of pass.");
        //PWM_ramp(false); //lights fade off
        fade(false,10);
        state=get_data;
        break;

  case override_state:
       VFD.clear(); //display out
       for(int i=0; i<strip.numPixels(); i++) strip.setPixelColor(i,0); //lights out
       strip.show();
       state=overridden_state;
       break;

  case overridden_state: //state is exited in clock() @ 7am local time
        clock();
        break;
  }

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

//void(* resetFunc) () = 0; //declare reset function @ address 0

void clock()
{


    unsigned long hours=((currentEpoch  % 86400L) / 3600)+GMT_plus; //calc the hour (86400 equals secs per day)
    if (DST) hours++; //daylight savings boolean is checked
    unsigned long minutes=((currentEpoch % 3600) / 60);
    unsigned long seconds= (currentEpoch % 60);

    //reset from overridden_state at 7am local time.
    if(state == overridden_state && hours == 7 && minutes == 0 && seconds == 0) {
      state=get_data;
      showClock=true;
    }

    // HH:MM:SS
    if ( hours > 23 ) hours = hours-24; //offset check since GMT and DST offsets are added after modulo
    displaybuffer=""; //start out empty
    if( hours < 10 ) displaybuffer+="0"; //add leading '0' to hours lower than 10
    displaybuffer+=String(hours);
    displaybuffer+=":";

    if ( minutes < 10 ) displaybuffer+="0"; //add leading '0' to minutes lower than 10
    displaybuffer+=String(minutes);

    if(state != overridden_state){ //experimenting with how the clock looks in low-light mode
    displaybuffer+=":";
    if ( seconds < 10 ) displaybuffer+="0"; //add leading '0' to seconds lower than 10
    displaybuffer+=String(seconds);
    }

    if(showClock==true){
          VFD.setPos(DISPLAY_SIZE-displaybuffer.length()); //set at the far right end of display 
          VFD.sendString(displaybuffer);
    } 
}

void errorclock()
{
  static unsigned long epochAtError=currentEpoch;
  //PWM_ramp(false); //lights off
  //digitalWrite(PWM_PIN,true);
  //unsigned int error_seconds=0;

  VFD.setPos(1); //set VFD position.
  VFD.sendChar('!'); //print error indicator

  while(1)
  {
    while(millis()<lastmillis+1000) yield(); //WAIT FOR ABOUT A SECOND
    currentEpoch++;
//    currentEpoch+=((millis()-lastmillis)/1000); //add  a second or more to the current epoch
    lastmillis=millis();
    clock();
    ArduinoOTA.handle();
    server.handleClient();
    if (currentEpoch>epochAtError+30) reset(); //reset after 30 seconds
  }
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

void sendISSpacket(IPAddress& address)
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

void lookup_ntp_ip()
{
int retries=0;

    //TODO: ping current timeServer IP, to see it it is still valid. then return without changes.. 

    while( WiFi.hostByName(NTP_hostName, timeServer) !=1) //if it returns something other than 1 we're in trouble.
    {
      delay(2500);
      retries++;

      if(retries==20) //after 50 seconds
      {
        VFD.clear();
        VFD.sendString("  NTP_DNS");
        errorclock();
      }
    }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
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

void UDPwait(boolean ISSorNTP) //true if ISS, false if NTP.
{

int UDPretryDelay = 0;
int UDPretries = 0;

while (!Udp.parsePacket())
  {
  delay(100);
  UDPretryDelay++;
  if (UDPretryDelay==50)  //if 5 seconds has passed without an answer
    {
      //TODO: refresh NTP IP if no response for "a while"... 
      if(ISSorNTP) sendISSpacket(robottobox);
      else sendNTPpacket(timeServer);
      UDPretries++;
      UDPretryDelay=0;
      //VFD.sendChar('.');
    }
  if(UDPretries==10) //after 50 seconds
    {
      VFD.clear();
      VFD.sendString("  UDP_RX");
      if(ISSorNTP) VFD.sendString("_ISS");
      else VFD.sendString("_NTP");
      errorclock();
    }
  }

//UDPretries=0;
//UDPretryDelay=0;
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

//    The recieved data is formatted as follows: Each '\' represents a null byte (0b00000000)
//
//     V \ 1 \ -2.8 \ 1406937446 \ SSE-37 \ 1406937446 \ SSE-37 \ 1406851222 \ ESE-10
//     |   |     |        |          |         |           |          |          |
//     |   |     |        |          |         |           |          |          Direction at pass end.
//     |   |     |        |          |         |           |          |
//     |   |     |        |          |         |           |          Unix time at pass end.
//     |   |     |        |          |         |           |
//     |   |     |        |          |         |           Direction at pass maximum.
//     |   |     |        |          |         |
//     |   |     |        |          |         Unix time at pass maximum.
//     |   |     |        |          |
//     |   |     |        |          Direction at pass start.
//     |   |     |        |
//     |   |     |        Unix time at pass start.
//     |   |     |
//     |   |     Pass magnitude.
//     |   |
//     |   Daylight savings time: Assigns value to the 'DST' bool: 0=false, 1=true
//     |
//     Visibility: assigns value to the 'passVisible' bool: V=true, R=false


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

 VFD.clear();


if (packetBuffer[0]=='V') passVisible=true; //stringcount=7; //VISIBLE PASS
else if (packetBuffer[0]=='R') passVisible=false; //stringcount=6; //REGULAR PASS
else
  {
  VFD.sendString("  RX_ISS_DATA_FORMAT");
  errorclock();
  }

if (packetBuffer[2]=='1') DST = true;
else if (packetBuffer[2]=='0') DST = false;


byte *startp=packetBuffer; //point to the packetBuffer
startp+=4; //jump the visibility and DST indicator bytes and their respective \null chars

char Epoch_TEMP[12]; //holds epoch char arrays for atol() to convert them to a number.

if (passVisible)
  {
   VFD.sendString("Next pass is visible!  ");
   //MAGNITUDE
   passMagnitude=String((char *)startp);
   while(*startp) startp++; //jump to the next string in the UDP packet
   startp++; //jump past '\0'
  }

else VFD.sendString("Next pass not visible. ");

    delay(1500);
    VFD.clear();

    //START TIME:

    String((char *)startp).toCharArray(Epoch_TEMP, 11) ; //these will break when the epoch gains another digit (in ~272 years)
    passStartEpoch=(unsigned long)atol(Epoch_TEMP);

   // VFD.sendString(passStartEpoch);

    while(*startp) startp++; //jump pointer to the next \0 in the UDP packet
    startp++; //point at char after \0

    //START DIR:
    passStartDir=String((char *)startp);

    //PRINT
    if (passVisible)
    {
    VFD.sendString("Next pass start direction: ");
    VFD.sendString(passStartDir);
    delay(2000);
    VFD.clear();
  }

    while(*startp) startp++; //jump pointer to the next \0 in the UDP packet
    startp++; //point at char after \0

    //MAX TIME:
    String((char *)startp).toCharArray(Epoch_TEMP, 11) ;
    passMaxEpoch=(unsigned long)atol(Epoch_TEMP);

    while(*startp) startp++; //jump pointer to the next \0 in the UDP packet
    startp++; //point at char after \0

    //MAX DIR:
    passMaxDir=String((char *)startp);

    //PRINT:
    /*
    VFD.sendString("Next pass MAX direction: ");
    VFD.sendString(passMaxDir);
    delay(1000);
    VFD.clear();
  */

    while(*startp) startp++; //jump pointer to the next \0 in the UDP packet
    startp++; //point at char after \0

    //END TIME:
    String((char *)startp).toCharArray(Epoch_TEMP, 11) ;
    passEndEpoch=(unsigned long)atol(Epoch_TEMP);


    while(*startp) startp++; //jump pointer to the next \0 in the UDP packet
    startp++; //point at char after \0

    //END DIR:
    passEndDir=String((char *)startp);

    //PRINT:
    /*
    VFD.sendString("Next pass END direction: ");
    VFD.sendString(passEndDir);
    delay(1000);
    VFD.clear();
    */

    //SECS TO PASS:
    secs_to_next_pass=passStartEpoch-currentEpoch;
    VFD.sendString("Seconds to next pass: ");
    VFD.sendString(String(secs_to_next_pass));
    delay(1500);
    VFD.clear();

    VFD.sendString("Duration: ");
    VFD.sendString(String((int)(passEndEpoch-passStartEpoch)));
    VFD.sendString(" seconds...");

    delay(1500);
    VFD.clear();

/*

    VFD.sendString("  SECONDS TO PASS MAX: ");
    VFD.sendString(String((int)(passMaxEpoch-currentEpoch)));


    delay(300);
    VFD.clear();

    VFD.sendString("  SECONDS TO PASS END: ");
    VFD.sendString(String((int)(passEndEpoch-currentEpoch)));


    delay(300);
    VFD.clear();
    */
}



void timekeeper()
{

    while(millis()<lastmillis+1000) yield(); //WAIT FOR ABOUT A SECOND  - yield to allow ESP8266 background functions

    if(timekeeper_standalone_seconds>=1800) //drift seems to be about 5 seconds pr 20 minutes > 1 second in 4 minutes > check every 30minutes
        {
            timekeeper_standalone_seconds=0;
            lookup_ntp_ip(); //can call errorclock
            sendNTPpacket(timeServer);
            UDPwait(false); //can call errorclock
            handle_ntp();
            currentEpoch++; // meh.. calibration...
        }

    currentEpoch+=((millis()-lastmillis)/1000); //add  a second or more to the current epoch
    lastmillis=millis();
    timekeeper_standalone_seconds++;
    ntp_ip_refresh_seconds++;
    secs_to_next_pass=passStartEpoch-currentEpoch;

    hh_to_next_pass=(secs_to_next_pass  % 86400L) / 3600;
    mm_to_next_pass=(secs_to_next_pass % 3600) / 60;
    ss_to_next_pass=secs_to_next_pass % 60;

    /*if(ntp_ip_refresh_seconds>3600)
        {
            ntp_ip_refresh_seconds=0;
            lookup_ntp_ip(); //for every hour.
        }*/
}

void reset()
{
    VFD.clear();
    VFD.sendString("Reset in 3");
    delay(1000);
    VFD.backspace(1);
    VFD.sendChar('2');
    delay(1000);
    VFD.backspace(1);
    VFD.sendChar('1');
    delay(1000);
    VFD.backspace(5);
    VFD.sendString("!    ");
    ESP.restart();
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }

    strip.show();

    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void fade(bool upDown, uint8_t wait)
{
	int i,j;
	if(upDown)
	{
		for(j=0; j<255;j++)
		{
			for(i=0; i<strip.numPixels(); i++)
			{
				strip.setPixelColor(i,strip.Color(j,j,j));
			}
			strip.show();
			delay(wait);
		}
	}

	else
	{
		for(j=254; j>=0;j--)
		{
			for(i=0;i<strip.numPixels(); i++)
			{
				strip.setPixelColor(i,strip.Color(j,j,j));
			}
			strip.show();
			delay(wait);
		}
	}
}

void http_set_mode() {

for (uint8_t i=0; i < server.args(); i++){

    if(server.argName(i) == "B") {
        showClock=false;
        state=override_state;
        server.send(200, "text/plain", "Blank mode on.");
        return;
    }

    if(server.argName(i) == "C") {
        showClock=true;
        state=override_state;
        server.send(200, "text/plain", "Clock mode on.");
        return;
    }
  
    if(server.argName(i) == "R") {
        server.send(200, "text/plain", "RESET.");
        reset();
    }

}
  server.send(200,"text/plain", "OK");

}

void http_handle_root() {
  server.sendContent("HTTP/1.1 200 OK\r\n"); //send new p\r\nage
  server.sendContent("Content-Type: text/html\r\n");
  server.sendContent("\r\n");
  server.sendContent("<HTML>\r\n");
  server.sendContent("<HEAD>\r\n");
  server.sendContent("<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />\r\n");
  server.sendContent("<meta name='viewport' content='width=device-width' />\r\n");
  server.sendContent("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />\r\n");
  server.sendContent("<link rel='stylesheet' type='text/css' href='https://moore.dk/doorcss.css' />\r\n");
  server.sendContent("<TITLE>ISS LAMP</TITLE>\r\n");
  server.sendContent("</HEAD>\r\n");
  server.sendContent("<BODY>\r\n");
  server.sendContent("<H1>ISS LAMP</H1>\r\n");
  server.sendContent("<hr />\r\n");
  server.sendContent("<br />\r\n");
  if(state == overridden_state){
    server.sendContent("<H2>Currently overridden</H2>\r\n");
    if(showClock) server.sendContent("<a class=\"red\" href=\"/setmode?B=1\"\">Hide Clock</a><br><br><br>\r\n");
    else server.sendContent("<a href=\"/setmode?C=1\"\">Show Clock</a><br><br><br>\r\n");
    }
  else {
    server.sendContent("<H2>Operating normally</H2>\r\n");
    server.sendContent("<a href=\"/setmode?C=1\"\">Clock only mode</a><br><br><br>\r\n");
    server.sendContent("<a class=\"red\" href=\"/setmode?B=1\"\">Blank mode</a><br><br><br>\r\n");
  }
    
  server.sendContent("<a class=\"red\" href=\"/setmode?R=1\"\">RESET</a>\r\n");  
  server.sendContent("</BODY>\r\n");
  server.sendContent("</HTML>\r\n");
 }

void http_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}