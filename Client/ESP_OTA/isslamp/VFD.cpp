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

#include <Arduino.h>
#include "VFD.h"



VFD::VFD(/*int T0, int CS, int RD, int RESET,*/ int A_0, int WR, int VFD7, int VFD6, int VFD5, int VFD4, int VFD3, int VFD2, int VFD1, int VFD0)
{

//_RESET=RESET;
_A_0=A_0;
_WR=WR;

//create an array of pins for easy access with for loops:
_VFD_data_pins[0] = VFD0;
_VFD_data_pins[1] = VFD1;
_VFD_data_pins[2] = VFD2;
_VFD_data_pins[3] = VFD3;
_VFD_data_pins[4] = VFD4;
_VFD_data_pins[5] = VFD5;
_VFD_data_pins[6] = VFD6;
_VFD_data_pins[7] = VFD7;


//DATA PORT:

for (int pin=0; pin < 8; pin++)
{
pinMode (_VFD_data_pins[pin], OUTPUT);
digitalWrite (_VFD_data_pins[pin], LOW);
}


//CONTROL PINS
pinMode(_WR, OUTPUT); //!WR
pinMode(_A_0, OUTPUT); //A0
//pinMode(_RESET, OUTPUT); //RESET
//pinMode(RD, OUTPUT); //!RD
//pinMode(CS, OUTPUT); //!CS
//pinMode(T0, OUTPUT); //T0

//initial states:
digitalWrite(_A_0, LOW);
//digitalWrite(_RESET, LOW);
//digitalWrite(RD, HIGH);
//digitalWrite(CS, LOW);
//digitalWrite(T0, HIGH);
digitalWrite(_WR,HIGH);

}

void VFD::begin()
{
    VFDreset();

    cursorMode(VFD_CURSOR_OFF);

    scrollMode(true);

    clear();

}

void VFD::sendChar(unsigned char databyte)
{
  digitalWrite(_A_0,LOW); //set
  setDataportAndSend(databyte);
}

void VFD::backspace(unsigned int backspaces)
{
    for(int i=0;i<backspaces;i++) sendChar(0x08);
}

void VFD::sendString(String inputstring)
{
  int i=0;
  while (i<=inputstring.length())
  {
  byte checkbyte=inputstring[i+1]; //needs to be a byte to see non ascii unsigned stuff.
                                   //also skip the strange non ascii identifyer byte.
  switch (checkbyte)
    {
    case 166: //æ
      sendChar(0x1c);
      sendChar(0x7b);
      i++;
    break;

    case 184: //ø
      sendChar(0x1c);
      sendChar(0x7c);
      i++;
    break;

    case 165: //å
      sendChar(0x1c);
      sendChar(0x7d);
      i++;
    break;

    case 134: //Æ
      sendChar(0x1c);
      sendChar(0x5b);
      i++;
    break;

    case 152: //Ø
      sendChar(0x1c);
      sendChar(0x5c);
      i++;
    break;

    case 133: //Å
      sendChar(0x1c);
      sendChar(0x5d);
      i++;
    break;

    default:
      sendChar(inputstring[i]);
    break;
    }
    i++;

  }
}

void VFD::flashyString(String inputstring)
{
 sendChar(0x06); //start of flashy string
 sendString(inputstring);
 sendChar(0x07); //end of flashy string
}

void VFD::clear()
{
  sendChar('\r');
  sendChar('\n');
}

void VFD::setPos(byte position) //0-40 decimal
{
 command(position);
}

void VFD::scrollMode(boolean onoff)
{
  if(onoff)  sendChar(VFD_SCROLL_ON);
  else sendChar(VFD_SCROLL_OFF);
}

void VFD::cursorMode(byte cursormode)
{
  sendChar(cursormode);
}

void VFD::command(unsigned char commandbyte)
{
  digitalWrite(_A_0,HIGH);
  setDataportAndSend(commandbyte);
}

void VFD::VFDreset()
{
  /*digitalWrite(_RESET, HIGH);
  delay(100);
  digitalWrite(_RESET, LOW);
  delay(500);*/
  setDataportAndSend(VDF_FLICKERLESS_MODE);
}

void VFD::setDataportAndSend(unsigned char byte_of_doom)
{
    digitalWrite(_WR,LOW);
    delay(1);

    for (unsigned char i = 0; i < 8; i++)
    {
     digitalWrite(_VFD_data_pins[i], (byte_of_doom >> i) & 0x01);
    }

    delay(1);
    digitalWrite(_WR,HIGH);
    delay(1);

}



void VFD::makeSmiley()
{
  sendChar(0x1b); //ESC
  sendChar(SMILEY_POSITION);
  sendChar(0b00011000);
  sendChar(0b00010001);
  sendChar(0b00010000);
  sendChar(0b10000000);
  sendChar(0b00111000);
//  sendChar(customcharposition);  //print
}


void VFD::dancingSmileyForever()
{
    cursorMode(VFD_CURSOR_OFF);
    makeSmiley();

    while(1)
    {
    for(int i=0;i<41;i++) { setPos(i); sendChar(SMILEY_POSITION); sendChar(0x08); sendChar(' '); delay(25); }
    for(int i=40;i>=0;i--) { setPos(i); sendChar(SMILEY_POSITION); sendChar(0x08); sendChar(' '); delay(25); }
    }

}


