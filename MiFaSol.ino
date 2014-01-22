//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                          MIDI Controller
//                           Version 0.06a
//
//
//

// -- not in use yet ---
//#include <Wire.h> 
//#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

//LiquidCrystal_I2C lcd(0x20,16,2);


#define  IO1     5
#define  IO2     6
#define  IO3     7
#define  IO4     8
#define  IO5    13
#define  IO6    12
#define  IO7    11
#define  IO8    10
#define  IO9     9
#define  EXP    A0

volatile byte CC = 1;
volatile byte CCSelect = 1;
volatile byte PreviousExpPedal = 0;
volatile int KeyDelay = 50;                  // 50ms for keypress
volatile int Patch = 0;
volatile int Volume = 0;
volatile byte MChannel = 1;

IRrecv irrecv(IO9);
decode_results results;

void setup()
{
  irrecv.enableIRIn(); // Start the receiver

/*  lcd.init();
  pinMode(IO1,INPUT_PULLUP);
  pinMode(IO2,INPUT_PULLUP);
  pinMode(IO3,INPUT_PULLUP);
  pinMode(IO4,INPUT_PULLUP);
  pinMode(IO5,INPUT_PULLUP);
  pinMode(IO6,INPUT_PULLUP);
  pinMode(IO7,INPUT_PULLUP);
  pinMode(IO8,INPUT_PULLUP);
// WARNING: IO9 is the TSOP1838T IR Receiver
//  pinMode(IO9,INPUT_PULLUP);  
*/  
  Serial.begin(31250);
/*  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    MiFaSol     ");
  lcd.setCursor(0,1);
  lcd.print("MIDI Controller ");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print(" (c)2014, Magla ");
  delay(1500);
  lcd.clear();
  lcd.print("PGM:---  EXP:---");
  lcd.setCursor(0,1);
  lcd.print("");
  */
}

void loop()
{
  if (irrecv.decode(&results)) 
  {
    switch (results.value)
    {
      case 0x90:
        Patch+=1;
        if (Patch>127)
          Patch=127;
        ChangeProgram(MChannel, Patch);
        delay(100);
        break;
      case 0x890:
        Patch-=1;
        if (Patch<0)
          Patch=0;
        ChangeProgram(MChannel, Patch);
        delay(100);
        break;
      case 0x10:      // Key 1 on remote
        ChangeProgram(MChannel,0);
        break;
      case 0x810:     // Key 2 on remote
        ChangeProgram(MChannel,1);
        break;
      case 0x410:
        ChangeProgram(MChannel,2);
        break;
      case 0xc10:
        ChangeProgram(MChannel,3);
        break;
      case 0x210:
        ChangeProgram(MChannel,4);
        break;
      case 0xA10:
        ChangeProgram(MChannel,5);
        break;
      case 0x610:
        ChangeProgram(MChannel,6);
        break;
      case 0xe10:
        ChangeProgram(MChannel,7);
        break;
      case 0x110:
        ChangeProgram(MChannel,8);
        break;
      case 0x910:     // Key 0 on remote
        ChangeProgram(MChannel,9);
        break;
        
      case 0xc90:
        Volume-=1;
        if (Volume<0)
          Volume=0;
        SendMidiCC(MChannel,0,Volume);
        break;
      case 0x490:
        Volume+=1;
        if (Volume>127)
          Volume=127;
        SendMidiCC(MChannel,0,(byte)Volume);
        break;
    }
    irrecv.resume();
  }
/*
  byte Key = Keypress();
  if (Key != 0)
  {
    long tmpmillis=millis();    
    ChangeProgram(Key);
    LCDNumber(0,4,Key);
    LCDText(1,"Done!");
    while ((millis()-tmpmillis<KeyDelay) && (Keypress()==Key));  // DELAY for key depress or time
    LCDText(1,"     ");
  }
    
  if (PreviousExpPedal != ExpPedal())
  {
    PreviousExpPedal = ExpPedal();
    LCDNumber(1,13,PreviousExpPedal);
    SendMidiCC(CC,CCSelect,PreviousExpPedal);
  }
*/
}


byte Keypress()
{
  if (digitalRead(IO1)==LOW)
    return 1;
  else
    if (digitalRead(IO2)==LOW)
      return 2;
    else
      if (digitalRead(IO3)==LOW)
        return 3;
      else
        if (digitalRead(IO4)==LOW)
          return 4;
        else
          if (digitalRead(IO5)==LOW)
            return 5;
          else
            if (digitalRead(IO6)==LOW)
              return 6;
            else
              if (digitalRead(IO7)==LOW)
                return 7;
              else
                if (digitalRead(IO8)==LOW)
                  return 8;
                else
                  if (digitalRead(IO9)==LOW)
                    return 9;
                  else
                    return 0;
}
  
  
  
byte ExpPedal()
{
  // WARNING. R1 is not installed on PCB
  return map(analogRead(EXP),0,1024,0,127);
}



// Change Program (Patch).
void ChangeProgram(byte MidiChannel, byte Patch)
{
  // 0xC0 = Program Change / Channel 0
  // 0xBF + X = Program Change  @ Channel X
  Serial.write(0xBF+MidiChannel);
  Serial.write(Patch);

}



// Send a MIDI Control Change Message
void SendMidiCC(byte MidiChannel, byte CCNumber, byte Value)
{
  // 0xB0 = Control Change / Channel 0.
  // 0xAF + X = Control Change @ Channel X
  Serial.write(0xAF+MidiChannel);
  Serial.write(CCNumber);
  Serial.write(Value);
}



/*
void LCDNumber(byte line, byte pos, byte number)
{
  lcd.setCursor(pos,line);
  if (number<100)
    lcd.print("0");
  if (number<10)
    lcd.print("0");
  lcd.print(number);
}

void LCDText(byte line, char* text)
{
  lcd.setCursor(0,line);
  lcd.print(text);
}

*/
