//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                          MIDI Controller
//                           Version 0.08a
//
//
//

// -- not in use yet ---
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
//#include <IRremote.h>

LiquidCrystal_I2C lcd(0x27,16,2);


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

byte MChannel = 1;
byte FS1 = 1;
byte FS2 = 2;
byte FS3 = 3;
byte FS4 = 4;
byte FS5 = 5;
byte FS6 = 6;
byte FS7 = 7;
byte FS8 = 8;
byte FS9 = 9;
byte PEDAL = 10;



//IRrecv irrecv(IO9);
//decode_results results;


// Main Menu Strings
char* MenuItems[5] = { "",
                       "Set MIDI Channel", 
                       "Set Footswitches", 
                       "Calibrate Expr. ",
                       "Factory Reset   "
                      };

// Main Menu Strings
char* FootSwitches[11] = { "",
                           "Set FootSwitch 1", 
                           "Set FootSwitch 2", 
                           "Set FootSwitch 3", 
                           "Set FootSwitch 4", 
                           "Set FootSwitch 5", 
                           "Set FootSwitch 6", 
                           "Set FootSwitch 7", 
                           "Set FootSwitch 8", 
                           "Set FootSwitch 9", 
                           "Set Expr. Pedal ", 
                         };







void setup()
{
//  irrecv.enableIRIn(); // Start the receiver
  MChannel=EEPROM.read(0);
  FS1=EEPROM.read(1);
  FS2=EEPROM.read(2);
  FS3=EEPROM.read(3);
  FS4=EEPROM.read(4);
  FS5=EEPROM.read(5);
  FS6=EEPROM.read(6);
  FS7=EEPROM.read(7);
  FS8=EEPROM.read(8);
  FS9=EEPROM.read(9);
  PEDAL=EEPROM.read(10);
  lcd.init();
  pinMode(IO1,INPUT_PULLUP);
  pinMode(IO2,INPUT_PULLUP);
  pinMode(IO3,INPUT_PULLUP);
  pinMode(IO4,INPUT_PULLUP);
  pinMode(IO5,INPUT_PULLUP);
  pinMode(IO6,INPUT_PULLUP);
  pinMode(IO7,INPUT_PULLUP);
  pinMode(IO8,INPUT_PULLUP);
// WARNING: IO9 is the TSOP1838T IR Receiver
  pinMode(IO9,INPUT_PULLUP);  

  Serial.begin(31250);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    MiFaSol     ");
  lcd.setCursor(0,1);
  lcd.print("MIDI Controller ");
  delay(1500);
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
/*
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
*/
  byte Key = Keypress();
  if (Key != 0)
  {
    long tmpmillis=millis();    
    ChangeProgram(MChannel,Key);
    LCDNumber(0,4,Key);
    LCDText(1,"Done!");
    while ((millis()-tmpmillis<KeyDelay) && (Keypress()==Key));  // DELAY for key depress or time
    LCDText(1,"     ");
  }
    
  if (PreviousExpPedal != ExpPedal())
  {
    PreviousExpPedal = ExpPedal();
    LCDNumber(0,13,PreviousExpPedal);
    SendMidiCC(MChannel,0,(byte)PreviousExpPedal);
  }
}


byte Keypress()
{
  if ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==LOW))
  {
    // Enter menu
    MainMenu();
    return 0;
  }
  
  
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
  // or
  // Serial.write(0xB0 | (MidiChannel & 0x0F));
  Serial.write(CCNumber);
  Serial.write(Value);
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


void MainMenu()
{
  lcd.clear();
  lcd.print("Setup");
  lcd.setCursor(0,1);
  byte Menu=1;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(MenuItems[Menu]);
    byte tmp=Keypress();
    switch (tmp)
    {
      case 1:  // left
        Menu-=1;
        if (Menu<1)
          Menu=4;
          break;
      case 2:  // right
        Menu+=1;
        if (Menu>4)
          Menu=1;
          break;
      case 3:  // enter
        switch (Menu)
        {
          case 1: // Set midi channel
            SetupMIDIChannel();
            break;
          case 2: // set foot switches
            FootSwitchMenu();
            break;
          case 3: // calibrate expr. pedal
            break;
          case 4: // factory reset
            FactoryReset();
            break;
        }
        break;
      case 4:  // back-exit
        StayInside=false;
        break;
    }
  }
}

void SetupMIDIChannel()
{
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("MIDI Channel    ");
  lcd.setCursor(13,1);
  byte tmpChannel=MChannel;
  boolean StayInside=true;
  while (StayInside)
  {
    byte tmp=Keypress();
    switch (tmp)
    {
      case 1: // left
        if (tmpChannel==0)
          tmpChannel=17;
        else
          tmpChannel-=1;
        ShowMIDIChannel(tmpChannel);
        break;
      case 2: // right
        tmpChannel+=1;
        if (tmpChannel>17)
          tmpChannel=1;
        ShowMIDIChannel(tmpChannel);
        break;
      case 3: // enter
        MChannel=tmpChannel;
        EEPROM.write(0,MChannel);  // write to eeprom
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}


void ShowMIDIChannel(byte Channel)
{
  lcd.setCursor(13,1);
  if (Channel==17)
    lcd.print("ALL");
  else
    {
      if (Channel<10)
        lcd.print("0");
      lcd.print(Channel);
    }
}

void FactoryReset()
{
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("* PLEASE RESET *");
  EEPROM.write(0,1);
  EEPROM.write(1,0);
  EEPROM.write(2,1);
  EEPROM.write(3,2);
  EEPROM.write(4,3);
  EEPROM.write(5,4);
  EEPROM.write(6,5);
  EEPROM.write(7,6);
  EEPROM.write(8,7);
  EEPROM.write(9,8);
  EEPROM.write(10,9);
  while (true);
}


void FootSwitchMenu()
{
  byte Item=1;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(FootSwitches[Item]);
    byte tmp=Keypress();
    switch (tmp)
    {
      case 1: // left
        Item-=1;
        if (Item<1)
          Item=10;
        break;
      case 2: // right
        Item+=1;
        if (Item>10)
          Item=1;
        break;
      case 3: // enter
        SetupFootSwitch(Item);
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}

void SetupFootSwitch(byte Switch)
{
  lcd.setCursor(0,1);
  if ((Switch>=1) && (Switch<=9))
  {
    lcd.print("SW ");
    lcd.print(Switch);
    lcd.print(" - CC: [  ] ");
    // 0123456789012345
    // SW 1 - CC: [01]
  }
  else
    lcd.print("EXPR - CC: [  ] ");
  boolean StayInside=true;
  byte tmpCC=1;
  while (StayInside)
  {
    DisplayCC(tmpCC);
    byte tmp=Keypress();
    switch (tmp)
    {
      case 1: // left
        tmpCC-=1;
        if (tmpCC<1)
          tmpCC=17;
        break;
      case 2: // right
        tmpCC+=1;
        if (tmpCC>17)
          tmpCC=1;
        break;
      case 3: // enter
        EEPROM.write(Switch,(tmpCC-1));
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}

void DisplayCC(byte num)
{
  lcd.setCursor(12,1);
  if (num<11)
    lcd.print("0");
  lcd.print(num-1);
}
