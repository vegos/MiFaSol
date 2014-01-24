//                  __  ____ ______      _____       __
//                 /  |/  (_) ____/___ _/ ___/____  / /
//                / /|_/ / / /_  / __ `/\__ \/ __ \/ / 
//               / /  / / / __/ / /_/ /___/ / /_/ / /  
//              /_/  /_/_/_/    \__,_//____/\____/_/ 
//
//         ©2014, Antonis Maglaras :: maglaras@gmail.com
//                         MIDI IR Controller
//                           Version 0.10a
//
//
//

#include <IRremote.h>

#define  LED    11
#define  IR      9

volatile byte CC = 1;
volatile byte CCSelect = 1;
volatile byte PreviousExpPedal = 0;
volatile int KeyDelay = 50;                  // 50ms for keypress
volatile int Patch = 0;
volatile int Volume = 0;
volatile byte MChannel = 1;

IRrecv irrecv(IR);
decode_results results;

void setup()
{
  irrecv.enableIRIn(); // Start the receiver
  pinMode(LED,OUTPUT);
  Serial.begin(31250);
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
}





// Change Program (Patch).
void ChangeProgram(byte MidiChannel, byte Patch)
{
  digitalWrite(LED,HIGH);
  // 0xC0 = Program Change / Channel 0
  // 0xBF + X = Program Change  @ Channel X
  Serial.write(0xBF+MidiChannel);
  Serial.write(Patch);
  digitalWrite(LED,LOW);
}



// Send a MIDI Control Change Message
void SendMidiCC(byte MidiChannel, byte CCNumber, byte Value)
{
  digitalWrite(LED,HIGH);
  // 0xB0 = Control Change / Channel 0.
  // 0xAF + X = Control Change @ Channel X
  Serial.write(0xAF+MidiChannel);
  // or
  // Serial.write(0xB0 | (MidiChannel & 0x0F));
  Serial.write(CCNumber);
  Serial.write(Value);
  digitalWrite(LED,LOW);
}
