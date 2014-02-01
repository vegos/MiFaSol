//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                          MIDI Controller
//                           Version 3.02b
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
// 1 = MIDI Channel
// 3 - Backlight Timeout
// 5 - MIDI Thru M
// 31-40 = Type of message for Footswitch-31 (1..4)
// 41-50 = Value (0..127)


#define Version " Version 3.02b  "

#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>                        // LiquidCrystal_I2C V2.0 - Mario H. / atmega@xs4all.nl
                                                      // http://www.xs4all.nl/~hmario/arduino/LiquidCrystal_I2C/LiquidCrystal_I2C.zip 

LiquidCrystal_I2C lcd(0x27,16,2);                     // 0x27 is the address of the I2C driver for the LCD Screen

byte NoteChar[8] = { B00100, B00110, B00101, B00101, B01100, B11100, B11000, B00000 };    // Custom character for displaying a music note

#define  IO1        5
#define  IO2        6
#define  IO3        7
#define  IO4        8
#define  IO5       13
#define  IO6       12
#define  IO7       11
#define  IO8       10
#define  IO9        9
#define  EXP       A0

#define  TUNERCC  127

volatile byte PreviousExpPedal = 0;                   // Expression Pedal previous reading
volatile int MinExp = 0;                              // Expression Pedal Minimum reading
volatile int MaxExp = 1024;                           // Expression Pedal Maximum reading

volatile byte MIDIChannel = 1;                        // MIDI Channel for RX (1..16)

volatile byte BacklightTimeOut = 100;                 // Valid from 0..99 sec, 100 for Always On, 101 for Always Off.

volatile byte FootSwitch[10][2];                      // Array for storing settings for foot switches

volatile boolean MIDIThru = true;                     // MIDI Thru mode: enabled/disabled

volatile byte Patch = 0;

long ExpTimeOutMillis = 0;                            //
long FootswitchtTimeOutMillis = 0;                    // Storing millis for calculations
long BacklightTimeOutMillis = 0;                      //

#define LCDTimeOut  5000                              // Time to stay on LCD the pressed switch/exp. pedal value

const char* MenuItems[6] = { "MIDI Channel    ", 
                             "MIDI Thru Mode  ",
                             "Set Footswitches", 
                             "Calibrate Expr. ",
                             "Backlight Time  ",
                             "Factory Reset   "
                           };



// --- Setup procedure --------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(31250);
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
  if (BacklightTimeOut==101)
    lcd.noBacklight();
  else
    lcd.backlight();
  ClearScreen();
}



// --- Main procedure ---------------------------------------------------------------------------------------------------------
void loop()
{
  byte Key = Keypress();
  if (Key != 0)
  {
    SendCommand(Key);                               // SEND Command based on keypress    
    lcd.setCursor(0,1);
    lcd.print("FS ");
    lcd.print(Key);
    FootswitchtTimeOutMillis=millis();
    while (Keypress()==Key)
      delay(1);
    lcd.setCursor(0,0);
    lcd.print("Standby");
  }    
  if (PreviousExpPedal != ExpPedal())               // Exp. pedal is pressed, send the message
  {
    PreviousExpPedal = ExpPedal();
    lcd.setCursor(9,1);
    lcd.print("EXP");
    DisplayNumber(1,13,PreviousExpPedal,3);
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
    ExpTimeOutMillis=millis();
  }    
  
  if (MIDIThru)
    if (Serial.available())
      Serial.write(Serial.read());
    
    
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
  if (BacklightTimeOut<100)
  {
    if (millis()-BacklightTimeOutMillis>(BacklightTimeOut*1000))
      lcd.noBacklight();
  }
}









void CheckMenuTuner()
{
  long tmpmillis=millis();
  while (true)
  {
    if ((millis()-tmpmillis>3000) && (digitalRead(IO1)==LOW))
    {
      EnterTuner();
      while (digitalRead(IO1)==LOW)
        delay(1);
      return;      
    }
    if (digitalRead(IO1)==HIGH)
    {
      MainMenu();
      return;
    }
  }
}


void EnterTuner()
{
  MIDICC(TUNERCC,127);
}



// --- Detect and process footswitches/keypresses ----------------------------------------------------------------------------
byte Keypress()
{
  
  if (digitalRead(IO1)==LOW)
  {
    BacklightCheck();         
    CheckMenuTuner();
    return 0;
  }
  
  if ((digitalRead(IO2)==LOW) && (digitalRead(IO3)==LOW))
  {
    Patch-=1;
    if (Patch<1)
      Patch=128;
    BacklightCheck();      
    MIDIProgramChange(Patch);
    DisplayCommandType(2);
    DisplayNumber(0,4,Patch,3);      
    while (digitalRead(IO3)==LOW)
      delay(1);
    return 0;
  } 
  else
  if ((digitalRead(IO2)==LOW) && (digitalRead(IO4)==LOW))
  {
    Patch+=1;
    if (Patch>128)
      Patch=1;
    BacklightCheck();  
    MIDIProgramChange(Patch);    
    DisplayCommandType(2);
    DisplayNumber(0,4,Patch,3);  
    while (digitalRead(IO4)==LOW)
      delay(1);
    
    return 0;
  }  
  else
/*  
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
*/  
    if ((digitalRead(IO2)==LOW) && (digitalRead(IO3)==HIGH) && (digitalRead(IO4)==HIGH))
    {
      BacklightCheck();
      return 2;
    }
    else
      if ((digitalRead(IO3)==LOW) && (digitalRead(IO2)==HIGH))
      {
        BacklightCheck();
        return 3;
      }
      else
        if ((digitalRead(IO4)==LOW) && (digitalRead(IO2)==HIGH))
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
  
  
  
// --- Get the Expr. pedal position --------------------------------------------------------------------------------------------
byte ExpPedal()
{
  byte tmp = map(analogRead(EXP),MinExp,MaxExp,0,127);  
  if (tmp>127)
    tmp=0;
  return tmp;
}



// --- Display Command --------------------------------------------------------------------------------------------------------
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
void DisplayNumber(byte line, byte pos, byte number, byte decimals)
{
  lcd.setCursor(pos,line);
  if ((decimals==4) && (number<1000))
    lcd.print("0");
  if ((number<100) && (decimals>=3))
    lcd.print("0");
  if ((number<10) && (decimals>=2))
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
          case 1: // Set midi channel
            SetupMIDIChannel();
            break;
          case 2:
            SetupMIDIThru();
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



// --- Setup MIDI Thru ------------------------------------------------------------------------------------------------------------
void SetupMIDIThru()
{
  lcd.setCursor(0,0);
  lcd.print("Set MIDI Thru   ");
  
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("Thru  [        ]");
  lcd.setCursor(13,1);
  boolean tmpMIDIThru=MIDIThru;
  boolean StayInside=true;
  while (StayInside)
  {
    ShowMIDIThru(tmpMIDIThru);    
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
    switch (tmp)
    {
      case 1: // left
        tmpMIDIThru=!(tmpMIDIThru);
        break;
      case 2: // right
        tmpMIDIThru=!(tmpMIDIThru);
        break;
      case 3: // enter
        MIDIThru=tmpMIDIThru;
        if (MIDIThru)
        {
          EEPROM.write(5,1);  // write to eeprom
        }
        else
        {
          EEPROM.write(5,0);
        }
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }
}



// --- Show MIDI Thru Status -----------------------------------------------------------------------------------------------------
void ShowMIDIThru(boolean tmpMode)
{
  lcd.setCursor(7,1);
  if (tmpMode)
    lcd.print("Enabled ");
  else
    lcd.print("Disabled");  
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
        EEPROM.write(3,BacklightTimeOut);  // write to eeprom
        if (BacklightTimeOut==100)
          lcd.backlight();
        else
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
void SetupMIDIChannel()
{
  lcd.setCursor(0,0);
  lcd.print("Select Channel  ");

  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("MIDIChannel [  ]");
  byte tmpChannel = MIDIChannel;
  boolean StayInside=true;
  while (StayInside)
  {
    DisplayNumber(1,13,tmpChannel,2);
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
    switch (tmp)
    {
      case 1: // left
        tmpChannel-=1;
        if (tmpChannel<1)
          tmpChannel=16;
        break;
      case 2: // right
        tmpChannel+=1;
        if (tmpChannel>16)
          tmpChannel=1;
        break;
      case 3: // enter
        MIDIChannel=tmpChannel;
        EEPROM.write(1,MIDIChannel);  // write to eeprom
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
        EEPROM.write(3,30);
        EEPROM.write(5,1);
        for (byte tSwitch=1; tSwitch<=9; tSwitch++)
        {    
          EEPROM.write(30+tSwitch,2);
          EEPROM.write(40+tSwitch,tSwitch-1);
        }
        EEPROM.write(40,1);
        EEPROM.write(50,7);
        lcd.setCursor(0,0);
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
    if (Item<10)
    {
      lcd.print("Footswitch ");
      lcd.print(Item);
      lcd.print("    ");
    }
    else         
      lcd.print("Expr. Pedal     ");
    byte tmp=Keypress();
    if (tmp!=0)
      delay(250);
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
  lcd.print("    ");
  
  lcd.setCursor(0,1);
  if ((Switch>=1) && (Switch<=9))
  {
    lcd.print("0");
    lcd.print(Switch);
    lcd.print(": ");
    DisplayMode(Mode);
    lcd.print("  [   ]");
    // 0123456789012345
    // 01: CC     [001]
  }
  else
  {
    lcd.print("EXP: ");
    DisplayMode(Mode);
    lcd.print(" [   ]");
  }
  boolean StayInside=true;
  byte tmpSetting=FootSwitch[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNumber(1,12,tmpSetting-1,3);
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
        FootSwitch[Switch-1][0]=1;
        EEPROM.write(40+Switch,(tmpSetting-1));
        EEPROM.write(30+Switch,1);
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
    lcd.setCursor(10,1);
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
        StayInside=false;
        break;
    }
  }    
}



// --- Calibrate Exp. Pedal -------------------------------------------------------------------------------------------------------
void CalibratePedal()
{
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("Heel Down (Low) ");
  lcd.setCursor(0,1);
  lcd.print("Value     [    ]");
  boolean StayInside=true;
  int tmpMinMax=1024;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayNumber(1,11,tmpMinMax,4);
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
  //         0123456789012345
  lcd.print("Toe Down (High) ");
  lcd.setCursor(0,1);
  lcd.print("Value     [    ]");
  StayInside=true;
  tmpMinMax=0;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayNumber(1,11,tmpMinMax,4);
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




// --- Clear / Setup LCD Display --------------------------------------------------------------------------------------------------
void ClearScreen()
{
  lcd.clear();
  lcd.print("Standby         ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}  



// --- Write Integers to EEPROM procedure -----------------------------------------------------------------------------------------
// Write numbers (0-65535) to EEPROM (using 2 bytes).
void WriteToMem(byte address, int number)
{
  EEPROM.write(address,number/256);
  EEPROM.write(address+1,number % 256);
}



// --- Read Integers from EEPROM procedure ------------------------------------------------------------------------------------------
// Read numbers (0-65535) from EEPROM (using 2 bytes).
int ReadFromMem(byte Address)
{
  return EEPROM.read(Address)*256+EEPROM.read(Address+1);
}



// --- Initialize Settings / Read from EEPROM ---------------------------------------------------------------------------------------
void Initialize()
{
  MIDIChannel=EEPROM.read(1);
  BacklightTimeOut=EEPROM.read(3);  
  if (EEPROM.read(5)==1)
  {
    MIDIThru = true;
  }
  else
  {
    MIDIThru = false;
  }
  MinExp=ReadFromMem(11);
  MaxExp=ReadFromMem(13);
  for (byte tSwitch=1; tSwitch<=10; tSwitch++)
  {    
    FootSwitch[tSwitch-1][0]=EEPROM.read(30+tSwitch);
    FootSwitch[tSwitch-1][1]=EEPROM.read(40+tSwitch);    
  }
}




// --- Send a command, based on pressed footswitch ----------------------------------------------------------------------------
void SendCommand(byte Switch)
{
  DisplayCommandType(FootSwitch[Switch-1][0]);
  DisplayNumber(0,4,(FootSwitch[Switch-1][1]),3);  
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
    if (BacklightTimeOut=100)
      lcd.backlight();
    else
    {
      lcd.backlight();
      BacklightTimeOutMillis=millis();
    }
}





// Send CC
void MIDICC(byte CCNumber, byte Value)
{
  // 0xB0 = Control Change / Channel 0.
  // 0xAF + X = Control Change @ Channel X
  Serial.write(0xB0 + (MIDIChannel - 1));
  // or
  // Serial.write(0xB0 | (MidiChannel & 0x0F));
  Serial.write(CCNumber);
  Serial.write(Value);
}


// Send Program Change
void MIDIProgramChange(byte Patch)
{
  Serial.write(0xC0 + (MIDIChannel - 1));
  Serial.write(Patch);
}


// Send a Note
void MIDINote(byte Note, byte Velocity, byte Mode)
{
  if (Mode==1)
  {
    Serial.write(0x9 + (MIDIChannel - 1));
    Serial.write(Note);
    Serial.write(Velocity);
  }
  else
  {
    Serial.write(0x8 + (MIDIChannel - 1));
    Serial.write(Note);
    Serial.write(Velocity);
  }
}
