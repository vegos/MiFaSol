#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display


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

volatile byte CC = 0x80;
volatile byte CCSelect = 1;
volatile byte PreviousExpPedal = 0;
volatile int KeyDelay = 50;                  // 50ms for keypress

void setup()
{
  lcd.init();
  pinMode(IO1,INPUT_PULLUP);
  pinMode(IO2,INPUT_PULLUP);
  pinMode(IO3,INPUT_PULLUP);
  pinMode(IO4,INPUT_PULLUP);
  pinMode(IO5,INPUT_PULLUP);
  pinMode(IO6,INPUT_PULLUP);
  pinMode(IO7,INPUT_PULLUP);
  pinMode(IO8,INPUT_PULLUP);
  pinMode(IO9,INPUT_PULLUP);  
  Serial.begin(31250);
  lcd.backlight();
  lcd.clear();
  //         0123456789012345
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
}

void loop()
{
  byte Key = Keypress();
  if (Key != 0)
  {
    long tmpmillis=millis();    
    SendProgram(Key);
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
  return map(analogRead(EXP),0,1024,0,127);
}


void SendProgram(byte patch)
{
  Serial.write(0xC0);
  Serial.write(patch);

}

void SendMidiCC(byte CC_data, byte midiCCselect, byte AnalogValue)
{
  Serial.write(CC_data);
  Serial.write(midiCCselect);
  Serial.write(AnalogValue);
}

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
