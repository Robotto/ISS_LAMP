#include <VFD.h>

VFD myVFD(A0,A1,A2,A3,A4,A5,1,0,7,6,5,4,3,2);


void setup()
{

    myVFD.begin();

    myVFD.sendString("Tests commence in: 3");
     delay(1000);
    myVFD.backspace(1);
    myVFD.sendChar('2');

    delay(1000);
    myVFD.backspace(1);
    myVFD.sendChar('1');

    delay(1000);
    myVFD.clear();
    myVFD.sendString("NOW!");
}

void loop()
{

    delay(500);
    myVFD.clear();
    myVFD.sendString("Cursor test: flashy block: ");
    myVFD.cursorMode(VFD_CURSOR_FLASHING_BLOCK);

    delay(1500);
    myVFD.backspace(7);
    myVFD.sendString("Line: ");
    myVFD.cursorMode(VFD_CURSOR_FLASHING_UNDERLINE);

    delay(1500);
    myVFD.backspace(13);
    myVFD.cursorMode(VFD_CURSOR_NON_FLASHING_UNDERLINE);
    myVFD.sendString("Perpetual line: ");

    delay(1500);
    myVFD.backspace(16);
    myVFD.sendString("None...         ");
    myVFD.cursorMode(VFD_CURSOR_OFF);

    delay(1500);
    myVFD.clear();

    myVFD.sendString("Position test: ");
    delay(1000);
    myVFD.clear();
    myVFD.setPos(10);
    myVFD.sendString("10");
    delay(500);
    myVFD.setPos(25);
    myVFD.sendString("25");
    delay(500);
    myVFD.setPos(39);
    myVFD.sendString("39");
    delay(500);
    myVFD.setPos(0);
    myVFD.sendString("0");
    delay(500);
    myVFD.setPos(2);
    myVFD.sendString("2");
    delay(500);
    myVFD.setPos(1);
    myVFD.sendString("1");

    delay(1500);
    myVFD.clear();

    myVFD.scrollMode(true);
    myVFD.sendString("Scroll test: 123456789ABCDEFGHIJLKMNOPQRSTUVXYZÆØÅ");
    delay(500);
    myVFD.sendString(" S");
    delay(500);
    myVFD.sendString("C");
    delay(500);
    myVFD.sendString("R");
    delay(500);
    myVFD.sendString("O");
    delay(500);
    myVFD.sendString("L");
    delay(500);
    myVFD.sendString("L");
    delay(1500);

    myVFD.clear();
    myVFD.scrollMode(false);
    myVFD.sendString("SCROLL IS NOW OFF ... ?");
    delay(500);
    myVFD.sendString("123456789 123456789 123456789");
    delay(500);
    myVFD.sendString(" S");
    delay(500);
    myVFD.sendString("C");
    delay(500);
    myVFD.sendString("R");
    delay(500);
    myVFD.sendString("O");
    delay(500);
    myVFD.sendString("L");
    delay(500);
    myVFD.sendString("L ");
    for(int j=0;j<25;j++)
    {
    delay(100);
    myVFD.sendChar(0x09);
    }
    delay(1500);

    myVFD.clear();

    myVFD.flashyString("FLASHY TEST! :D :D :D  ");
    myVFD.sendString("no flash here!");


    delay(2500);

    myVFD.clear();
    myVFD.cursorMode(VFD_CURSOR_FLASHING_UNDERLINE);
    myVFD.sendString("End of tests.");

    delay(2500);

}