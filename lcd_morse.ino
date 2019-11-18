// TODO: Use extra resistors and switcher to make high/low volume (sound module)

// TODO: Create HashMap class for other projects (String array[2])

#include <LiquidCrystal.h>
#include <ResponsiveAnalogRead.h>

#include "morse.h"

/////
String words[] = {
  "apply", "awake", "berry", "class", "clean", "curvy", "enjoy", "fetch", "funny", "house", "match", "occur", "peace", "press", "ratty", "right",
  //  "snail", "start", "state", "straw", "sugar", "swear", "tough", "value", "weave"
  //  "tough"
};
/////

const int morseLength = sizeof(morse) / (sizeof(String) * 2);

const byte ledPin = 8;
const int rotationPin = A4;
const byte greenButtonPin = 6;
const byte redButtonPin = 7;
const byte soundPin = 9;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// s
const int LCD_S_WIDTH = 16;
const int LCD_S_HEIGHT = 2;

const int MOTION_S_MAX = 1024;

// g
String stage = ""; // STARTUP/SEQUENCE/RESULT/SCORE
boolean component_LCD = false;
boolean component_LED = false;
boolean component_SOUND = false;

String currentWord = "";
String currentSequence = "";
int sequenceIndex = 0;
long sequenceEndedT = 0;
const int gameEndDelay = 2e3;

const int charDelay = 125; // delay between characters
const int spaceDelay = 200; // delay for space (C_C)
const int shortDelay = 125; // short char duration (.)
const int longDelay = 250; // long char duration (-)

long lastSPlayedTime = 0;
long lastSPlayedTimeSW = 0;
boolean isActiveS = false;
boolean guessRight = false;

boolean greenPressed = false;
boolean redPressed = false;

const String componentPages[3] = { "LCD", "LED", "SOUND" };

String LCDL_1 = "";
String LCDL_V_1 = "";
String LCDL_2 = "";
String LCDL_V_2 = "";
//
void startGame(String csequence) {
  if (csequence == "") {
    randomSeed(millis());
    // generate sequence
    String gword = words[random(sizeof(words) / sizeof(String))];
    gword.trim();

    currentSequence = generateMorse(gword);
    currentWord = gword;
  }

  // else:
  //   currentSequence => the same sequence
  //   currentWord => the same word

  sequenceIndex = 0;
  sequenceEndedT = 0;
  stage = "SEQUENCE";
  guessRight = false;

  return;
}
//
String generateMorse(String gword) {
  String gsequence = "";

  for (int ma = 0; ma < gword.length(); ma++) {
    gsequence += toMorse(String(gword[ma])) + " ";
  }

  gsequence.trim();

  return gsequence;
}
//
String toMorse(String l) {
  l.toLowerCase();
  String letter;

  for ( int ma = 0; ma < morseLength; ma++ ) {
    String a = morse[ma][1];
    a.toLowerCase();

    if (a == l) {
      letter = morse[ma][0];
      break;
    }
  }

  return letter;
}
//
void startSetting() {
  component_LCD = component_LED = component_SOUND = false;
  stage = "STARTUP";
}
//
void setup() {
  Serial.begin(9600);
  lcd.begin(LCD_S_WIDTH, LCD_S_HEIGHT); // intialize display with number of rows and columns

  //
  pinMode(ledPin, OUTPUT);
  pinMode(soundPin, OUTPUT);

  pinMode(rotationPin, INPUT);
  pinMode(greenButtonPin, INPUT);
  pinMode(redButtonPin, INPUT);
  //
  startSetting();
}

void onGreenClick() {}

void onRedClick() {}

void loop() {
  const boolean just_greenClicked = digitalRead(greenButtonPin) == HIGH && greenPressed == false;
  const boolean just_redClicked = digitalRead(redButtonPin) == HIGH && redPressed == false;

  if (stage == "STARTUP") {
    const int oi = map(analogRead(rotationPin), MOTION_S_MAX, 0, 0, sizeof(componentPages) / sizeof(String) + 1);

    if (oi != 3) {
      const boolean oib = (oi == 0) ?
                          component_LCD :
                          (oi == 1) ?
                          component_LED :
                          (oi == 2) ?
                          component_SOUND
                          : false;
      const String oibs = (oib) ? "yes" : "no";

      LCDL_V_1 = String(oi + 1) + ". " + componentPages[oi];
      LCDL_V_2 = "SELECTED: " + oibs;

      if (just_greenClicked || just_redClicked) {
        if (oi == 0) component_LCD = !component_LCD;
        else if (oi == 1) component_LED = !component_LED;
        else if (oi == 2) component_SOUND = !component_SOUND;
      }
    } else {
      if (component_LCD || component_LED || component_SOUND) {
        LCDL_V_1 = "Ready to play?";
        LCDL_V_2 = "> Press GREEN... <";
        if (digitalRead(greenButtonPin) == HIGH) {
          startGame("");
        }
      } else {
        LCDL_V_1 = "Choose atl one";
        LCDL_V_2 = "component";
      }
    }
  } else if (stage == "SEQUENCE") {
    if (sequenceIndex + 1 <= currentSequence.length()) { // is playing
      const String current = String(currentSequence[sequenceIndex]);

      if (millis() - lastSPlayedTimeSW >= charDelay) {
        if (!isActiveS && (current == "." || current == "-")) {
          isActiveS = true;

          if (component_LED) digitalWrite(ledPin, HIGH);
          if (component_SOUND) digitalWrite(soundPin, HIGH);

          lastSPlayedTime = millis();
        } else if (!isActiveS && current == " ") {
          isActiveS = true;
          lastSPlayedTime = millis();
        } else if (
          isActiveS && (
            (current == "." && millis() - lastSPlayedTime >= shortDelay) ||
            (current == "-" && millis() - lastSPlayedTime >= longDelay) ||
            (current == " " && millis() - lastSPlayedTime >= spaceDelay)
          )
        ) {
          isActiveS = false;
          if (component_LED) digitalWrite(ledPin, LOW);
          if (component_SOUND) digitalWrite(soundPin, LOW);

          sequenceIndex++;
          lastSPlayedTimeSW = millis();
        }
      }

      // display current symbol
      if (component_LCD) {
        LCDL_V_1 = currentSequence[sequenceIndex];
        LCDL_V_1 = (LCDL_V_1 != " ") ? LCDL_V_1 : "SPACE";

        LCDL_V_2 = "";
      } else {
        LCDL_V_1 = "Playing...";
        LCDL_V_2 = "";
      }
    } else {
      if (sequenceEndedT == 0) {
        sequenceEndedT = millis();

        if (component_LCD) { // display the whole sequence
          if (currentSequence.length() > LCD_S_WIDTH) {
            LCDL_V_1 = currentSequence.substring(0, LCD_S_WIDTH);
            LCDL_V_2 = currentSequence.substring(LCD_S_WIDTH);
          } else {
            LCDL_V_1 = currentSequence;
            LCDL_V_2 = "";
          }
        }
      } else if (millis() - sequenceEndedT >= gameEndDelay) { // 3.
        LCDL_V_1 = "Replay? > GREEN";
        LCDL_V_2 = "> RED to dismiss <";

        if (just_greenClicked) { // play again
          startGame(currentSequence);
        } else if (just_redClicked) { // dismissed
          stage = "RESULT";
        }

      }
    }
  } else if (stage == "RESULT") {
    const int wl = sizeof(words) / sizeof(String); // words.length
    const int pw = map(analogRead(rotationPin), MOTION_S_MAX, 0, 0, wl); // selected word index

    LCDL_V_1 = "- " + String(words[pw]) + " -";
    LCDL_V_2 = "> " + String(pw + 1) + "/" + wl + " <";

    if (just_greenClicked) { // validate word
      guessRight = currentWord == words[pw];
      stage = "SCORE";
    }
  } else if (stage == "SCORE") {
    String a = (guessRight) ? "GREAT JOB!" : "NO, WORD: " + currentWord;
    LCDL_V_1 = a;
    LCDL_V_2 = "continue > /ANY/";

    if (just_greenClicked || just_redClicked) {
      startSetting();
    }
  }

  // Display lines
  if (LCDL_1 != LCDL_V_1 || LCDL_2 != LCDL_V_2) {
    // r
    LCDL_1 = LCDL_V_1;
    LCDL_2 = LCDL_V_2;
    // c
    lcd.clear();
    // p
    lcd.setCursor(0, 0); // 1 symbol, 1 line
    lcd.print(LCDL_1);
    lcd.setCursor(0, 1); // 1 symbol, 2 line
    lcd.print(LCDL_2);
  }

  // Buttons action
  if (just_greenClicked) { // ::click
    greenPressed = true;
    onGreenClick();
  } else if (digitalRead(greenButtonPin) == false && greenPressed == true) { // ::release
    greenPressed = false;
  }
  if (just_redClicked) { // ::click
    redPressed = true;
    onRedClick();
  } else if (digitalRead(redButtonPin) == false && redPressed == true) { // ::release
    redPressed = false;
  }
}
