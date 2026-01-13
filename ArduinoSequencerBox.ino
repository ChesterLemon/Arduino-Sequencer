/*
16-Step Sequencer by Justin Slater July 2024
*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EncoderButton.h>

int currentMode = 0; //for tracking current mode
const int led[] = {12, 11, 10, 9, 8, 7, 6, 5, 12, 11, 10, 9, 8, 7, 6, 5}; //sequence LED pins, repeated for 9-16th steps in sequence
unsigned long timeNow = 0; //used for tracking how long to stay on a given step
unsigned long nextTime = 0; //used for tracking how long to stay on a given step
int sequence[] = {8, 17, 25, 17, 8, 17, 25, 17, 8, 25, 17, 25, 8, 17, 25, 17}; //sequence of 16 indexes of notes[] to be played
int sequenceIndex = 0; //used to determine note to be played and current position in sequence
String stringNotes[] = {"-- ", "C3 ", "C#3", "D3 ", "D#3", "E3 ", "F3 ", "F#3", "G3 ", "G#3", "A3 ", "A#3", "B3 ", "C4 ", 
    "C#4 ", "D4 ", "D#4", "E4 ", "F4 ", "F#4", "G4 ", "G#4", "A4 ", "A#4", "B4 ", "C5 "}; //actual string note values for LCD display
float notes[] = {0.0, 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.0, 196.0, 207.65, 220.0, 233.08, 246.94, 261.63, 
    277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.0, 415.30, 440.0, 466.16, 493.88, 523.25}; //frequencies in MHz for notes (C3 - C5)
int noteIndex = 0; //used to determine which note is being played (return value for getNote())
int BPM[] = {36, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 63, 66, 69, 72, 76, 80, 84, 88, 
    92, 96, 100, 104, 108, 112, 116, 120, 126, 132, 138, 144, 152, 160, 168, 176, 184, 192, 200, 208,
    216, 224, 232, 240, 248, 256, 264}; //standard BPMs (47 in total)
int bpmIndex = 11; //used to calculate interval in getTempo()
int tempo = 60; //used to calculate interval in getTempo()
int stepCount = 16; //number of steps to iterate through for sequence
bool stateChanged = false; //determine if new note is being set during sequencing mode
int potVal = 0; //resistance value of potentiometer for pitch change
int modVal = 0; //actual value to modify current tone by
int tonePin = 13; //pin to play audio out of (used for switching between phone jack and internal buzzer)
LiquidCrystal_I2C lcd(0x27,20,4); //set I2C address for LCD display
EncoderButton rightEb(A2, A3, A1);

void rightEbLongClick(EncoderButton& eb) { //switch audio output sources on long press of encoder button
  if (tonePin == 13) tonePin = 3;
  else tonePin = 13;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Output Changed");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
}

void rightEbClicked(EncoderButton& eb) {
  if (currentMode == 0) { //single click increments sequence
    if (eb.clickCount() == 2) {
        currentMode = 2; //play sequence
        nextTime = millis() + 30000/tempo; //calculate duration of first note
        digitalWrite(led[sequenceIndex], LOW); //turn off current LED
        sequenceIndex = 0; //go to first step in sequence before playing
        lcd.clear();
        lcd.print("Playing Sequence");
        lcd.setCursor(0, 1);
        lcd.print("BPM: " + String(tempo) + "      ");
      }
    else if (eb.clickCount() == 3) {
      currentMode = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Step Count: " + String(stepCount) + " ");
    }
    else {
      if (stateChanged == true) { //set new note only if encoder position has changed
        sequence[sequenceIndex] = noteIndex; 
        stateChanged = false;
      }
      digitalWrite(led[sequenceIndex], LOW); //turn off current LED
      if (sequenceIndex < (stepCount - eb.clickCount())) { //advance to next step in sequence
        sequenceIndex += eb.clickCount();
      }
      else sequenceIndex = 0;
      tone(tonePin, notes[sequence[sequenceIndex]], 500); //play sample tone of currently encoded note
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
    }
  }
  else if (currentMode == 1) { //switch from step-selection mode back to sequencing mode
    currentMode = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
  }
  else if (currentMode == 2) { //stop playback and go back to sequencing mode
    currentMode = 0; //pause sequence
      noTone(tonePin); //stop audio playback
      lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
      digitalWrite(led[sequenceIndex], LOW); //turn off current LED
      sequenceIndex = 0; //go back to first LED in sequence
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
  }
}

void rightEbEncoder(EncoderButton& eb) {
  if (currentMode == 0) { //select new note for currently selected LED
    if (noteIndex >= 1 && noteIndex <= 24) noteIndex += eb.increment();
    else if (noteIndex == 0 && eb.increment() == 1) noteIndex += eb.increment();
    else if (noteIndex == 25 && eb.increment() == -1) noteIndex += eb.increment();
    tone(tonePin, notes[noteIndex], 500); //play sample tone for currently selected note
    stateChanged = true;
    lcd.setCursor(0, 0);
    lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
    lcd.setCursor(0, 1);
    lcd.print("New Note: " + stringNotes[noteIndex]); //show newly selected note on LCD
  }
  else if (currentMode == 1) { //step selection mode
    if (stepCount >= 2 && stepCount <= 15) stepCount += eb.increment(); //change stepCount by +/- 1
    else if (stepCount == 1 && eb.increment() == 1) stepCount += eb.increment();
    else if (stepCount == 16 && eb.increment() == -1) stepCount += eb.increment();
    lcd.setCursor(0, 0);
    lcd.print("Step Count: " + String(stepCount) + " ");
  }
  else if (currentMode == 2) { //playback mode, change bpm
    if (bpmIndex >= 1 && bpmIndex <= 45) bpmIndex += eb.increment();
    else if (bpmIndex == 0 && eb.increment() == 1) bpmIndex += eb.increment(); //handle step down from bpmIndex 46
    else if (bpmIndex == 46 && eb.increment() == -1) bpmIndex += eb.increment(); //handle step up from bpmIndex 0
    tempo = BPM[bpmIndex];
    lcd.setCursor(0, 1);
    lcd.print("BPM: " + String(tempo) + "      ");
  }
}

void setup() {
  for(int i : led) pinMode(i, OUTPUT);//initialize all LED pins as OUTPUT
  lcd.init(); //initialize LCD
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Set Note: " + stringNotes[sequence[sequenceIndex]]); //show currently encoded note on LCD
  rightEb.setClickHandler(rightEbClicked);
  rightEb.setEncoderHandler(rightEbEncoder);
  rightEb.setLongClickHandler(rightEbLongClick);
  rightEb.setLongClickDuration(1000); //rightEbLongClick only fires after button held for 1 second
}

void loop() {
  if (currentMode == 0) { //sequencing mode
    digitalWrite(led[sequenceIndex], HIGH); //toggle on LED of current step
  }
  else if (currentMode == 2) { //play mode
    potVal = analogRead(A0)/2 - 5; //read potentiometer resistance value for pitch modulation
    digitalWrite(led[sequenceIndex], HIGH); //toggle on current LED
    tone(tonePin, (notes[sequence[sequenceIndex]] + potVal)); //play the current tone in sequence[] modified by potentiometer value
    timeNow = millis();
    if (timeNow >= nextTime) { //if it's time to play next LED in sequence, toggle off current LED and increment
      digitalWrite(led[sequenceIndex], LOW);
      if (sequenceIndex < (stepCount - 1)) sequenceIndex++; //increment sequence
      else sequenceIndex = 0;
      nextTime = nextTime + 30000/tempo; //calculate time to start next LED in sequence
    }
  }
  rightEb.update();
}
