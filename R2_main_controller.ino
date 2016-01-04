#include <Wire.h>
#include <Servo.h>
#include <ArduinoNunchuk.h> // info http://www.gabrielbianconi.com/arduinonunchuk/
#include <Sabertooth.h> // to control Sabertooth 2x12 foot motor controller
#include <SyRenSimplified.h> // to control SyRen head motor controller
#include <SPI.h>
#include <Adafruit_VS1053.h> // for sound control
#include <SD.h> // for SD card support

Sabertooth ST = Sabertooth(128); // Sabertooth foot controller
Sabertooth SR = Sabertooth(129); // SyRen head controller
  
// ADAFRUIT MP3 PLAYER SHIELD
  // These are the pins used for the Adafruit Music Maker shield
  #define SHIELD_RESET  9      // VS1053 reset pin (unused!)
  #define SHIELD_CS     10      // VS1053 chip select pin (output)
  #define SHIELD_DCS    8      // VS1053 Data/command select pin (output)
  // These are common pins between breakout and shield
  #define CARDCS 4     // Card chip select pin
  // DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
  #define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

ArduinoNunchuk nunchuk = ArduinoNunchuk();

int led1 = 13; // power-indicator led... port 13 is same as onboard LED
int xjoystick;
int yjoystick;
int xtilt;
int ytilt;
char fileName[30];  // an array to hold the audio filename
int fileToPlay ;    // numeric variable for audio filename to play
int value1 = 2;  // 1st (minimum) joystick value... for determining which "quadrant" the joystick is in
int value2 = 85 ;  // 2nd joystick value
int value3 = 170 ;  // 3rd joystick value
int value4 = 255;  // 4th (max) joystick value
int volume = 10; // Set volume for left, right audio channels... lower number = louder volume
int velocity = 40; // Set foot motor speed... 127 max
int headVel = 60; // Set head motor speed... 127 max

void setup()
{
  nunchuk.init();
  pinMode(led1, OUTPUT);
  
  // SABERTOOTH & SYREN MOTOR DRIVER SETUP
  SabertoothTXPinSerial.begin(9600); // Initiate serial communication for both Sabertooth and SyRen
  ST.drive(0); // The Sabertooth won't act on mixed mode until
  ST.turn(0);  // it has received power levels for BOTH throttle and turning, since it
               // mixes the two together to get diff-drive power levels for both motors.
               // So, we set both to zero initially.
  SR.motor(0); // Set initial head motor speed to zero
  ST.setRamping(18); // 0-10 = fast ramp, 11-20 slow ramp, 21-80 intermediate ramp
  SR.setRamping(20);
  
  // ADAFRUIT MP3 PLAYER SHIELD SETUP
  Serial.begin(9600);
    Serial.println("Adafruit VS1053 Simple Test");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  SD.begin(CARDCS);    // initialise the SD card containing mp3 files
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(volume,volume);

  // Timer interrupts are not suggested, better to use DREQ interrupt!
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Play one file, don't return until complete
  //Serial.println(F("Playing track 2"));
  //musicPlayer.playFullFile("2.mp3");
  // Play another file in the background, REQUIRES interrupts!
  //Serial.println(F("Playing track 1"));
  //musicPlayer.startPlayingFile("1.mp3");

} // End setup


void loop(){ // Main program loop

  digitalWrite(led1, HIGH); // power indicator LED constant on

  nunchuk.update();
  
  /*Serial.print(nunchuk.analogX, DEC); // To display & debug Nunchuk data
  Serial.print(' ');
  Serial.print(nunchuk.analogY, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelX, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelY, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.accelZ, DEC);
  Serial.print(' ');
  Serial.print(nunchuk.zButton, DEC);
  Serial.print(' ');
  Serial.println(nunchuk.cButton, DEC);*/
  
// BODY MOVEMENT... JOYSTICK WITH Z BUTTON
  if(nunchuk.zButton == 1){ // If Z button is pressed...

   xjoystick = nunchuk.analogX;
   xjoystick = constrain(xjoystick, 2, 255); // calibrate X tilt value
   xjoystick = map(xjoystick, 0, 255, (velocity*-1), velocity);
   ST.turn(xjoystick);
   
   yjoystick = nunchuk.analogY;
   yjoystick = constrain(yjoystick, 2, 255); // calibrate Y tilt value
   yjoystick = map(yjoystick, 0, 255, (velocity*-1), velocity);
   ST.drive(yjoystick);
   } else { // when Z button is released...
    ST.turn(0); // Stop turn
    ST.drive(0); // Stop move
   }

// HEAD TURN LEFT OR RIGHT... TILT NUNCHUK LEFT OR RIGHT
  xtilt = nunchuk.accelX; // to turn head left or right
  xtilt = constrain(xtilt, 350, 650); // calibrate X tilt value
  xtilt = map(xtilt, 350, 650, (-1*headVel), headVel);
  if((xtilt >= -30) && (xtilt <= 30)){
    SR.motor(0);
  }else{
    SR.motor(xtilt);
  }
  // delay(20); // slow down the motion


// SOUNDS... PRESS C BUTTON
  if(nunchuk.cButton == 1){ // See if C button is pressed

    if((value1 <= nunchuk.analogX) && (nunchuk.analogX <= value2) && (value1 <= nunchuk.analogY) && (nunchuk.analogY <= value2)){ // joystick in quadrant A
      musicPlayer.stopPlaying();
      fileToPlay = random(1,16); //a random number between 1 and 16... number of files in quadrant A "Mischief"
      sprintf(fileName, "%d.mp3", fileToPlay);  //turn the fileToPlay number into a string and add extension... store it in fileName array
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value2 <= nunchuk.analogX) && (nunchuk.analogX <= value3) && (value1 <= nunchuk.analogY) && (nunchuk.analogY <= value2)){ // joystick in quadrant B
      musicPlayer.stopPlaying();
      fileToPlay = random(17,34); //a random number... number of files in quadrant B "Happy"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value3 <= nunchuk.analogX) && (nunchuk.analogX <= value4) && (value1 <= nunchuk.analogY) && (nunchuk.analogY <= value2)){ // joystick in quadrant C
      musicPlayer.stopPlaying();
      fileToPlay = random(35,41); //a random number... number of files in quadrant C "Question"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value1 <= nunchuk.analogX) && (nunchuk.analogX <= value2) && (value2 <= nunchuk.analogY) && (nunchuk.analogY <= value3)){ // joystick in quadrant D
      musicPlayer.stopPlaying();
      fileToPlay = random(42,53); //a random number... number of files in quadrant D "Ooh"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value2 <= nunchuk.analogX) && (nunchuk.analogX <= value3) && (value2 <= nunchuk.analogY) && (nunchuk.analogY <= value3)){ // joystick at rest... quandrant E
      musicPlayer.stopPlaying();
      fileToPlay = random(54,99); //a random number... number of files in quandrant E "Chat"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value3 <= nunchuk.analogX) && (nunchuk.analogX <= value4) && (value2 <= nunchuk.analogY) && (nunchuk.analogY <= value3)){ // joystick in quadrant F
      musicPlayer.stopPlaying();
      fileToPlay = random(100,131); //a random number... number of files in quadrant F "Yell"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value1 <= nunchuk.analogX) && (nunchuk.analogX <= value2) && (value3 <= nunchuk.analogY) && (nunchuk.analogY <= value4)){ // joystick in quadrant G
      musicPlayer.stopPlaying();
      fileToPlay = random(132,145); //a random number... number of files in quadrant G "Scared"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value2 <= nunchuk.analogX) && (nunchuk.analogX <= value3) && (value3 <= nunchuk.analogY) && (nunchuk.analogY <= value4)){ // joystick in quadrant H
      musicPlayer.stopPlaying();
      fileToPlay = random(146,172); //a random number... number of files in quadrant H "Annoyed"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }
    if((value3 <= nunchuk.analogX) && (nunchuk.analogX <= value4) && (value3 <= nunchuk.analogY) && (nunchuk.analogY <= value4)){ // joystick in quadrant I
      musicPlayer.stopPlaying();
      fileToPlay = random(173,176); //a random number... number of files in quadrant I "Scream"
      sprintf(fileName, "%d.mp3", fileToPlay);
      musicPlayer.startPlayingFile(fileName); // play file
    }

  //delay(100);
  }
  
} // Loop program
