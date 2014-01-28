//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                          MIDI Controller
//                           Version 2.57b
//
//
//
// ------- Some Documentation ---------------------------------------------------------------
//
//
// EEPROM Memory & Settings
// ------------------------
//
//            
//       Type        Value
//           \      / 
//            \    /  
// FootSwitch[10][1]
// 
// - Type can be:
//   1. CC Message
//   2. Program Change
//   3. Note On
//   4. Note Off
//
// - Value can be:
//   0..127
//
//
// EEPROM:
// 1 = MIDI Out Channel
// 2 = MIDI In Channel
// 3 - Backlight Timeout
// 31-40 = Type of message for Footswitch-31 (1..4)
// 41-50 = Value (0..127)


#define Version " Version 2.57b  "

#include <MIDI.h>                                     // http://sourceforge.net/projects/arduinomidilib/files/Releases/ - fortyseveneffects@gmail.com
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>                        // LiquidCrystal_I2C V2.0 - Mario H. / atmega@xs4all.nl
                                                      // http://www.xs4all.nl/~hmario/arduino/LiquidCrystal_I2C/LiquidCrystal_I2C.zip 

LiquidCrystal_I2C lcd(0x27,16,2);                     // 0x27 is the address of the I2C driver for the LCD Screen

byte NoteChar[8] = { B00100, B00110, B00101, B00101, B01100, B11100, B11000, B00000 };    // Custom character for displaying a music note

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

volatile byte PreviousExpPedal = 0;                   // Expression Pedal previous reading
volatile int MinExp = 0;                              // Expression Pedal Minimum reading
volatile int MaxExp = 1024;                           // Expression Pedal Maximum reading

volatile byte MIDIInChannel = 1;                      // MIDI Channel for RX (1..16)
volatile byte MIDIOutChannel = 17;                    // MIDI Channel for RX (1..16, 17=All Channels/OMNI)

volatile byte BacklightTimeout = 100;                 // Valid from 0..99 sec, 100 for Always On, 101 for Always Off.
volatile boolean NoBacklight = false;
volatile boolean AlwaysBacklight = false;

volatile byte FootSwitch[10][2];                      // Array for storing settings for foot switches

long ExpTimeOutMillis = 0;                            //
long FootswitchtTimeOutMillis = 0;                    // Storing millis for calculations
long BacklightTimeOutMillis = 0;                      //

#define LCDTimeOut  5000                              // Time to stay on LCD the pressed switch/exp. pedal value

char* MenuItems[7] = { "",                            // Main Menu Text
                       "MIDI In Channel ", 
                       "MIDI Out Channel", 
                       "Set Footswitches", 
                       "Calibrate Expr. ",
                       "Backlight Time  ",
                       "Factory Reset   "
                      };

char* FootSwitches[11] = { "",                        // Foot Switches Text
                           "FootSwitch 1    ", 
                           "FootSwitch 2    ", 
                           "FootSwitch 3    ", 
                           "FootSwitch 4    ", 
                           "FootSwitch 5    ", 
                           "FootSwitch 6    ", 
                           "FootSwitch 7    ", 
                           "FootSwitch 8    ", 
                           "FootSwitch 9    ", 
                           "Expr. Pedal     ", 
                         };



// --- Setup procedure --------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  Initialize();
  lcd.init();
  lcd.createChar(1,NoteChar);
  pinMode(IO1,INPUT_PULLUP);
  pinMode(IO2,INPUT_PULLUP);
  pinMode(IO3,INPUT_PULLUP);
  pinMode(IO4,INPUT_PULLUP);
  pinMode(IO5,INPUT_PULLUP);
  pinMode(IO6,INPUT_PULLUP);
  pinMode(IO7,INPUT_PULLUP);
  pinMode(IO8,INPUT_PULLUP);
  pinMode(IO9,INPUT_PULLUP);  
  MIDI.begin(MIDIInChannel);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    MiFaSol     ");
  lcd.setCursor(0,1);
  lcd.print("MIDI Controller ");
  delay(1500);
  lcd.setCursor(0,1);
  lcd.print(" (c)2014 Magla  ");
  delay(1500);
  lcd.setCursor(0,1);
  lcd.print(Version);
  delay(1000);
  BacklightTimeOutMillis=millis();
  if (NoBacklight)
    lcd.noBacklight();
  else
    lcd.backlight();
  ClearScreen();
}



// --- Main procedure ---------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  byte Key = Keypress();
  if (Key != 0)
  {
    SendCommand(Key);                              // SEND Command based on keypress    
    lcd.setCursor(0,1);
    lcd.print("FS ");
    lcd.print(Key);
    FootswitchtTimeOutMillis=millis();
    while (Keypress()==Key)
      delay(1);
    lcd.setCursor(0,0);
    lcd.print("Standby");
  }    
  if (PreviousExpPedal != ExpPedal())                                               // Exp. pedal is pressed, send the message
  {
    PreviousExpPedal = ExpPedal();
    lcd.setCursor(9,1);
    lcd.print("EXP");
    DisplayNumber(1,13,PreviousExpPedal);

    switch (FootSwitch[10][0])
    {
      case 1:
        MIDI.sendControlChange(FootSwitch[10][1],PreviousExpPedal,MIDIOutChannel);    
        break;
      case 2:
        MIDI.sendProgramChange(PreviousExpPedal,MIDIOutChannel);
        break;
      case 3:
        MIDI.sendNoteOn(FootSwitch[10][1],PreviousExpPedal,MIDIOutChannel);      
        break;
      case 4:
        MIDI.sendNoteOn(FootSwitch[10][1],PreviousExpPedal,MIDIOutChannel);      
        break;
    }
    ExpTimeOutMillis=millis();
  }    
  
  if (MIDI.read())                                                                  // Process MIDI Input
  {
    ProcessMIDIInput();
  }
  
  if (millis()-ExpTimeOutMillis>LCDTimeOut)
  {
    lcd.setCursor(9,1);
    lcd.print("       ");
  }
  if (millis()-FootswitchtTimeOutMillis>LCDTimeOut)
  {
    lcd.setCursor(0,1);
    lcd.print("       ");
  }  
  if ((!NoBacklight) && (!AlwaysBacklight))
    if (millis()-BacklightTimeOutMillis>(BacklightTimeout*1000))
    {
      lcd.noBacklight();
    }
}



// --- Detect and process footswitches/keypresses -----------------------------------------------------------------------------------------------------
byte Keypress()
{
  if ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==LOW))
  {
    BacklightCheck();
    while ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==LOW))    // Delay while switches are still pressed
      delay(1);
    // Enter menu
    MainMenu();
    return 0;
  }    
  if (digitalRead(IO1)==LOW)
  {
    BacklightCheck();
    return 1;
  }
  else
    if (digitalRead(IO2)==LOW)
    {
      BacklightCheck();
      return 2;
    }
    else
      if (digitalRead(IO3)==LOW)
      {
        BacklightCheck();
        return 3;
      }
      else
        if (digitalRead(IO4)==LOW)
        {
          BacklightCheck();
          return 4;
        }
        else
          if (digitalRead(IO5)==LOW)
          {
            BacklightCheck();
            return 5;
          }
          else
            if (digitalRead(IO6)==LOW)
            {
              BacklightCheck();
              return 6;
            }
            else
              if (digitalRead(IO7)==LOW)
              {
                BacklightCheck();
                return 7;
              }
              else
                if (digitalRead(IO8)==LOW)
                {
                  BacklightCheck();
                  return 8;
                }
                else
                  if (digitalRead(IO9)==LOW)
                  {
                    BacklightCheck();
                    return 9;
                  }
                  else
                    return 0;
}
  
  
  
// --- Get the Expr. pedal position ------------------------------------------------------------------------------------------------------------------- 
byte ExpPedal()
{
  byte tmp = map(analogRead(EXP),MinExp,MaxExp,0,127);  
  if (tmp>127)
    tmp=0;
  return tmp;
}



// --- Display Command --------------------------------------------------------------------------------------------------------------------------------
void DisplayCommandType(byte type)
{
  lcd.setCursor(0,0);
  lcd.print("       ");
  lcd.setCursor(0,0);
  switch (type)
  {
    case 1:
      lcd.print("CC ");
      break;
    case 2:
      lcd.print("PGM");
      break;
    case 3:
      lcd.write(1);
      lcd.print("ON");
      break;
    case 4:
      lcd.write(1);    
      lcd.print("OF");
      break;
  }
}



// --- Display Number as 000 at specific position --------------------------------------------------------------------------------
void DisplayNumber(byte line, byte pos, byte number)
{
  lcd.setCursor(pos,line);
  if (number<100)
    lcd.print("0");
  if (number<10)
    lcd.print("0");
  lcd.print(number);
}



// --- Main Menu Procedure -------------------------------------------------------------------------------------------------------
void MainMenu()
{  
  lcd.clear();
  lcd.print("Setup Menu");
  lcd.setCursor(0,1);
  byte Menu=1;
  boolean StayInside=true;
  while (Keypress()!=0) 
    delay(1);
  while (StayInside)
  {
    //         0123456789012345
    lcd.setCursor(0,0);    
    lcd.print("Setup Menu      ");
    lcd.setCursor(0,1);
    lcd.print(MenuItems[Menu]);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(200);
    switch (tmp)
    {
      case 4:  // back-exit
        StayInside=false;
        white (Keypress()==4)
          delay(1);
        break;     
      case 1:  // left
        Menu-=1;
        if (Menu<1)
          Menu=6;
          break;
      case 2:  // right
        Menu+=1;
        if (Menu>6)
          Menu=1;
          break;
      case 3:  // enter
        switch (Menu)
        {
          case 2: // Set midi out channel
            SetupMIDIChannel(1);
            break;
          case 1: // Set midi in channel
            SetupMIDIChannel(2);
            break;
          case 3: // set foot switches
            FootSwitchMenu();
            break;
          case 4: // calibrate expr. pedal
            CalibratePedal();
            break;
          case 5: // set backlight
            SetupBacklight();
            break;
          case 6: // factory reset
            FactoryReset();
            break;
        }
//        StayInside=false;
        break;
    }
  }
  ClearScreen();
}



// --- Setup Backlight -----------------------------------------------------------------------------------------------------------
void SetupBacklight()
{
  lcd.setCursor(0,0);
  lcd.print("Set Backlight   ");
  
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("Backlight       ");
  lcd.setCursor(13,1);
  byte tmpBacklight=BacklightTimeout;
  boolean StayInside=true;
  while (StayInside)
  {
    ShowBacklight(tmpBacklight);    
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpBacklight-=1;
        if (tmpBacklight<1)
          tmpBacklight=101;
        break;
      case 2: // right
        tmpBacklight+=1;
        if (tmpBacklight>101)
          tmpBacklight=1;
        break;
      case 3: // enter
        BacklightTimeout=tmpBacklight;
        EEPROM.write(3,BacklightTimeout);  // write to eeprom
        if (BacklightTimeout==100)
        {
          AlwaysBacklight=true;
          NoBacklight=false;
          lcd.backlight();
        }
        else
          if (BacklightTimeout==101)
          {
            AlwaysBacklight=false;
            NoBacklight=true;
            lcd.noBacklight();
          }
          else
          {
            lcd.backlight();
            AlwaysBacklight=false;
            NoBacklight=false;
            BacklightTimeOutMillis=millis();
          }
          StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Show Backlight Mode -------------------------------------------------------------------------------------------------------
void ShowBacklight(byte tmpBacklight)
{
  lcd.setCursor(10,1);
  if (tmpBacklight==100)
    lcd.print("ON    ");
  else
    if (tmpBacklight==101)
      lcd.print("OFF   ");   
    else
    {
      if (tmpBacklight<10)
        lcd.print("0");
      lcd.print(tmpBacklight);
      lcd.print(" sec");
    }
}



// --- Setup MIDI Channel --------------------------------------------------------------------------------------------------------
void SetupMIDIChannel(byte Mode)
{
  lcd.setCursor(0,0);
  lcd.print("Select Channel  ");

  lcd.setCursor(0,1);
  //           0123456789012345
  if (Mode==1)
    lcd.print("MIDI Out   [   ]");
  else
    lcd.print("MIDI In    [   ]");
  lcd.setCursor(13,1);
  byte tmpChannel;
  if (Mode==1)
    tmpChannel=MIDIOutChannel;
  else
  {
    if (MIDIInChannel==0)
      tmpChannel=17;
    else
      tmpChannel=MIDIInChannel;
  }
  boolean StayInside=true;
  while (StayInside)
  {
    ShowMIDIChannel(tmpChannel);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpChannel-=1;
        if (tmpChannel<1)
          if (Mode==1)
            tmpChannel=16;
          else
            tmpChannel=17;
        break;
      case 2: // right
        tmpChannel+=1;
        if (Mode==1)
        {
          if (tmpChannel>16)
            tmpChannel=1;
        }
        else
        {
          if (tmpChannel>17)
            tmpChannel=1;
        }
        break;
      case 3: // enter
        if (Mode==1)                       // 1 MIDI Out, 2 MIDI In
        {
          MIDIOutChannel=tmpChannel;
          EEPROM.write(1,MIDIOutChannel);  // write to eeprom
        }
        else
        {
          MIDIInChannel=tmpChannel;
          EEPROM.write(2,MIDIInChannel);
          if (MIDIInChannel==17)
            MIDIInChannel=0;
          MIDI.setInputChannel(MIDIInChannel);
        }
          StayInside=false;                  
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Show MIDI Channel ---------------------------------------------------------------------------------------------------------
void ShowMIDIChannel(byte Channel)
{
  lcd.setCursor(12,1);
  if (Channel==17)
    lcd.print("ALL");
  else
  {
    lcd.print(" ");
    if (Channel<10)
      lcd.print("0");
    lcd.print(Channel);
  }
}



// --- Factory Reset / Initialize to Default Settings ----------------------------------------------------------------------------
void FactoryReset()
{
  lcd.setCursor(0,0);
  lcd.print("    WARNING!    ");
  lcd.setCursor(0,1);
  lcd.print(" Are you sure?  ");
  boolean StayInside=true;
  while (StayInside)
  {
    byte tmp=Keypress();
    switch (tmp)
    {
      case 3: // enter
        WriteToMem(11,0);
        WriteToMem(13,1024);
        EEPROM.write(1,1);
        EEPROM.write(2,0);
        EEPROM.write(3,30);
        for (byte tSwitch=1; tSwitch<=9; tSwitch++)
        {    
          EEPROM.write(30+tSwitch,2);
          EEPROM.write(40+tSwitch,tSwitch);
        }
        EEPROM.write(40,1);
        EEPROM.write(50,7);
        lcd.setCursor(0,0);
        //         0123456789012345
        lcd.print("System Restored!");
        lcd.setCursor(0,1);
        lcd.print("** RESETTING! **");
        delay(3000);
        SoftReset();
        while (true);
      case 4:
        StayInside=false;
        break;
    }
  }
}



// --- Setup Footswitch Main Menu ------------------------------------------------------------------------------------------------
void FootSwitchMenu()
{
  lcd.setCursor(0,0);
  lcd.print("Select Switch   ");
  
  byte Item=1;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(FootSwitches[Item]);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
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
        ChooseFSOption(Item);
        StayInside=false;
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Setup Footswitch CC Message -----------------------------------------------------------------------------------------------
void SetupFootSwitchCC(byte Switch)
{
  lcd.setCursor(0,0);
  lcd.print("Select CC       ");
  
  lcd.setCursor(0,1);
  if ((Switch>=1) && (Switch<=9))
  {
    lcd.print("0");
    lcd.print(Switch);
    lcd.print(": CC     [   ]");
    // 0123456789012345
    // 01: CC     [001]
  }
  else
    lcd.print("EXP: CC    [   ]");
  boolean StayInside=true;
  byte tmpCC=FootSwitch[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNumber(1,12,tmpCC-1);
    if (tmpCC!=0)
      delay(150);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpCC-=1;
        if (tmpCC<1)
          tmpCC=128;
        break;
      case 2: // right
        tmpCC+=1;
        if (tmpCC>128)
          tmpCC=1;
        break;
      case 3: // enter
        FootSwitch[Switch-1][1]=tmpCC-1;
        FootSwitch[Switch-1][0]=1;
        EEPROM.write(40+Switch,(tmpCC-1));
        EEPROM.write(30+Switch,1);
        StayInside=false;
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}



// --- Setup Footswitch Patch ----------------------------------------------------------------------------------------------------
void SetupFootSwitchPatch(byte Switch)
{
  lcd.setCursor(0,0);
  lcd.print("Select Patch    ");
  
  lcd.setCursor(0,1);
  if (Switch<10)
  {
    lcd.print("0");
    lcd.print(Switch);
    lcd.print(": Patch  [   ]");
  }
  else
    lcd.print("EXP: Patch [   ]");
  boolean StayInside=true;
  byte tmpPatch=FootSwitch[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNumber(1,12,tmpPatch-1);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpPatch-=1;
        if (tmpPatch<1)
          tmpPatch=128;
        break;
      case 2: // right
        tmpPatch+=1;
        if (tmpPatch>128)
          tmpPatch=1;
        break;
      case 3: // enter
        EEPROM.write(Switch+40,(tmpPatch-1));
        EEPROM.write(Switch+30,2);
        FootSwitch[Switch-1][1]=tmpPatch-1;
        FootSwitch[Switch-1][0]=2;
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}



// --- Setup Footswitch Note On/Off Mode -----------------------------------------------------------------------------------------
void SetupFootSwitchNote(byte Switch, byte Mode)
{
  lcd.setCursor(0,0);
  lcd.print("Select Note     ");
  
  lcd.setCursor(0,1);
  if (Switch<10)
  {
    lcd.print("0");
    lcd.print(Switch);
    lcd.print(":  ");
  }
  else
    lcd.print("EXP: ");
  if (Mode==1)
  {
    lcd.write(1);
    lcd.print(" ON  [   ]");
  }
  else
  {
    lcd.write(1);
    lcd.print(" OFF [   ]");
  }
  // 0123456789012345
  // 01: NoteOn [001] 
  boolean StayInside=true;
  byte tmpNote=FootSwitch[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNumber(1,12,tmpNote-1);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpNote-=1;
        if (tmpNote<1)
          tmpNote=128;
        break;
      case 2: // right
        tmpNote+=1;
        if (tmpNote>128)
          tmpNote=1;
        break;
      case 3: // enter
        EEPROM.write(Switch+40,(tmpNote-1));
        if (Mode==1)
          EEPROM.write(Switch+30,3);
        else
          EEPROM.write(Switch+30,4);      
        FootSwitch[Switch-1][1]=tmpNote-1;          
        if (Mode==1)
          FootSwitch[Switch-1][0]=3;
        else
          FootSwitch[Switch-1][0]=4;        
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}



// --- Choose Footswitch Mode ----------------------------------------------------------------------------------------------------
void ChooseFSOption(byte Switch)
{
  lcd.setCursor(0,0);
  lcd.print("Select Message  ");
  lcd.setCursor(0,1);
  if (Switch==10)
    lcd.print("EXP");
  else
  {
    lcd.print("0");
    lcd.print(Switch);
    lcd.print(":");
  }
  lcd.print(" Type [     ]");
  boolean StayInside=true;
  byte tmpMode=FootSwitch[Switch-1][0];
  while (StayInside)
  {
    DisplaySwitchMode(tmpMode);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(200);
    switch (tmp)
    {
      case 1: // left
        tmpMode-=1;
        if (tmpMode<1)
          tmpMode=4;
        break;
      case 2: // right
        tmpMode+=1;
        if (tmpMode>4)
          tmpMode=1;
        break;
      case 3: // enter
        EEPROM.write(Switch+30,tmpMode);
        FootSwitch[Switch-1][0]=tmpMode;
        switch (tmpMode)
        {
          case 2:
            SetupFootSwitchPatch(Switch);
            break;
          case 1:
            SetupFootSwitchCC(Switch);
            break;
          case 3:
            SetupFootSwitchNote(Switch,1);      // Note On
            break;
          case 4:
            SetupFootSwitchNote(Switch,2);      // Note Off
            break;
        }
        StayInside=false;
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}



// --- Display to Screen Mode ----------------------------------------------------------------------------------------------------
void DisplaySwitchMode(byte num)
{
  lcd.setCursor(10,1);
  switch (num)
  {
    case 1:
      lcd.print("CC   ");
      break;
    case 2:
      lcd.print("PATCH");
      break;
    case 3:
      lcd.write(1);
      lcd.print(" ON ");
      break;
    case 4:
      lcd.write(1);
      lcd.print(" OFF");
      break;
  }
}



// --- Calibrate Exp. Pedal --------------------------------------------------------------------------------------------------------------------------
void CalibratePedal()
{
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("Press EXP. full ");
  lcd.setCursor(0,1);
  lcd.print("Raw      -     V");
  boolean StayInside=true;
  int tmpMinMax=1024;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayValuePedal(tmpMinMax);
    byte tmp=Keypress();
    switch (tmp)
    {
      case 3: // enter
        MaxExp = tmpMinMax;
        WriteToMem(13,MaxExp);
        StayInside=false;
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
  while (Keypress()!=0)
    delay(1);
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("Depress exp.    ");
  lcd.setCursor(0,1);
  lcd.print("Raw      -     V");
  StayInside=true;
  tmpMinMax=0;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayValuePedal(tmpMinMax);
    byte tmp=Keypress();
    switch (tmp)
    {
      case 3: // enter
        MinExp = tmpMinMax;
        WriteToMem(11,MinExp);
        StayInside=false;
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }      
}



// --- Display Exp. Pedal Value ----------------------------------------------------------------------------------------------------------------------
void DisplayValuePedal(int num)
{
  lcd.setCursor(4,1);
  if (num<1000)
    lcd.print("0");
  if (num<100)
    lcd.print("0");
  if (num<10)
    lcd.print("0");
  lcd.print(num);
  float t = (float)(num) * (5.0) / (1024.0);
  lcd.setCursor(11,1);
  lcd.print(t,2);
}



// --- Clear / Setup LCD Display ---------------------------------------------------------------------------------------------------------------------
void ClearScreen()
{
  lcd.clear();
  lcd.print("Standby         ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}  



// --- Write Integers to EEPROM procedure -------------------------------------------------------------------------------------------------------------
// Write numbers (0-65535) to EEPROM (using 2 bytes).
void WriteToMem(byte address, int number)
{
  int a = number/256;
  int b = number % 256;
  EEPROM.write(address,a);
  EEPROM.write(address+1,b);
}



// --- Read Integers from EEPROM procedure ------------------------------------------------------------------------------------------------------------
// Read numbers (0-65535) from EEPROM (using 2 bytes).
int ReadFromMem(byte Address)
{
  int NumA=EEPROM.read(Address);
  int NumB=EEPROM.read(Address+1);
  return NumA*256+NumB;
}



// --- Initialize Settings / Read from EEPROM ---------------------------------------------------------------------------------------------------------
void Initialize()
{
  MIDIOutChannel=EEPROM.read(1);
  MIDIInChannel=EEPROM.read(2);
  BacklightTimeout=EEPROM.read(3);  
  if (BacklightTimeout==100)
  {
    AlwaysBacklight = true;
    NoBacklight = false;
  }
  else
    if (BacklightTimeout==101)
    {
      AlwaysBacklight = false;
      NoBacklight = true;
    }
    else
    {
      AlwaysBacklight = false;
      NoBacklight = false;
    }
  if (EEPROM.read(2)==0)
    MIDIInChannel=MIDI_CHANNEL_OMNI;
  if ((MIDIOutChannel<1) || (MIDIOutChannel>16))
    MIDIOutChannel=1;
  MinExp=ReadFromMem(11);
  MaxExp=ReadFromMem(13);
  for (byte tSwitch=1; tSwitch<=10; tSwitch++)
  {    
    FootSwitch[tSwitch-1][0]=EEPROM.read(30+tSwitch);
    FootSwitch[tSwitch-1][1]=EEPROM.read(40+tSwitch);    
  }
}




// --- Send a command, based on pressed footswitch ----------------------------------------------------------------------------------------------------
void SendCommand(byte Switch)
{
  DisplayCommandType(FootSwitch[Switch-1][0]);
  DisplayNumber(0,4,(FootSwitch[Switch-1][1]));  
  switch (FootSwitch[Switch-1][0])
  {
    case 1:
      MIDI.sendControlChange((FootSwitch[Switch-1][1]),127,MIDIOutChannel);
      break;
    case 2:
      MIDI.sendProgramChange((FootSwitch[Switch-1][1]),MIDIOutChannel);
      break;
    case 3:
      MIDI.sendNoteOn((FootSwitch[Switch-1][1]),127,MIDIOutChannel);
      break;
    case 4:
      MIDI.sendNoteOn((FootSwitch[Switch-1][1]),0,MIDIOutChannel);
      break;
  }
}



// --- Software Reset Procedure --------------------------------------------------------------------------------------------------
void SoftReset()
{
  asm volatile ("  jmp 0");
}



// --- Check for Backlight -------------------------------------------------------------------------------------------------------
void BacklightCheck()
{
  if (NoBacklight)
    lcd.noBacklight();
  if (AlwaysBacklight)
    lcd.backlight();
  if ((!NoBacklight) && (!AlwaysBacklight))
  {
    lcd.backlight();
    BacklightTimeOutMillis=millis();
  }    
}



// --- Process MIDI Input Messages -----------------------------------------------------------------------------------------------
void ProcessMIDIInput()
{
}
