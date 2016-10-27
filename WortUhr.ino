// ToDo:
// Funkstatus Null Funk / Funk, Status in Variable (lastSync)
// Config Sprachen ("Dreiviertel")
// Es ist nacht

// Pinbelegung:
// 5V -- R10k -- A7 -- LDR(Fotowiderstand) -- GND
// A4 -- SDA (RTC, Real Time Clock)
// A5 -- SCL (RTC)
// Stromversorgung RTC 5V, GND
// C1000µF zwischen 5V und GND NeoPixel Strip
// D6 -- R300 -- DataIn NeoPixel Strip Matrix
// D7 -- R300 -- DataIn NeoPixel Strip 4Leds
// (Button normal: GND -- R10k  -- Pin -- Button -- 5V --- )
// Button mit internem Pullup: Pin -- Button -- GND  [pinMode(pinnummer, INPUT_PULLUP);]  LOW when button pressed
// D3 -- Button -- GND (Set)
// D4 -- Button -- GND (Enter)
// D2 -- DCF77 Signal
// Stromversorgung DCF 3,3V (!!!), GND

// Color definitions
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

// Define EEPROM Adresses to save configuration
#define ADDR_WRITEESIST 0  // Immer "Es ist" schreiben
#define ADDR_USEMEANTIME 1 // halb anzeigen von x:28:30 bis x:32:30
#define ADDR_BRIGHTNESS 2 //Helligkeitseinstellung

// Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>          // I2C Bibliothek, für RTC
#include "DCF77.h"
#include "RVstructWord2.h" // DOKU: http://forum.arduino.cc/index.php?topic=44890.0
#include <EEPROM.h>

// SETUP Funkuhr DCF77
#define DCF_PIN 2       // Connection pin to DCF-77 device
#define DCF_INTERRUPT 0 // Interrupt number associated with pin
DCF77 DCF = DCF77(DCF_PIN,DCF_INTERRUPT);

// SETUP Matrix and Display, first Led top left
#define MATRIXWIDTH 11
#define MATRIXHEIGHT 10
#define MATRIXPIN 6        // connection Pin
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIXWIDTH, MATRIXHEIGHT, MATRIXPIN, 
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

// SETUP 4Pixel Strip (Minutes), first Led top left
#define PIXELPIN 7        // connection Pin
#define NUMPIXELS 4       // Number of pixels on NeoPixel strip
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

const Word w_es = {0, 0, 2}; // x-Koordinate, y-Koord., Laenge. Nutzung: w_es.x = 12;
const Word w_ist = {3, 0, 3};
const Word w_fuenfM = {7, 0, 4}; // Fünf oben bei Minuten
const Word w_zehnM = {0,1,4};    // Zehn oben
const Word w_viertel = {4,2,7};
const Word w_dreiviertel = {0,2,11};
const Word w_zwanzig = {4,1,7};
const Word w_vor = {0,3,3};
const Word w_nach = {7,3,4};
const Word w_halb = {0,4,4};
const Word w_ein = {2,5,3};
const Word w_eins = {2,5,4};
const Word w_zwei = {0,5,4};
const Word w_drei = {7,7,4};
const Word w_vier = {7,6,4};
const Word w_fuenf = {7,4,4};
const Word w_sechs = {5,5,5};
const Word w_sieben = {5,8,6};
const Word w_acht = {1,8,4};
const Word w_neun = {3,7,4};
const Word w_zehn = {0,7,4};
const Word w_elf = {5,4,3};
const Word w_zwoelf = {0,9,5};
const Word w_uhr = {8,9,3};
const Word w_null = {1,6,4};
const Word w_nacht = {0,8,5};
const Word w_ulli = {2,6,4};
const Word w_sechsy = {5,5,6};
const Word w_rvi = {6,6,3};
const Word w_tilt = {3,4,4};
const Word w_funkuhr = {4,9,7};
const Word w_funk = {4,9,4};

const uint32_t timeColor = pixels.Color(255, 255, 255); //Kritisch: gleiche Farben matrix und pixel!?
uint32_t farbeStatus = RED; //Statusfarbe Funkuhr
int secondsShown = -1; //Gerade angezeigte Zeit
long lastSyncDCF; //Zeitpunkt letzter SyncVersuch RTC mit DCF (in Sekunden seit 1970)
long lastSyncDCFsuccess; //Zeitpunkt letzter erfolgreicher Sync
#define delayval 50
#define delayvalConfig 550
#define brightnessPin A7 //Light sensor
#define buttonSet 3 //Pin Button Set
#define buttonEnter 4 //Pin Button Enter
int brightness = 50; // Brightness (0-255)
int brightnessConfig; // Brightness configuration value (1-20)
time_t time;
boolean write_esist; //Config: "Es Ist" schreiben
boolean useMeanTime; //Config: schon 2,5 Minuten vor halb "halb" anzeigen

void setup() {
  initConfig();  // Read configuration from EEPROM
  // Serial.begin(9600); // Seriellen Monitor einschalten //!
  setSyncProvider(RTC.get);   // the function to get the time from the RTC every 5 minutes
  setSyncInterval(300);
  DCF.Start();
  lastSyncDCF = now() - 3420; //Start bei 3420, um nach 3 Minuten ersten Sync zu probieren (bei 3600)
                             //now() gibt die Zeit in Sekunden seit 1970 wieder
  lastSyncDCFsuccess = now();
  matrix.begin();
  matrix.setTextWrap(false); //Kein zeilenumbruch, damit Marque Effekt funktioniert
  matrix.setBrightness(brightness);
  pixels.begin();
  pixels.setBrightness(brightness);
  pinMode(buttonSet, INPUT_PULLUP);  // Set
  pinMode(buttonEnter, INPUT_PULLUP);  // Enter
  initShow();    // Startup Animation
}

void loop() {
  if (digitalRead(buttonEnter) == LOW && digitalRead(buttonSet) == LOW)
     configMenu();     // Both buttons pressed > config
   if (digitalRead(buttonEnter) == LOW){
     showSeconds();     // Enter pressed -> Sekunden zeigen
     //showMatrixAnim();
   }
   if (digitalRead(buttonSet) == LOW)
     showDCFStatus();     // Set pressed -> Funkuhrstatus anzeigen

  uint8_t writeMinute = minute();
  uint8_t writeHour = hour();

  if (useMeanTime) {
    if (second() > 30) writeMinute = writeMinute + 3;
    else writeMinute = writeMinute + 2;
    if (writeMinute >= 60) {
      writeHour = hour() + 1;
      writeMinute = writeMinute - 60;
    }
  }

  if (secondsShown != second()){
    brightness = 50 + 10 * brightnessConfig - (0.0439 + 0.00878 * brightnessConfig) * analogRead(brightnessPin);
    matrix.setBrightness(brightness);
    pixels.setBrightness(brightness);
    writeTime(writeMinute, writeHour, timeColor);
    writeMinuteLeds(timeColor, minute(), second(), useMeanTime);
    matrix.show();
    pixels.show();
    secondsShown = second();
  } // if

  if (minute() == 0 && second() == 0) { //Animation zum Beginn einer neuen Stunde
    showMatrixAnim();
  }

  if (lastSyncDCF < now() - 3600)
     setDCFTime();  // alle 60 Minuten mit Funkuhr synchronisieren (3600s)
  
  delay(delayval);
} // void loop

void writeTime(uint8_t getminute, uint8_t gethour, uint32_t getcolor) {
  matrix.fillScreen(0);
  switch (getminute/5) {
    case 0: // volle Stunde
    case 12: //sollte nie vorkommen
      writeWord(w_es, getcolor);
      writeWord(w_ist, getcolor);
      writeHour(gethour, true, getcolor);
      writeWord(w_uhr, getcolor);
      break;
    case 1: //fünf nach
      writeEsIst(getcolor);
      writeWord(w_fuenfM, getcolor);
      writeWord(w_nach, getcolor);
      writeHour(gethour, false, getcolor);
      break;
    case 2: //zehn nach
      writeEsIst(getcolor);
      writeWord(w_zehnM, getcolor);
      writeWord(w_nach, getcolor);
      writeHour(gethour, false, getcolor);
      break;
    case 3: //viertel nach
      writeEsIst(getcolor);
      writeWord(w_viertel, getcolor);
      writeWord(w_nach, getcolor);
      writeHour(gethour, false, getcolor);
      break;    
    case 4: //zwanzig nach
      writeEsIst(getcolor);
      writeWord(w_zwanzig, getcolor);
      writeWord(w_nach, getcolor);
      writeHour(gethour, false, getcolor);
      break; 
    case 5: //fünf vor halb
      writeEsIst(getcolor);
      writeWord(w_fuenfM, getcolor);
      writeWord(w_vor, getcolor);
      writeWord(w_halb, getcolor);
      writeHour(gethour+1, false, getcolor);
      break; 
    case 6: //halb
      writeWord(w_es, getcolor);
      writeWord(w_ist, getcolor);
      // writeEsIst(getcolor);
      writeWord(w_halb, getcolor);
      writeHour(gethour+1, false, getcolor);
      break; 
    case 7: //fünf nach halb
      writeEsIst(getcolor);
      writeWord(w_fuenfM, getcolor);
      writeWord(w_nach, getcolor);
      writeWord(w_halb, getcolor);
      writeHour(gethour+1, false, getcolor);
      break; 
    case 8: //zwanzig vor
      writeEsIst(getcolor);
      writeWord(w_zwanzig, getcolor);
      writeWord(w_vor, getcolor);
      writeHour(gethour+1, false, getcolor);
      break; 
    case 9: //viertel vor
      writeEsIst(getcolor);
      writeWord(w_viertel, getcolor);
      writeWord(w_vor, getcolor);
      writeHour(gethour+1, false, getcolor);
      break; 
    case 10: //zehn vor
      writeEsIst(getcolor);
      writeWord(w_zehnM, getcolor);
      writeWord(w_vor, getcolor);
      writeHour(gethour+1, false, getcolor);
      break;
    case 11: //fünf vor
      writeEsIst(getcolor);
      writeWord(w_fuenfM, getcolor);
      writeWord(w_vor, getcolor);
      writeHour(gethour+1, false, getcolor);
      break;
   } //switch
   //matrix.show(); //Steht jetzt in LOOP
} //void writeTime

void writeEsIst(uint32_t getcolor) {
    if (write_esist) {
      writeWord(w_es, getcolor);
      writeWord(w_ist, getcolor);
    }
}//void writeEsIst

void writeHour(int gethour, boolean fullhour, uint32_t getcolor) {
  switch(gethour) {
    case 1:
    case 13:
      if (fullhour)
         writeWord(w_ein, getcolor);
      else
        writeWord(w_eins, getcolor);
      break;
    case 2:
    case 14:
      writeWord(w_zwei, getcolor);
      break;
    case 3:
    case 15:
      writeWord(w_drei, getcolor);
      break;
    case 4:
    case 16:
      writeWord(w_vier, getcolor);
      break;
    case 5:
    case 17:
      writeWord(w_fuenf, getcolor);
      break;
    case 6:
    case 18:
      writeWord(w_sechs, getcolor);
      break;
    case 7:
    case 19:
      writeWord(w_sieben, getcolor);
      break;
    case 8:
    case 20:
      writeWord(w_acht, getcolor);
      break;
    case 9:
    case 21:
      writeWord(w_neun, getcolor);
      break;
    case 10:
    case 22:
      writeWord(w_zehn, getcolor);
      break;
    case 11:
    case 23:
      writeWord(w_elf, getcolor);
      break;    
    case 12:
      writeWord(w_zwoelf, getcolor);
      break;
    case 0:
    case 24: // kommt nur vor, wenn useMeanTime TRUE (23:58 --> 24:00)
      if (fullhour)
        writeWord(w_null, getcolor);
      else
        writeWord(w_zwoelf, getcolor);
      break;
   } //switch
} // void writeHour

void writeWord(Word writeme, uint32_t getcolor) { // WriteWord als Linie mit GFX-Library
  matrix.drawFastHLine(writeme.x, writeme.y, writeme.l, getcolor);  
} //End writeWord

void setDCFTime(){
  time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
  if (DCFtime!=0) {
    //Serial.println("Time is updated to DCF time");
    RTC.set(DCFtime);
    farbeStatus = matrix.Color(0, 0, 255);
    lastSyncDCF = now();
    lastSyncDCFsuccess = now();
  }
  else {
    if (lastSyncDCFsuccess < (now() - 172800)){
      farbeStatus = matrix.Color(255, 0, 0);   // Wenn letzter erfolgreicher Sync >48h her (172800s)
    }
    //Serial.print("No DCF Time available! Last Sync: ");
    //Serial.println(lastSyncDCF);
    lastSyncDCF = lastSyncDCF + 20; // in 20 Sekunden nochmal probieren
  }
}

void writeText(String getText, uint32_t getColor) { //with Marque effect
  matrix.setTextColor(getColor);
  for (int i=0; i<getText.length()*6+11; i++){
    matrix.fillScreen(0);
    matrix.setCursor(10-i, 1);
    //getText.toUpperCase();
    matrix.print(getText);
    matrix.show();
    delay(120);
  }
} //void writeText

void writeShortText(String getText, uint32_t getColor, uint16_t duration) { //without Marque, only 2 letters
  matrix.setTextColor(getColor);
  matrix.fillScreen(0);
  matrix.setCursor(0, 1);
  matrix.print(getText);
  matrix.show();
  delay(duration);
} //void writeShortText

void writeDigits(uint8_t getNumber, uint32_t getColor) {
  uint8_t xpos = 0;
  matrix.setTextColor(getColor);
  matrix.fillScreen(0);
  if (getNumber < 10)
    xpos = 3; 			// center one digit numbers
  matrix.setCursor(xpos, 1);
  matrix.print(getNumber);
  matrix.show();
} //void writeText

void configMenu() {
  //matrix.setBrightness(brightness+30);
  //delay(1400); //Zeit, um Tasten loszulassen
  writeText("STD", RED);
  do {
    if (digitalRead(buttonSet) == LOW)
      addHour();
    writeDigits(hour(), BLUE);
    delay(delayvalConfig);
  } while (digitalRead(buttonEnter) == HIGH); 
  
  writeText("MIN", RED);
  do {
    if (digitalRead(buttonSet) == LOW)
      addMinute();
    writeDigits(minute(), BLUE);
    delay(delayvalConfig*0.75);     // Etwas schnellerer Minutensprung
  } while (digitalRead(buttonEnter) == HIGH);
  
  writeText("HELL", RED);
  do {
    if (digitalRead(buttonSet) == LOW)
       if (brightnessConfig == 20)
          brightnessConfig = 1;
       else
          brightnessConfig++;    
    writeDigits(brightnessConfig, BLUE);
    delay(delayvalConfig);
  } while (digitalRead(buttonEnter) == HIGH);
  EEPROM.write(ADDR_BRIGHTNESS, brightnessConfig);

  writeText("MEHR", RED); //Ausstiegsmöglichkeit!
  do {
    writeShortText("->", BLUE, 1000);
    if (digitalRead(buttonSet) == LOW)
		return;
    delay(40);
  } while (digitalRead(buttonEnter) == HIGH);

  writeText("ES IST", RED);
  do {
    if (digitalRead(buttonSet) == LOW) {
       if (write_esist)
          write_esist = 0;
       else
          write_esist = 1;    
    }
    writeDigits(write_esist, BLUE);
    delay(delayvalConfig);
  } while (digitalRead(buttonEnter) == HIGH);
  EEPROM.write(ADDR_WRITEESIST, write_esist);  
  
  writeText("MITTELZEIT", RED);
  do {
    if (digitalRead(buttonSet) == LOW) 
       if (useMeanTime)
          useMeanTime = 0;
       else
          useMeanTime = 1;    
    writeDigits(useMeanTime, BLUE);
    delay(delayvalConfig);
  } while (digitalRead(buttonEnter) == HIGH);
  EEPROM.write(ADDR_USEMEANTIME, useMeanTime); 
} //void configMenu

void addHour(){
  setTime(hour()+1,minute(),second(),day(),month(),year());  // Stunde +1,
  RTC.set(now());                    // sending the onboard time to the RTC
  delay(delayvalConfig);
}

void addMinute(){
  if (minute() == 59)
    setTime(hour(),0,0,day(),month(), year());  // Minute +1, Sekunden auf Null setzen
  else  
    setTime(hour(),minute()+1,0,day(),month(),year()); //Minute +1, Sekunden auf Null setzen    
  RTC.set(now());                    // sending the onboard time to the RTC
  delay(delayvalConfig);
}

void minuteLedsOff() {
  for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, 0);
  }
}

void writeMinuteLeds(uint32_t getcolor, uint8_t getminute, uint8_t getsecond, boolean getUseMeanTime) {
  minuteLedsOff();
  if (getUseMeanTime) {  // 2 rote links abnehmend, 2 blaue rechts zunehmend
    uint16_t timex = (getminute%5)*60 + getsecond;
    uint32_t colorDown = pixels.Color(255, 50, 50);
    uint32_t colorUp = pixels.Color(50, 50, 255);
	if (timex >= 151 && timex <= 269)
		pixels.setPixelColor(0, colorDown); // Led oben links
	if (timex >= 30 && timex <= 150)
		pixels.setPixelColor(1, colorUp); // Led oben rechts
	if (timex >= 90 && timex <= 150)
		pixels.setPixelColor(2, colorUp); // Led unten rechts
	if (timex >= 151 && timex <= 209)
		pixels.setPixelColor(3, colorDown); // Led unten links
  }
  else { // Pixel im Uhrzeigersinn
    for (int i=0; i<getminute%5; i++) {
      //pixels.setPixelColor(i, getcolor);   // 1. Led oben links
      pixels.setPixelColor((i+1)%4, getcolor);  // 1. Led oben rechts (ungetestet)
    }
  }
}

void initConfig() {
  if (EEPROM.read(ADDR_WRITEESIST) > 1){
     EEPROM.write(ADDR_WRITEESIST, 0);
  }
  if (EEPROM.read(ADDR_USEMEANTIME) > 1){
     EEPROM.write(ADDR_USEMEANTIME, 0);
  }
  if (EEPROM.read(ADDR_BRIGHTNESS) > 20){
  EEPROM.write(ADDR_BRIGHTNESS, 10);
  }
  write_esist = EEPROM.read(ADDR_WRITEESIST);
  useMeanTime = EEPROM.read(ADDR_USEMEANTIME);
  brightnessConfig = EEPROM.read(ADDR_BRIGHTNESS);
}

void initShow() {   // Startup animation
	writeWord(w_rvi, RED);
	matrix.show();
	delay(2000);
}

void showDCFStatus(){  
  uint8_t i = 0;
  do {
    delay(50);
    if (digitalRead(buttonSet) == LOW && digitalRead(buttonEnter) == HIGH) // break wenn anderer button auch gedrückt oder dieser button losgelassen
      i++;
    else
      return;
  } while (i < 20); //ca. 1s
  matrix.fillScreen(0);
  writeWord(w_funk, farbeStatus);
  matrix.show();
  delay(3000);
}

void showSeconds(){  
  uint8_t i = 0;
  do {
    delay(50);
    if (digitalRead(buttonEnter) == LOW && digitalRead(buttonSet) == HIGH) // break wenn anderer button auch gedrückt oder dieser button losgelassen
      i++;
    else
      return;
  } while (i < 20); //ca. 1s
  for (i=0;i<8;i++){
    writeDigits(second(), BLUE);
    delay(1000);
   }
}

void showMatrixAnim() {
  uint16_t runtimes = 0;
  uint16_t i;
  uint16_t j;
  uint32_t dotColor;
  uint32_t vektor[10] = {40, 40, 40, 40, 60, 70, 100, 150, 200, 255}; // generische Spalte mit 10 Zeilen
  uint32_t vstart[11] = {3, 5, 7, 6, 5, 10, 1, 10, 2, 1, 3}; // Startpunkt Spalte, von hier nach oben gen. Spalte schreiben

  do {
    matrix.fillScreen(0);
    // matrix schreiben:
    for (i = 0; i <= 10; i++){  // Spalten
      for (j = 0; j <= 9; j++) { // Zeilen
        dotColor = matrix.Color(0, vektor[j], 0);
        matrix.drawPixel(i, (j+vstart[i])%10, dotColor); // Spalte, Zeile, Farbe      
      }
    }
    matrix.show();
    for (i=0;i<=10;i++){    // vector start: increment +1
      vstart[i]++;
    }
    delay(90);
    runtimes++;
  } while (runtimes < 50);
}

