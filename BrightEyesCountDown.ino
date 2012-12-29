
/* *** Simple count down clock for Bright Eyes. ***
 - the clock starts at 0:57 (it takes about 3 seconds to start)
 - click the button to increase to add minutes
 - if the countdown reaches zero, the program functions as the sdplayer (it starts movie000.dat)
*/

// timing  NB: These need to be above the #includes below because they need the timing variables.

const int frameRate = 30; 
const int framePeriod = 1e6/frameRate; 
unsigned long lastMicros = 0;


// touch sense button
#include "touch.h"

// DMX
#include "dmx.h"

// SD card 
#include <SD.h>
const int numLEDs = 174;
const int DMXframeSize = 180; // six extra bytes to keep Mike's drivers happy
const int LEDpin = 9;
const int SDPOWER = 6;    //needs to be LOW to enable SD power -> 3.3V

unsigned char DMXframe[DMXframeSize]; 

// input movies
// manually added function prototype to overcome Arduino's autoprototyping
// this allows us to pass the index by reference and so update it
bool openMovie(int &index);
const int minMovieIndex = 0;
const int maxMovieIndex = 999;
int movieIndex = minMovieIndex;
File inputFile;
int fileSize;
const int headerSize = 16;
char fileName[13] = "movie000.dat"; // 8 in the filename, 1 period, 3 in the extension and 1 null terminator = 13



int mode = 0;
int minutes = 0;
int seconds = 57;
int pos1[] = {
1,	2,	3,	4,
12,	13,	14,	15,
25,	26,	27,	28,
40,	41,	42,	43,
55,	56,	57,	58,
69,	70,	71,	72,
79,	80,	81,	82,
};
int pos2[] = {
6,	7,	8,	9,
17,	18,	19,	20,
30,	31,	32,	33,
45,	46,	47,	48,
60,	61,	62,	63,
74,	75,	76,	77,
84,	85,	86,	87
};
int pos3[] = {
88,	89,	90,	91,
98,	99,	100,	101,
112,	113,	114,	115,
127,	128,	129,	130,
142,	143,	144,	145,
155,	156,	157,	158,
166,	167,	168,	169
};
int pos4[] = {
93,	94,	95,	96,
103,	104,	105,	106,
117,	118,	119,	120,
132,	133,	134,	135,
147,	148,	149,	150,
160,	161,	162,	163,
171,	172,	173,	174,
};

long numbers[] = {110729622L, 35791394L, 252799111L, 126380167L, 143194521L, 126382367L, 110719254L, 143165583L, 110717334L, 126413206L};

void setup(){

  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, HIGH); // The LED is active LOW, so set it HIGH to turn it off.

  // sets us up for DMX output
  setupDMX(DMXframe);

  // enable the SD CARD POWER -> active LOW
  pinMode(SDPOWER,OUTPUT);
  digitalWrite(SDPOWER,LOW);
  
   // init SD card
  bool initOK = initSD();
 
   // if we have an SD init or movie error, sit in an infinite loop and don't 
   // continue program execution - flash the LED to indicate this error
   if(!initOK || !openMovie(movieIndex)){
     while(1){
       digitalWrite(LEDpin, LOW);
       delay(1000);
       digitalWrite(LEDpin, HIGH);
       delay(1000);
     };  
   }

  delay(500);
  digitalWrite(LEDpin, LOW); // turn LED ON
  delay(500);
  digitalWrite(LEDpin, HIGH); // turn LED OFF
}



void loop(){

  if (mode == 0) {
    if(micros()>=lastMicros+1e6){
      lastMicros = micros();
      tick();
    }
     if(readTouch()){
       minutes++;
       if (minutes > 59) minutes = 0;
       showTime();
       delay(100);
    }
  } else {
    if(micros()>=lastMicros+framePeriod){
      // if we're ready for a new frame, reset our timer and get the next frame
      lastMicros = micros();
      frameAdvance();
    }
  }
}




void tick(){
 
  seconds--;
  if (seconds == 0 && minutes == 0) mode++;
  if (seconds < 0) {
    seconds = 59;
    minutes--;
  }
  
  showTime();
}

void showTime(){
  int a = minutes / 10;
  int b = minutes % 10;
  int c = seconds / 10;
  int d = seconds % 10;
  for (int i=0;i<28;i++) {
     DMXframe[pos1[i]]=bitRead(numbers[a],i) * 255;
     DMXframe[pos2[i]]=bitRead(numbers[b],i) * 255;
     DMXframe[pos3[i]]=bitRead(numbers[c],i) * 255;
     DMXframe[pos4[i]]=bitRead(numbers[d],i) * 255;
  }
  
   sendDMX(DMXframe, DMXframeSize);
}


void frameAdvance(){
  
  // advances the frame and sends it out to the LEDs
  
  if(readTouch()){
    digitalWrite(LEDpin, LOW); // pulse LED if we have a touch input
    if(++movieIndex>maxMovieIndex){
      movieIndex = minMovieIndex; 
    } 
    // open the next movie
    openMovie(movieIndex);
  } else {
    digitalWrite(LEDpin, HIGH); // switch LED off if we do not have a touch input
  }
  
  // whatever happens, get a new frame and send it out
  getNextFrame();
  sendDMX(DMXframe, DMXframeSize);  
}

void getNextFrame(){
  
  // loads the next frame from the SD card
  
  // if we don't have even one frame, we have nothing to do 
  if(inputFile.size()>=headerSize + numLEDs){  
    // only do this if we have enough bytes left to load for a full frame...
    if(inputFile.available()>=numLEDs){

      inputFile.read(DMXframe+1, numLEDs); // this should read everything in one quick block read
      
    } else {
      // ...else go back to the beginning of the file and start again
      inputFile.seek(headerSize); 
      getNextFrame ();   
    }
  }
}

boolean initSD(){
  
  // initialises the SD card and loads our input file
  
  String fileName;
  
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);// enable SD card power - THIS IS IMPORTANT!
  
  delay(100); // wait for SD card to stabilise...
  
  // pin10 is the CS pin for the BrightEyes board, if it is not
  // set as an output, the SD card code will not function
  pinMode(10, OUTPUT);

  if (!SD.begin(10)) {
    return false;
  }
}

bool openMovie(int &index){
  
  // opens a movie with a given index - for example, index 13 opens movie013.dat
  
  // if we're sent a silly index, return false
  if(index>maxMovieIndex || index<minMovieIndex){
    return false;  
  }
  
  // alter the filename to reflect the index
  fileName[5] = '0' + (index/100);
  fileName[6] = '0' + ((index/10)%10);
  fileName[7] = '0' + (index%10); 
  
  // check we have the file...
  if(SD.exists(fileName)){
    // ... if we do, close the old file and open the new one
    inputFile.close();
    inputFile = SD.open(fileName);
    // miss out the header
    inputFile.seek(headerSize);
    Serial.println(fileName);
    return true;
  } else {
    if(index==minMovieIndex){
      return false;  // if we can't find the first file, there is no hope
    } else {
      index = minMovieIndex; // otherwise, try again from the beginning (and update the index)
      return openMovie(index); 
    }
  }
}


