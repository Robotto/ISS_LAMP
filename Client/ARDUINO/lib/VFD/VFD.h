//include guard
#ifndef VFD_H
#define VFD_H

#include <Arduino.h>

#define VFD_CURSOR_NON_FLASHING_UNDERLINE 0x14
#define VFD_CURSOR_FLASHING_BLOCK 0x15
#define VFD_CURSOR_OFF 0x16
#define VFD_CURSOR_FLASHING_UNDERLINE 0x17

#define VFD_SCROLL_ON 0x13
#define VFD_SCROLL_OFF 0x11

#define SMILEY_POSITION 0xa0


//My VFD display library for the arduino.
class VFD {

public:

	//constructor:
	//The tricky thing here is to call the constructor with the corrent pin numbers, it all depends on how you've connected the hardware:
	//Name:			Pin # on VFD: 		Description:															Current hardware config on MY Arduino uno:

	//T0			25					Expendable - (if set to 5V = high)										A0
	//CS 			23					Expendable - Chip select (if set to 0V = low)							A1
	//RD 			21					Expendable - READ pin - pull low to read from VFD (if set to 5V = high)	A2

	//RESET 		20					Reset VFD on on high pulse												A3
	//A_0 			19					Data/Command pin (high for command, low for data)						A4
	//WR 			17					Write pin - pull low to write to VFD 									A5

	//D7			1 					Data pins. These transfer a data byte to the VFD where D7 is MSb.		1
	//D6			3																							0
	//D5			5																							7
	//D4			7																							6
	//D3			9																							5
	//D2			11																							4
	//D1			13																							3
	//D0			15																							2

	VFD(int T0, int CS, int RD, int RESET, int A_0, int WR, int D7, int D6, int D5, int D4, int D3, int D2, int D1, int D0);

    void begin(); //handles the timing sensitive init, since constructors can't use the delay function..

	//writing:
	void sendChar(unsigned char databyte);
	void backspace(unsigned int backspaces);
	void sendString(String inputstring);
	void flashyString(String inputstring);

	//util:
	void clear();
	void setPos(byte position); //0-40 decimal
	void scrollMode(boolean onoff); //scroll at end of screen? default on.
	void cursorMode(byte cursormode); //visible cursor on or off default on.

    void dancingSmileyForever();


private: //everything under here is private, only accessible inside the class
    void command(unsigned char commandbyte);
    void VFDreset();
    void setDataportAndSend(unsigned char byte_of_doom);
    void makeSmiley();

	//int _T0;
	//int _CS;
	//int _RD;
	int _RESET;
	int _A_0;
	int _WR;

	int _VFD_data_pins[8];

}; //end of class

#endif //include guard end if