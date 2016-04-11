
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);

char block = 219; // first level obstacles' symbol by ASCII code (rectangle empty inside, some Chinese symbol)
const char gamer = '*'; // gamer's symbol

int blockCounter = 0; // counts how cells to skip after obstacle has been created
int counter = 0; // how many cycles gamer survived
const int baseDelay = 100;
int _delay; // game's speed, the lesser the faster

char line0[25]; // print buffer for lcd, line 0
char line1[25]; // print buffer for lcd, line 1

int gamerPos = 0; // what the gamer is located (line 0 or 1)
int gamerLastPos = 0; // where the gamer was located before he moved (line 0 or 1)

// set pin numbers:
const int button1Pin = 15;   // the number of the pushbutton pin
const int button2Pin = 13;   // the number of the pushbutton pin
const int led1Pin =  2;      // the number of LED pin for button 1
const int led2Pin = 0;       // the unmber of LED pin for button 2

int button1State = 0;         // variable for reading the pushbutton status
int button2State = 0;         // variable for reading the pushbutton status 
int button1LastState = 0;     // variable for reading the pushbutton status
int button2LastState = 0;     // variable for reading the pushbutton status

String matrix1Str = String(16); // string for matrix effect intro
String matrix2Str = String(16); // string for matrix effect intro
char matrix1[17];               // char array for matrix effect intro
char matrix2[17];               // char array for matrix effect intro

// creates obstacles on lines
// draws checkpoints every 50 symbols
void GenBlocks(int current, char line0[], char line1[]) {

  // checkpoint
  if (counter > 0 && (counter % 50 == 0)) {
    line0[current] = '|';
    line1[current] = '|';  
  }
  
  if (blockCounter > 0){
    blockCounter--;
    return;
  }

  // Lets try to create block on line 0
  if (line0[current]!=block){
    int factor = random(1000);
    if (factor > 500) { // success
      line0[current]=block;
    }
  }

  // if there was a block created on line 0 we should then create also on line 1
  if (line0[current] == block) {
    if (random(1000) < 500) {
      line1[current + 2] = block;
      blockCounter = 3; 
    }
    
  }       
}

// New game init procedure
void NewGame() {

  counter = 0;
  _delay = baseDelay;
  block = 219;
   
  strcpy(line0,"                    ");
  strcpy(line1,"                    ");
  
  // generate some blocks for start
  for (int i = 7; i < 16; i++) {
     GenBlocks(i, line0, line1); 
  }
  
  gamerPos = 0;
  gamerLastPos = 0;
  line0[4]=gamer;
  
  lcd.clear();
  lcd.print("Hello gamer :)");
  lcd.setCursor(0,1);
  lcd.print("Get ready!");

  delay(1000);

  Serial.println("Print game field");
  Serial.println(line0);
  Serial.println(line1);
  
  lcd.setCursor(0,0);
  lcd.print(line0);
  lcd.setCursor(0,1);
  lcd.print(line1);

  Serial.println("Start game cycle");
}

void FillStringWithChars(char str[], int strLength) {
  for (int i = 0; i< strLength; i++) {
    str[i] = random(34,255);
  }
}

// procedure shows "Matrix" effect when symbols fall down in random manner
void ShowMatrix(){

  // init of char arrays with 16 space symbols (lcd is 16 symbols in a row)
  strcpy(matrix1,"                    ");
  strcpy(matrix2,"                    ");
  matrix1[17]='\0';
  matrix2[17]='\0';

  // fill first string with random symbols from ASCII
  FillStringWithChars(matrix1, 16);
  
  Serial.println("matrix1");
  Serial.println(matrix1);

  // main cycle
  for (int j = 0; j < 8; j++) {

    // fill second string with random symbols
    FillStringWithChars(matrix2, 16);
  
    Serial.println("matrix2");
    Serial.println(matrix2);
    
    matrix1Str = String(matrix1);
    matrix2Str = String(matrix2);

    // override second string with some symbols from first string
    for (int i = 0; i< 6; i++) {
      int element = random(16);
      matrix2Str[element]=matrix1Str[element];
    }

  //convert for printing
  matrix1Str.toCharArray(matrix1,17);
  matrix2Str.toCharArray(matrix2,17);
  Serial.println("matrix1");
  Serial.println(matrix1);
  Serial.println("matrix2");
  Serial.println(matrix2);

  // print
  lcd.setCursor(0,1);
  lcd.print(matrix1);
  lcd.setCursor(0,0);
  lcd.print(matrix2);
  delay(500);
  
  FillStringWithChars(matrix1, 16);
  
  Serial.println("matrix1");
  Serial.println(matrix1);
  
  for (int i = 0; i< 6; i++) {
    int element = random(16);
    matrix1Str[element]=matrix2Str[element];
  }

   //convert for printing
  matrix1Str.toCharArray(matrix1,17);
  matrix2Str.toCharArray(matrix2,17);
  
  lcd.setCursor(0,1);
  lcd.print(matrix2);
  lcd.setCursor(0,0);
  lcd.print(matrix1);
  delay(500);
  }

  lcd.setCursor(0,0);
  lcd.print("---+ Runner +---");
  delay(3000);
}

// one time start-up procedure
void setup(void){
  
  // init serial port and lcd 
  Serial.begin(115200);
  lcd.begin(16, 2);

  // initialize the LED pin as an output:
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);

  // read the state of the pushbutton value:
  button1LastState = digitalRead(button1Pin);
  button2LastState = digitalRead(button2Pin);

  // show intro
  ShowMatrix();

  // start new game
  NewGame();
   
}

void RefreshPic(char line0[], char line1[]){
  // we do not want to copy gamer's symbol from 4th column
  for (int i = 0; i < 18; i++) {
    if (i == 3) {
      if (gamerLastPos == 0) {
        line0[i] = '_';
        line1[i] = line1[i+1];
      }
      else {
        line0[i] = line0[i+1];
        line1[i] = '_';
      }
    }
    else
    { // transfer each symbol for 1 cell left creating movement
      line0[i] = line0[i+1];
      line1[i] = line1[i+1];
    }
  }
  
}

void ReadButtons(int *positn) {
 // read the state of the pushbutton value:
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);
    
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (button1State == HIGH) {
    // turn LED on:
    digitalWrite(led1Pin, LOW);
    if (button1State != button1LastState) {
      *positn = 0;
      button1LastState = button1State;
    }
    
  } else {
    button1LastState = 0;
    // turn LED off:
    digitalWrite(led1Pin, HIGH);
  }

  if (button2State == HIGH) {
    // turn LED on:
    digitalWrite(led2Pin, LOW);
    if (button2State != button2LastState) {
      *positn = 1;
      button2LastState = button2State;
    }
    
  } else {
    button2LastState = 0;
    // turn LED off:
    digitalWrite(led2Pin, HIGH);
  } 
}

void PrintScore(){
  lcd.setCursor(0,1);
  lcd.print("U did:");
  lcd.print(counter);
  lcd.print(" cycles");
}

// procedure for game over
void GameOver(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Game over TT");
  PrintScore();
  
  delay(5000);

  // restart the game
  NewGame();
}

void GameFinish() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("-+-+- You won!");
  lcd.setCursor(0,1);
  PrintScore();
  delay(4000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Next player...  ");
  lcd.setCursor(0,1);
  lcd.print("  >>> EXIT >>> ");
  delay(3000);
  
  ShowMatrix();
  NewGame();
}

// change obstacles' view and increase speed
void UpdateGameLevel(int *_counter){
  int level = *_counter / 50;
  
  Serial.print("current counter ");
  Serial.println(*_counter);
  Serial.print("current level ");
  Serial.println(level);
  
  if (level == 1) {
    block = '#';
    return;
  }

  if (level == 2) {
    block = ':';
    _delay = baseDelay - level*5;
    return;
  }

  if (level == 3) {
    block = '.';
    _delay = baseDelay - level*5;
    return;
  }

  if (level == 4) {
    block = '_';
    _delay = baseDelay - level*5;
    return;
  }

  if (level == 5) {
    block = 255;
    _delay = baseDelay - level*5;
    return; 
  }

  if (level == 6) {
    GameFinish();
  }
}

//main loop
void loop(void){
  
  ReadButtons(&gamerPos);
  Serial.print("Gamer sits on line ");
  Serial.println(gamerPos);
  
  RefreshPic(line0, line1);
  gamerLastPos = gamerPos;

  // check whether the gamer is in the same cell where the obstacle is
  if (gamerPos == 0) {
    if (line0[4] == block) {
      GameOver();
    }
  }
  else {
    if (line1[4] == block) {
      GameOver();
    }
  }

  // print gamer's symbol
  if (gamerPos == 0) {
    line0[4] = gamer;
    }
  else {
    line1[4] = gamer;
  }

  // show current game field to user
  lcd.setCursor(0,0);
  lcd.print(line0);
  lcd.setCursor(0,1);
  lcd.print(line1);
  Serial.println(line0);
  Serial.println(line1);

  // generate new obstacles
  GenBlocks(15, line0, line1);

  counter++;

  UpdateGameLevel(&counter);

  // wait for user's input 5ms*_delay
  for (int i = 0; i < _delay; i++)
  {
    delay(5);
    ReadButtons(&gamerPos);
  }
  
}
