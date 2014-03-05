// -------------------------------------------------------------------------------------------------------------------------------
//
//                  __  ____ ______      _____       __               |  This project started as a MIDI Foot Controller for
//                 /  |/  (_) ____/___ _/ ___/____  / /               |  my MIDI-enabled effects.
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ /                |
//               / /  / / / __/ / /_/ /___/ / /_/ / /                 |  It's build on a custom PCB (contact me if you want one)
//              /_/  /_/_/_/    \__,_//____/\____/_/                  |  that's a simple Arduino-compatible board, based on an
//                                                                    |  ATmega328.
//          ©2014, Antonis Maglaras :: maglaras@gmail.com             |
//                         MIDI Controller                            |  More info: http://www.facebook.com/mifasolproject
//                    Version is .. still beta :)                     |  Code: http://github.com/vegos/MiFaSol
//                                                                    |  GitHub Micro Site: http://vegos.github.io/MiFaSol
//
// ------- Some Documentation ----------------------------------------------------------------------------------------------------
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
// - Note On sends a note with velocity 127.
// - Note Off sends a note with velocity 0.
// - Control Change sends fixed value of 127 (or variable for expression pedal).
// - MIDI In mode decodes incoming MIDI Messages (Program Change only).
//
//
// EEPROM:
// 1 = MIDI Channel
// 4 - Backlight Timeout
// 5 - MIDI In Mode
// 31-40 = Type of message for Footswitch-31 (1..4)
// 41-50 = Value (0..127)
//
//
// SWITCHES:
//
// 1 & 2: Setup Menu
//   On Setup menu:
//   1: Left
//   2: Right
//   3: Enter
//   4: Back/ESC/Cancel/Whatever
//
// 5 & 6: Previous Program / Patch 
// 5 & 7: Next Program / Patch 
//


#define Version "    Version 1.19    "                   // Current Version

#include <EEPROM.h>                                  
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>                           // LiquidCrystal_I2C V2.0 - Mario H. / atmega@xs4all.nl
                                                         // http://www.xs4all.nl/~hmario/arduino/LiquidCrystal_I2C/LiquidCrystal_I2C.zip 
#include <MemoryFree.h>                                  // http://playground.arduino.cc/Code/AvailableMemory
                                                      

LiquidCrystal_I2C lcd(0x20,20,4);                        // 0x27 is the address of the I2C driver for the LCD Screen

byte NoteChar[8] = { B00100, B00110, B00101, B00101, B01100, B11100, B11000, B00000 };    // Custom character for displaying a music note

#define  IO1        5                                    // Pin for Switch 1 -- Mandatory on my PCB
#define  IO2        6                                    // Pin for Switch 2 -- Mandatory on my PCB
#define  IO3        7                                    // Pin for Switch 3 -- Mandatory on my PCB
#define  IO4        8                                    // Pin for Switch 4 -- Mandatory on my PCB
#define  IO5       13                                    // Pin for Switch 5 -- Mandatory on my PCB
#define  IO6       12                                    // Pin for Switch 6 -- Mandatory on my PCB
#define  IO7       11                                    // Pin for Switch 7 -- Mandatory on my PCB
#define  IO8       10                                    // Pin for Switch 8 -- Mandatory on my PCB
#define  IO9        9                                    // Pin for Switch 9 -- Mandatory on my PCB
#define  EXP       A0                                    // Pin for Expr. Pedal -- Mandatory on my PCB

volatile byte PreviousExpPedal = 0;                      // Expression Pedal previous reading
volatile int MinExp = 0;                                 // Expression Pedal Minimum reading
volatile int MaxExp = 1024;                              // Expression Pedal Maximum reading

volatile byte MIDIInChannel = 1;                         // MIDI In Channel (1..17 -- 17 is Disabled MIDI In)
volatile byte MIDIOutChannel = 1;                        // MIDI Out Channel (1..16)
volatile byte BacklightTimeOut = 15;                     // Valid from 0..99 sec, 100 for Always On, 101 for Always Off.
volatile byte FootSwitch[10][2];                         // Array for storing settings for foot switches
volatile int Patch = 0;                                 // Current patch
volatile boolean ExprPedalMode = false;                  // Expr. Pedal is active

long ExpTimeOutMillis = 0;                               //
long FootswitchtTimeOutMillis = 0;                       // Storing millis for calculations
long BacklightTimeOutMillis = 0;                         //

#define LCDMessageTimeOut  15                            // Time to stay on LCD the pressed switch/exp. pedal value
const char* MenuItems[8] = { "MIDI Out Channel    ",    // Array with the Main Menu items
                             "MIDI In Channel     ",
                             "Setup Foot switches ", 
                             "Expr. Pedal Mode    ",
                             "Calibrate Exp. Pedal",
                             "Backlight Time      ",
                             "Version/Memory      ",
                             "Factory Reset       "
                           };



// --- Setup procedure -----------------------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(31250);                                   // 31250 bauds is the MIDI speed
  Initialize();                                          // Initialize Values/read them from EEPROM
  lcd.init();                                            // Initialize LCD 
  lcd.createChar(1,NoteChar);                            // Create & Assign a custom character (Music Note)
  BacklightCheck();                                      // Check for Backlight setting
  pinMode(IO1,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO2,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO3,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO4,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO5,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO6,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO7,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO8,INPUT_PULLUP);                             // Enable the internal pull-up resistor on pin
  pinMode(IO9,OUTPUT);                                   // Setup the IO9 as Output (Rx/TX LED)
  lcd.clear();                                    
  lcd.setCursor(0,0);
  lcd.print("      MiFaSol       ");
  lcd.setCursor(0,1);
  lcd.print("  MIDI Controller   ");
  lcd.setCursor(0,2);
  lcd.print("      (c)2014       ");
  lcd.setCursor(0,3);
  lcd.print(" Antonis Maglaras   ");
  delay(1500);
  lcd.setCursor(0,1);
  lcd.print(Version);
  delay(1000);
  BacklightTimeOutMillis=millis();
  if (BacklightTimeOut==101)
    lcd.noBacklight();
  else
    lcd.backlight();
  ClearScreen();
}



// --- Main procedure ------------------------------------------------------------------------------------------------------------
void loop()
{
  byte Key = Keypress();
  if (Key != 0)
  {
    TurnLEDOn();
    SendCommand(Key);                                    // SEND Command based on keypress    
    lcd.setCursor(0,3);
    lcd.print("Footswitch ");
    lcd.print(Key);
    FootswitchtTimeOutMillis=millis();
    TurnLEDOff();
    while (Keypress()==Key)
      delay(1);
//delay(250);
    ClearLine(3);
  }    

if (ExprPedalMode)
  if (PreviousExpPedal != ExpPedal())                    // Exp. pedal is pressed, send the message
  {
    PreviousExpPedal = ExpPedal();
    lcd.setCursor(0,2);
    lcd.print("Expression Pedal    ");
    DisplayNumber(2,17,PreviousExpPedal,3);
    TurnLEDOn();
    switch (FootSwitch[10][0])
    {
      case 1:
        MIDICC(FootSwitch[10][1],PreviousExpPedal);
        break;
      case 2:
        MIDIProgramChange(PreviousExpPedal);
        break;
      case 3:
        MIDINote(PreviousExpPedal,127,1);
        break;
      case 4:
        MIDINote(PreviousExpPedal,0,2);
        break;
    }
    TurnLEDOff();
    ExpTimeOutMillis=millis();
  }    
  
  if (MIDIInChannel!=17)
    if (Serial.available())
      CheckMIDI(); // Read midi   
    
  if (millis()-ExpTimeOutMillis > (LCDMessageTimeOut*1000))
  {
    ClearLine(2);
  }
  if (millis()-FootswitchtTimeOutMillis > (LCDMessageTimeOut*1000))
  {
    ClearLine(1);
  }  
  if (BacklightTimeOut<100)
  {
    if (millis()-BacklightTimeOutMillis>(BacklightTimeOut*1000))
      lcd.noBacklight();
  }
}




// --- Detect and process footswitches/keypresses --------------------------------------------------------------------------------
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
  else   

/*
// - testing -----------------------------------------------------------------------------
if ((digitalRead(IO5)==LOW) && (digitalRead(IO6)==LOW))
{
  Patch-=1;
  if (Patch<0)
    Patch=127;
  MIDIProgramChange(Patch);
  while ((digitalRead(IO5)==LOW) || (digitalRead(IO6)==LOW))
    delay(1);
}
else
if ((digitalRead(IO5)==LOW) && (digitalRead(IO7)==LOW))
{
  Patch+=1;
  if (Patch>127)
    Patch=0;
  MIDIProgramChange(Patch);
  while ((digitalRead(IO5)==LOW) || (digitalRead(IO7)==LOW))
    delay(1);
}
else
// ---------------------------------------------------------------------------------------
*/

    if ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==HIGH))
    {
      BacklightCheck();
      return 1;
    }
    else
      if ((digitalRead(IO2)==LOW) && (digitalRead(IO1)==HIGH))
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
                TurnLEDOn();
                Patch-=1;
                if (Patch<0)
                  Patch=127;
                MIDIProgramChange(Patch);                
                lcd.setCursor(0,1);
                lcd.print("Program        (   )");
                DisplayNumber(1,16,Patch,3);  
                FootswitchtTimeOutMillis=millis();
                BacklightCheck();
                TurnLEDOff();
                return 0;
//                return 6;
              }
              else
                if (digitalRead(IO7)==LOW)
                {
                  TurnLEDOn();
                  Patch+=1;
                  if (Patch>127)
                    Patch=0;
                  MIDIProgramChange(Patch);     
                  lcd.setCursor(0,1);
                  lcd.print("Program        (   )");
                  DisplayNumber(1,16,Patch,3);  
                  FootswitchtTimeOutMillis=millis();
                  BacklightCheck();
                  TurnLEDOff();
                  return 0;
//                  return 7;
                }
                else
                  if (digitalRead(IO8)==LOW)
                  {
                    BacklightCheck();
                    return 8;
                  }
                  else
//                    if (digitalRead(IO9)==LOW)
//                    {
//                      BacklightCheck();
//                      return 9;
//                    }
//                    else
                      return 0;
}
  
  
  
// --- Get the Expr. pedal position ----------------------------------------------------------------------------------------------
byte ExpPedal()
{
  byte tmp = map(analogRead(EXP),MinExp,MaxExp,0,127);  
  if (tmp>127)
    tmp=0;
  return tmp;
}



// --- Display Command -----------------------------------------------------------------------------------------------------------
void DisplayCommandType(byte type)
{
  ClearLine(1);
  lcd.setCursor(0,1);
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
void DisplayNumber(byte Line, byte Pos, byte Number, byte Decimals)
{
  lcd.setCursor(Pos,Line);
  if ((Decimals==4) && (Number<1000))
    lcd.print("0");
  if ((Number<100) && (Decimals>=3))
    lcd.print("0");
  if ((Number<10) && (Decimals>=2))
    lcd.print("0");
  lcd.print(Number);
}



// --- Main Menu Procedure -------------------------------------------------------------------------------------------------------
void MainMenu()
{  
  lcd.clear();
  lcd.print("Main Menu");
  lcd.setCursor(0,1);
  byte Menu=1;
  boolean StayInside=true;
  while (Keypress()!=0) 
    delay(1);
  while (StayInside)
  {
    lcd.setCursor(0,0);    
    //         01234567890123456789
    lcd.print("Main Menu           ");
    lcd.setCursor(0,1);
    lcd.print(MenuItems[Menu-1]);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
    switch (tmp)
    {
      case 4:  // back-exit
        StayInside=false;
        while (Keypress()==4)
          delay(1);
        break;     
      case 1:  // left
        Menu-=1;
        if (Menu<1)
          Menu=8;
          break;
      case 2:  // right
        Menu+=1;
        if (Menu>8)
          Menu=1;
          break;
      case 3:  // enter
        ClearLine(0);
        ClearLine(1);
        ClearLine(2);
        ClearLine(3);
        switch (Menu)
        {
          case 1: // Set MIDI Out Channel
            SetupMIDIChannel(16);
            break;
          case 2: // Set MIDI In Channel
            SetupMIDIChannel(17);
            break;
          case 3: // Set Footswitches
            FootSwitchMenu();
            break;
          case 4: // Expr. Pedal Mode
            SetupExprPedalMode();
            break;
          case 5: // Calibrate Expr. Pedal
            CalibratePedal();
            break;
          case 6: // Set Backlight
            SetupBacklight();
            break;
          case 7: // Show version & free memory
            ShowVersion();
            break;
          case 8: // Factory Reset
            FactoryReset();
            break;
        }
//        StayInside=false;
        break;
    }
  }
  ClearScreen();
}



// --- Show Enabled/Disable Mode -------------------------------------------------------------------------------------------------
void ShowEnabledDisable(boolean tmpMode)
{
  lcd.setCursor(11,1);
  if (tmpMode)
    lcd.print("Enabled ");
  else
    lcd.print("Disabled");  
}



// --- Setup Backlight -----------------------------------------------------------------------------------------------------------
void SetupBacklight()
{
  lcd.setCursor(0,0);
  //         01234567890123456789
  lcd.print("Setup Backlight     ");
  
  lcd.setCursor(0,1);
  //         01234567890123456789
  lcd.print("Backlight           ");
  lcd.setCursor(10,1);
  byte tmpBacklight=BacklightTimeOut;
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
        BacklightTimeOut=tmpBacklight;
        EEPROM.write(4,BacklightTimeOut);  // write to eeprom
        if (BacklightTimeOut==101)
          lcd.noBacklight();
        else
        {
          lcd.backlight();
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
    lcd.print("always ON ");
  else
    if (tmpBacklight==101)
      lcd.print("always OFF");   
    else
    {
      lcd.setCursor(10,1);
      lcd.print("[");
      if (tmpBacklight<10)
        lcd.print("0");
      lcd.print(tmpBacklight);
      lcd.print("]");
      lcd.setCursor(15,1);
      if (tmpBacklight==1)
        lcd.print(" sec ");
      else
        lcd.print(" secs");
    }
}



// --- Setup Backlight -----------------------------------------------------------------------------------------------------------
void SetupExprPedalMode()
{
  lcd.setCursor(0,0);
  //         01234567890123456789
  lcd.print("Set Expression Pedal");  
  lcd.setCursor(0,1);
  lcd.print("Status    [        ]");
  byte tmpExprPedalMode=ExprPedalMode;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(11,1);
    ShowEnabledDisable(tmpExprPedalMode);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpExprPedalMode=!(tmpExprPedalMode);
        break;
      case 2: // right
        tmpExprPedalMode=!(tmpExprPedalMode);
        break;
      case 3: // enter
        ExprPedalMode=tmpExprPedalMode;
        EEPROM.write(6,ExprPedalMode);  // write to eeprom
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Setup MIDI Channel --------------------------------------------------------------------------------------------------------
void SetupMIDIChannel(byte TotalChannels)
{
  lcd.setCursor(0,0);
  if (TotalChannels==16)
    lcd.print("Set MIDI Out Channel");
  else
    lcd.print("Set MIDI In Channel ");
  lcd.setCursor(0,1);
  byte tmpChannel = 0;
  if (TotalChannels==16)
  {
    lcd.print("MIDI Channel    [  ]");
    tmpChannel = MIDIOutChannel;
  }
  else
  {
    lcd.print("MIDI Channel   [   ]");
    tmpChannel = MIDIInChannel;
  }
    
//  byte tmpChannel = MIDIChannel;
  boolean StayInside=true;
  while (StayInside)
  {
    if (TotalChannels==16)
      DisplayNumber(1,17,tmpChannel,2);
    else
    {
      if (tmpChannel!=17)
        DisplayNumber(1,16,tmpChannel,3);
      else
      {
        lcd.setCursor(16,1);
        lcd.print("OFF");
      }
    }
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
    switch (tmp)
    {
      case 1: // left
        tmpChannel-=1;
          if (tmpChannel<1)
          {
            if (TotalChannels==16)
              tmpChannel=16;
            else
              tmpChannel=17;
          }
        break;
      case 2: // right
        tmpChannel+=1;
        if (TotalChannels==16)
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
        if (TotalChannels==16)
        {
          MIDIOutChannel=tmpChannel;          
          EEPROM.write(1,MIDIOutChannel);  // write to eeprom
        }
        else
        {
          MIDIInChannel=tmpChannel;
          EEPROM.write(2,MIDIInChannel);
        }
        StayInside=false;                  
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Factory Reset / Initialize to Default Settings ----------------------------------------------------------------------------
void FactoryReset()
{
  lcd.setCursor(0,0);
  lcd.print(" ***  WARNING!  *** ");
  lcd.setCursor(0,1);
  lcd.print("All settings will be");
  lcd.setCursor(0,2);
  lcd.print("erased-factory reset");
  lcd.setCursor(0,3);
  lcd.print("   Are you sure?    ");
  boolean StayInside=true;
  ClearLine(0);
  ClearLine(1);
  ClearLine(2);
  ClearLine(3);
  while (StayInside)
  {
    byte tmp=Keypress();
    switch (tmp)
    {
      case 3: // enter
        WriteToMem(11,0);
        WriteToMem(13,1024);
        EEPROM.write(1,1);
        EEPROM.write(2,1);
        EEPROM.write(4,15);
        EEPROM.write(5,1);
        EEPROM.write(6,0);
        for (byte tSwitch=1; tSwitch<=9; tSwitch++)
        {    
          EEPROM.write(30+tSwitch,2);
          EEPROM.write(40+tSwitch,tSwitch-1);
        }
        EEPROM.write(40,1);
        EEPROM.write(50,7);
        lcd.setCursor(0,0);
        lcd.print("** FACTORY RESET ** ");
        lcd.setCursor(0,1);
        lcd.print("Default settings are");
        lcd.setCursor(0,2);
        lcd.print("      restored!     ");
        lcd.setCursor(0,3);
        lcd.print("  ** RESETTING! **  ");
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
  lcd.print("Select Foot Switch  ");
  
  byte Item=1;
  boolean StayInside=true;
  while (StayInside)
  {
    lcd.setCursor(0,1);
    if (Item<10)
    { //         01234567890123456789
      lcd.print("Footswitch ");
      lcd.print(Item);
      lcd.print("        ");
    }
    else         
      lcd.print("Expression Pedal    ");
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
    switch (tmp)
    {
      case 1: // left
        Item-=1;
        if (Item<1)
          Item=10;
        if (Item==7)
          Item=5;
        if (Item==6)
          Item=5;
        if (Item==9)
          Item=8;
        break;
      case 2: // right
        Item+=1;
        if (Item>10)
          Item=1;
        if (Item==6)
          Item=8;
        if (Item==7)
          Item=8;
        if (Item==9)
          Item=10;
        break;
      case 3: // enter
        ChooseFSOption(Item);
        StayInside=false;
        break;
      case 4: // back
        ClearLine(2);
        ClearLine(3);
        StayInside=false;
        break;
    }
  }
}



// --- Display Selected Mode -----------------------------------------------------------------------------------------------------
void DisplayMode(byte Mode)
{
  switch (Mode)
  {
    case 1:
      lcd.print("CC   ");
      break;
    case 2:
      lcd.print("Patch");
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



// --- Setup Specific Footswitch -------------------------------------------------------------------------------------------------
void SetupFootSwitchAll(byte Switch, byte Mode)
{
  lcd.setCursor(0,0);
  lcd.print("Select ");
  DisplayMode(Mode);
  lcd.print("        ");
  
  lcd.setCursor(0,1);
  if ((Switch>=1) && (Switch<=8))
  {
    lcd.print("Switch ");
    lcd.print(Switch);
    lcd.print(" ");
    DisplayMode(Mode);
    lcd.print(" [   ]");
  }
  else
  {
    lcd.print("ExpPedal ");
    DisplayMode(Mode);
    lcd.print(" [   ]");
  }
  boolean StayInside=true;
  byte tmpSetting=FootSwitch[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNumber(1,16,tmpSetting-1,3);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(150);
    switch (tmp)
    {
      case 1: // left
        tmpSetting-=1;
        if (tmpSetting<1)
          tmpSetting=128;
        break;
      case 2: // right
        tmpSetting+=1;
        if (tmpSetting>128)
          tmpSetting=1;
        break;
      case 3: // enter
        FootSwitch[Switch-1][1]=tmpSetting-1;
//        FootSwitch[Switch-1][0]=1;
        EEPROM.write(40+Switch,(tmpSetting-1));
//        EEPROM.write(30+Switch,1);
        StayInside=false;
        break;
      case 4: // back
        ClearLine(2);
        ClearLine(3);      
        StayInside=false;
        break;
    }
  }    
}



// --- Choose Footswitch Mode ----------------------------------------------------------------------------------------------------
void ChooseFSOption(byte Switch)
{
  lcd.setCursor(0,0);
  //         01234567890123456789
  lcd.print("Select MIDI MEssage ");
  lcd.setCursor(0,1);
  if (Switch==10)
    lcd.print("Expression Pedal    ");
  else
  {
    lcd.print("Foot Switch ");
    lcd.print(Switch);
  }
  lcd.setCursor(0,2);
  lcd.print("Type [     ]");
  boolean StayInside=true;
  byte tmpMode=FootSwitch[Switch-1][0];
  while (StayInside)
  {
    lcd.setCursor(6,2);
    DisplayMode(tmpMode);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
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
        ClearLine(2);
        EEPROM.write(Switch+30,tmpMode);
        FootSwitch[Switch-1][0]=tmpMode;
        switch (tmpMode)
        {
          case 2:
            SetupFootSwitchAll(Switch,2);
            break;
          case 1:
            SetupFootSwitchAll(Switch,1);
            break;
          case 3:
            SetupFootSwitchAll(Switch,3);
            break;
          case 4:
            SetupFootSwitchAll(Switch,4);
            break;
        }
        StayInside=false;
        break;
      case 4: // back
        ClearLine(2);
        ClearLine(3);      
        StayInside=false;
        break;
    }
  }    
}



// --- Calibrate Exp. Pedal ------------------------------------------------------------------------------------------------------
void CalibratePedal()
{
  lcd.setCursor(0,0);
  lcd.print("Heel Down (Lowest)  ");
  lcd.setCursor(0,1);
  lcd.print("Value         [    ]");
  boolean StayInside=true;
  int tmpMinMax=1024;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayNumber(1,15,tmpMinMax,4);
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
  while (Keypress()!=0)
    delay(1);
  lcd.setCursor(0,0);
  lcd.print("Toe Down (Highest)  ");
  lcd.setCursor(0,1);
  lcd.print("Value         [    ]");
  StayInside=true;
  tmpMinMax=0;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayNumber(1,15,tmpMinMax,4);
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
}



// --- Clear / Setup LCD Display -------------------------------------------------------------------------------------------------
void ClearScreen()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("     Magla MIDI     ");
  lcd.setCursor(0,1);
  if (MIDIInChannel!=17)
  {      //    01234567890123456789
    lcd.print("Program        (   )");
    DisplayNumber(1,16,Patch,3);  
  }
}  



// --- Write Integers to EEPROM procedure (using 2 bytes) ------------------------------------------------------------------------
void WriteToMem(byte address, int number)
{
  EEPROM.write(address,number/256);
  EEPROM.write(address+1,number % 256);
}



// --- Read Integers from EEPROM procedure (using 2 bytes) -----------------------------------------------------------------------
int ReadFromMem(byte Address)
{
  return EEPROM.read(Address)*256+EEPROM.read(Address+1);
}



// --- Initialize Settings / Read from EEPROM ------------------------------------------------------------------------------------
void Initialize()
{
  BacklightTimeOut=EEPROM.read(4);  
  MIDIOutChannel=EEPROM.read(1);
  MIDIInChannel=EEPROM.read(2);
  if (EEPROM.read(6)==1)
  {
    ExprPedalMode=true;
  }
  else
  {
    ExprPedalMode=false;
  }
  MinExp=ReadFromMem(11);
  MaxExp=ReadFromMem(13);
  for (byte tSwitch=1; tSwitch<=10; tSwitch++)
  {    
    FootSwitch[tSwitch-1][0]=EEPROM.read(30+tSwitch);
    FootSwitch[tSwitch-1][1]=EEPROM.read(40+tSwitch);    
  }
}



// --- Send a command, based on specific switch ----------------------------------------------------------------------------------
void SendCommand(byte Switch)
{
  DisplayCommandType(FootSwitch[Switch-1][0]);
  DisplayNumber(1,6,(FootSwitch[Switch-1][1]),3);  
  switch (FootSwitch[Switch-1][0])
  {
    case 1:
      MIDICC(FootSwitch[Switch-1][1],127);
      break;
    case 2:
      MIDIProgramChange(FootSwitch[Switch-1][1]);
      break;
    case 3:
      MIDINote(FootSwitch[Switch-1][1],127,1);
      break;
    case 4:
      MIDINote(FootSwitch[Switch-1][1],0,2);
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
  if (BacklightTimeOut==101)
    lcd.noBacklight();
  else
    if (BacklightTimeOut==100)
      lcd.backlight();
    else
    {
      lcd.backlight();
      BacklightTimeOutMillis=millis();
    }
}



// --- Send Control Change MIDI Message ------------------------------------------------------------------------------------------
void MIDICC(byte CCNumber, byte Value)
{
  Serial.write(0xB0 + (MIDIOutChannel - 1));
  Serial.write(CCNumber);
  Serial.write(Value);
}



// --- Send Program Change MIDI Message ------------------------------------------------------------------------------------------
void MIDIProgramChange(byte tmpPatch)
{
  Serial.write(0xC0 + (MIDIOutChannel - 1));
  Serial.write(tmpPatch);
}



// --- Send a Note On/Off MIDI Message -------------------------------------------------------------------------------------------
void MIDINote(byte Note, byte Velocity, byte Mode)
{
  if (Mode==1)
  {
    Serial.write(0x9 + (MIDIOutChannel - 1));
    Serial.write(Note);
    Serial.write(Velocity);
  }
  else
  {
    Serial.write(0x8 + (MIDIOutChannel - 1));
    Serial.write(Note);
    Serial.write(Velocity);
  }
}



// --- Check for incoming MIDI Messages ------------------------------------------------------------------------------------------
void CheckMIDI()
{
  byte StatusByte;
  byte DataByte1;
  byte DataByte2;
  TurnLEDOn();
  if (Serial.available() == 2)
  {
    StatusByte = Serial.read();
    DataByte1 = Serial.read();
//    ProcessInput(StatusByte,DataByte1,DataByte2,2);
    ProcessInput(StatusByte,DataByte1,DataByte2);
  }
  else
  {
    StatusByte = Serial.read();  // read first byte
    DataByte1 = Serial.read();   // read next byte
    DataByte2 = Serial.read();   // read final byte
//    ProcessInput(StatusByte,DataByte1,DataByte2,3);
    ProcessInput(StatusByte,DataByte1,DataByte2);
  }
  TurnLEDOff();
}



// --- Process Incoming MIDI Messages --------------------------------------------------------------------------------------------
//void ProcessInput(byte StatusByte, byte DataByte1, byte DataByte2, byte Bytes)
void ProcessInput(byte StatusByte, byte DataByte1, byte DataByte2)
{
  byte BottomNibble = StatusByte & 0xF;
  byte TopNibble = StatusByte >> 4;
  if ((TopNibble == B1100) && (BottomNibble == (MIDIInChannel - 1)))                          // Program Change is B1100
  {
    lcd.setCursor(0,1);
    lcd.print("Program        (   )");
    DisplayNumber(1,16,Patch,3);  
    
    Patch = DataByte1;
    DisplayNumber(1,16,Patch,3);
  }
  else
    if ((TopNibble == B1011) && (BottomNibble == (MIDIInChannel -1)))                         // Control Change (CC) is B1011
    {                                                                                         // 0x7F = 127 = Tap Tempo on My Digitech
      if (DataByte1 == 0x7F)
        digitalWrite(IO9,HIGH);
      else
        digitalWrite(IO9,LOW);
    }
    else    
      UnknownRX();
/*  
  switch (Bytes)
  {
    case 2:
      if ((StatusByte >= 0xC0) && (StatusByte <= 0xCF))
      {
        if ((StatusByte - 0xC0) == (MIDIInChannel - 1))
        {
          Patch = DataByte1;
          // do some display here -- currently for debuging purposes
          DisplayNumber(1,16,Patch,3);
        }
        else
        {
          UnknownRX();
        }
      }
      break;
    case 3:
      UnknownRX();
      break;
  }
  */
}



// --- Show System Version and free memory ---------------------------------------------------------------------------------------
void ShowVersion()
{
  lcd.clear();
  lcd.print("  :: Magla MIDI ::  ");
  lcd.setCursor(0,1);
  lcd.print(Version);
  lcd.setCursor(3,2);
  lcd.print(freeMemory());
  lcd.print(" bytes free");
  lcd.setCursor(0,3);
  lcd.print(" maglaras@gmail.com ");
  while (Keypress()!=4)
    delay(1);
  while (Keypress()==4)
    delay(1);
  ClearLine(2);
  ClearLine(3);
}



// --- Not Recognized (or Not interested in that) MIDI Message -------------------------------------------------------------------
void UnknownRX()
{
  lcd.setCursor(0,3);
  //         01234567890123456789
  lcd.print("RX!                 ");
//  if (MIDIInChannel != 17)
//    DisplayNumber(1,16,Patch,3);      
//  else
//  {
//    ClearLine(1);
//  }
  delay(75);
  ClearLine(3);
}



// --- Clear Specific Line -------------------------------------------------------------------------------------------------------
void ClearLine(byte line)
{
  lcd.setCursor(0,line);
  lcd.print("                    ");
}


void TurnLEDOn()
{
  digitalWrite(IO9, HIGH);
}

void TurnLEDOff()
{
  digitalWrite(IO9, LOW);
}
