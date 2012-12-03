/*
ISS LAMP  
 
This sketch gets ISS data from a py script on robottobox
using an Arduino Wiznet Ethernet shield.

and ALSO NTP time from the danish NTP pool.

Also: VFD!

Todo:
VFD to class
*/

//VFD STUFF:
int T0 = A0;
int CS = A1;
int RD = A2;
int RESET = A3;
int A_0 = A4;
int WR = A5;
byte customcharposition = 0xa0;
unsigned char VFD_data_pins[8];

void setup() {

  analogWrite(9, 255); //reset the PWM pin

  VFDsetup();
  
  VFDclear();



//  Serial.begin(9600);
  
  
  VFDstring("PWM TEST RUNNING");

//VFDchar(0,0x16); //cursor off.
}

int PWM_COUNTER=0;
unsigned long PWM_MILLIS=millis();
boolean PWM_UP_DOWN=true; //true=up, false = down
void loop()
{

//VFDdancingSmileyForever();

  PWM();

  
}

void PWM()
{
  if ((PWM_MILLIS+5)<=millis()) //has 8mS passed since last writeout?
  {
    if (PWM_UP_DOWN==true) PWM_COUNTER++; //increment 
    else PWM_COUNTER--; //decrement
    
    //PWM_COUNTER++;
    analogWrite(9,PWM_COUNTER); //Writeout
    PWM_MILLIS=millis(); //store new time

    if (PWM_COUNTER==255) 
    {
    PWM_UP_DOWN=false; //at the top? go down!
    delay(200);
    }
    else if (PWM_COUNTER==0)  PWM_UP_DOWN=true; //at the bottom? go up!

  }
}


//VFD STUFF:
void VFDsetup()
{
VFD_data_pins[0] = 1; //D7 - 9
VFD_data_pins[1] = 0; //D6 - 8
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

VFDchar(0,0x17); //flashing carriage

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

void VFDsetpos(byte position) //0-40 decimal
{
 VFDchar(1,position); 
}

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


void VFDchar(int isCommand, unsigned char databyte)
{
  if(isCommand==1) digitalWrite(A_0,HIGH); else digitalWrite(A_0,LOW);
  digitalWrite(WR,LOW);
  delay(10);
  VFDsetDataport(databyte);
  delay(10);
  digitalWrite(WR,HIGH);
  delay(30);
}

void VFDflashyString(String inputstring)
{
 VFDchar(0,0x06); //start of flashy string
 VFDstring(inputstring);
 VFDchar(0,0x07); //end of flashy string 
}

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

void VFDdancingSmileyForever()
{
    VFDchar(0,0x16); //cursor off
    VFDsmileyMake();
    
    while(1)
    {
    for(int i=0;i<39;i++) { VFDchar(1,i); VFDchar(0,customcharposition); VFDchar(0,0x08); VFDchar(0,' ');}
    for(int i=41;i!=0;i--) { VFDchar(1,i); VFDchar(0,customcharposition); VFDchar(0,0x08); VFDchar(0,' ');}
    }
}
