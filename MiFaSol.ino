//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                          MIDI Controller
//                           Version 2.30b
//
//
//
//------- Some Documentation ---------------------------------------------------------------
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
// 31-40 = Type of message for Footswitch-31 (1..4)
// 41-50 = Value (0..127)


#include <MIDI.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

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
volatile int KeyDelay = 75;                  // 50ms for keypress
volatile int Patch = 0;
volatile int Volume = 0;
volatile int MinExp = 0;
volatile int MaxExp = 1024;

byte MIDIChannel = 1;

volatile byte FS[10][2];                              // Array for storing settings for foot switches


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
  Initialize();
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

  MIDI.begin(MIDIChannel);
//  Serial.begin(31250);
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
  lcd.print("                ");

}

void loop()
{
  byte Key = Keypress();
  if (Key != 0)
  {
    long tmpmillis=millis();    
    SendCommand(Key);    // SEND Command based on keypress
    LCDNumber(0,4,Key);
    LCDText(1,"Done!");
    while ((millis()-tmpmillis<KeyDelay) && (Keypress()==Key));  // DELAY for key depress or time
    LCDText(1,"     ");
  }
    
  if (PreviousExpPedal != ExpPedal())
  {
    PreviousExpPedal = ExpPedal();
    LCDNumber(0,13,PreviousExpPedal);
    SendMidiCC(MIDIChannel,0,(byte)PreviousExpPedal);
  }
}


byte Keypress()
{
  if ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==LOW))
  {
    while ((digitalRead(IO1)==LOW) && (digitalRead(IO2)==LOW));
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
  byte tmp = map(analogRead(EXP),MinExp,MaxExp,0,127);  
  if (tmp>127)
    tmp=0;
  return tmp;
}



// Change Program (Patch).
void ChangeProgram(byte MidiChannel, byte Patch)
{
  MIDI.sendProgramChange(Patch,MidiChannel);
  // 0xC0 = Program Change / Channel 0
  // 0xBF + X = Program Change  @ Channel X
//  Serial.write(0xBF+MidiChannel);
//  Serial.write(Patch);

}



// Send a MIDI Control Change Message
void SendMidiCC(byte MidiChannel, byte CCNumber, byte Value)
{
  MIDI.sendControlChange(CCNumber,Value,MidiChannel);
  // 0xB0 = Control Change / Channel 0.
  // 0xAF + X = Control Change @ Channel X
//  Serial.write(0xAF+MidiChannel);
  // or
  // Serial.write(0xB0 | (MidiChannel & 0x0F));
//  Serial.write(CCNumber);
//  Serial.write(Value);
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
  while (Keypress()!=0);
  while (StayInside)
  {
    lcd.setCursor(0,1);
    lcd.print(MenuItems[Menu]);
    byte tmp=Keypress();
    while (Keypress()!=0);    
    switch (tmp)
    {
      case 4:  // back-exit
        StayInside=false;
        break;     
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
            CalibratePedal();
            break;
          case 4: // factory reset
            FactoryReset();
            break;
        }
        StayInside=false;
        break;
    }
  }
  ClearScreen();
}

void SetupMIDIChannel()
{
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("MIDI Channel    ");
  lcd.setCursor(13,1);
  byte tmpChannel=MIDIChannel;
  boolean StayInside=true;
  while (StayInside)
  {
    ShowMIDIChannel(tmpChannel);    
    byte tmp=Keypress();
    while (Keypress()!=0);
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


void ShowMIDIChannel(byte Channel)
{
  lcd.setCursor(13,1);
  if (Channel<10)
    lcd.print("0");
  lcd.print(Channel);
}

void FactoryReset()
{
  lcd.setCursor(0,1);
  //         0123456789012345
  lcd.print("** RESETTING! **");
  WriteToMem(11,0);
  WriteToMem(13,1024);
  WriteToMem(1,1);
  for (byte tSwitch=1; tSwitch<=9; tSwitch++)
  {    
    EEPROM.write(30+tSwitch,2);
    EEPROM.write(40+tSwitch,tSwitch);
  }
  EEPROM.write(40,1);
  EEPROM.write(50,7);
  delay(3000);
  SoftReset();
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
    while (Keypress()!=0);
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

void SetupFootSwitchCC(byte Switch)
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
  byte tmpCC=FS[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayCC(tmpCC-1);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
        FS[Switch-1][1]=tmpCC-1;
        FS[Switch-1][0]=1;
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

void DisplayCC(byte num)
{
  lcd.setCursor(12,1);
  if (num<11)
    lcd.print("0");
  lcd.print(num);
}


void SetupFootSwitchPatch(byte Switch)
{
  lcd.setCursor(0,1);
  lcd.print("(");
  lcd.print(Switch);
  lcd.print(") Patch [   ]");
  // 0123456789012345
  // (1) Patch [001] 
  boolean StayInside=true;
  byte tmpPatch=FS[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayPatchMemory(tmpPatch-1);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
        FS[Switch-1][1]=tmpPatch-1;
        FS[Switch-1][0]=2;
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}

void DisplayPatchMemory(byte num)
{
  lcd.setCursor(11,1);
  if (num<101)
    lcd.print("0");
  if (num<11)
    lcd.print("0");
  lcd.print(num);
}


void SetupFootSwitchNoteOn(byte Switch)
{
  lcd.setCursor(0,1);
  lcd.print("(");
  lcd.print(Switch);
  lcd.print(") NoteOn [  ]");
  // 0123456789012345
  // (1) NoteOn [001] 
  boolean StayInside=true;
  byte tmpNote=FS[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNoteMemory(tmpNote-1);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
        EEPROM.write(Switch+40,3);
        FS[Switch-1][1]=tmpNote-1;
        FS[Switch-1][0]=3;
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}

void SetupFootSwitchNoteOff(byte Switch)
{
  lcd.setCursor(0,1);
  lcd.print("(");
  lcd.print(Switch);
  lcd.print(") NoteOf [  ]");
  // 0123456789012345
  // (1) Patch [001] 
  boolean StayInside=true;
  byte tmpNote=FS[Switch-1][1]+1;
  while (StayInside)
  {
    DisplayNoteMemory(tmpNote-1);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
        EEPROM.write(Switch+30,4);
        FS[Switch-1][1]=tmpNote-1;
        FS[Switch-1][0]=4;
        StayInside=false;        
        break;
      case 4: // back
        StayInside=false;
        break;
    }
  }    
}

void DisplayNoteMemory(byte num)
{
  lcd.setCursor(12,1);
  if (num<100)
    lcd.print("0");
  if (num<10)
    lcd.print("0");
  lcd.print(num);
}


void ChooseFSOption(byte Switch)
{
  lcd.setCursor(0,1);
  lcd.print("(");
  lcd.print(Switch);
  lcd.print(") Msg:        ");
  // 0123456789012345
  // (1) Msg [      ]
  boolean StayInside=true;
  byte tmpMode=FS[Switch-1][0];
  while (StayInside)
  {
    DisplaySwitchMode(tmpMode);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
        FS[Switch-1][0]=tmpMode;
        switch (tmpMode)
        {
          case 2:
            SetupFootSwitchPatch(Switch);
            break;
          case 1:
            SetupFootSwitchCC(Switch);
            break;
          case 3:
            SetupFootSwitchNoteOn(Switch);
            break;
          case 4:
            SetupFootSwitchNoteOff(Switch);
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


void DisplaySwitchMode(byte num)
{
  lcd.setCursor(9,1);
  switch (num)
  {
    case 1:
      lcd.print("CC    ");
      break;
    case 2:
      lcd.print("Patch ");
      break;
    case 3:
      lcd.print("NoteOn");
      break;
    case 4:
      lcd.print("NoteOf");
      break;
  }
}

void CalibratePedal()
{
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("Fully press exp.");
  lcd.setCursor(0,1);
  lcd.print("Value [    ]    ");
  boolean StayInside=true;
  int tmpMinMax=1024;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayValuePedal(tmpMinMax);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("Fully de-press  ");
  lcd.setCursor(0,1);
  lcd.print("Value [    ]    ");
  StayInside=true;
  tmpMinMax=0;
  while (StayInside)
  {
    tmpMinMax=analogRead(EXP);
    DisplayValuePedal(tmpMinMax);
    byte tmp=Keypress();
    while (Keypress()!=0);    
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

void DisplayValuePedal(int num)
{
  lcd.setCursor(7,1);
  if (num<1000)
    lcd.print("0");
  if (num<100)
    lcd.print("0");
  if (num<10)
    lcd.print("0");
  lcd.print(num);
}


void ClearScreen()
{
  lcd.clear();
  lcd.print("PGM:---  EXP:---");
  lcd.setCursor(0,1);
  lcd.print("                ");
}  


// --- WRITE TO EEPROM procedure ---------------------------------------------------------------------------------------------------------------------
// Write numbers (0-65535) to EEPROM (using 2 bytes).

void WriteToMem(byte address, int number)
{
  int a = number/256;
  int b = number % 256;
  EEPROM.write(address,a);
  EEPROM.write(address+1,b);
}




// --- READ FRON EEPROM procedure --------------------------------------------------------------------------------------------------------------------
// Read numbers (0-65535) from EEPROM (using 2 bytes).

int ReadFromMem(byte address)
{
  int a=EEPROM.read(address);
  int b=EEPROM.read(address+1);

  return a*256+b;
}


void Initialize()
{
  MIDIChannel=EEPROM.read(1);
  if ((MIDIChannel<1) || (MIDIChannel>16))
    MIDIChannel=1;
  MinExp=ReadFromMem(11);
  MaxExp=ReadFromMem(13);
  for (byte tSwitch=1; tSwitch<=10; tSwitch++)
  {    
    FS[tSwitch-1][0]=EEPROM.read(30+tSwitch);
    FS[tSwitch-1][1]=EEPROM.read(40+tSwitch);    
  }
}

void SendCommand(byte Switch)
{
//  lcd.clear();
//  lcd.setCursor(0,0);
//  lcd.print(FS[Switch-1][0]);
//  lcd.setCursor(0,1);
//  lcd.print(FS[Switch-1][1]);
//  delay(1000);
  if (FS[Switch-1][0]==1)
    MIDI.sendControlChange((FS[Switch-1][1]),127,MIDIChannel);
  else
    if (FS[Switch-1][0]==2)
      MIDI.sendProgramChange((FS[Switch-1][1]),MIDIChannel);
    else
      if (FS[0][0]==3)
        MIDI.sendNoteOn((FS[Switch-1][1]),127,MIDIChannel);
      else
        if (FS[0][0]==4)
          MIDI.sendNoteOn((FS[Switch-1][1]),0,MIDIChannel);
}

void SoftReset()
{
  asm volatile ("  jmp 0");
}
