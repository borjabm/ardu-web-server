#include <SPI.h>
#include "DHT.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <SoftwareSerial.h> 

// Digital pins used for SoftwareSerial coms with HC-05 module
#define rx 3
#define tx 2
SoftwareSerial hc05Module(rx, tx); // HC-05 RX -> Digital2 | HC-05 TX -> Digital3 

// DHT22 sensor.
// For the UNO, DHT22 works with 3.3V
#define DHTPIN 7     
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Oled I2C address
#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

/** Oled specific fields to help move cursor around **/
int valueFieldWidth = 0;
int firstRowX0 = 0;  // First value column
int secondRowX0 = 0;  // First value column
int rHeight = 0;      // Rows per line.

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Uno Temp/Hum - Oled - Bluetooth poller");
  
  dht.begin();
  hc05Module.begin(9600); 
  Serial.println("***Connection Test - I am UNO***"); 
  pinMode(tx, OUTPUT);
  pinMode(rx, INPUT);
  setUpOled();
}

bool waitTemperatureResponse = false;
bool waitHumidityResponse = false;
bool waitingForAResponse = false;
char request_buffer[20] = {0};
char magnitud[20] = {0};
char reading[20] = {0};
int i = 0;

float t;
float h;

int waitTimer = 0;
int maxTimeOut = 30000;
int delayPeriod = 2000;

char nextReading = 't';
char const READ_TEMP = 't';
char const READ_HUM = 'h';

void loop() {
  // Wait a few seconds between measurements.
  delay(delayPeriod);

  
  t = dht.readTemperature();
  printTempInt(t);
//  Serial.print(F("Temperature: "));
//  Serial.print(t);
//  Serial.println(F("°C "));

  h = dht.readHumidity();
//  Serial.print(F("Humidity: "));
//  Serial.print(h);
//  Serial.println(F("% "));
  printHumidityInt(h);

  Serial.print("nextReading...");
  Serial.println(nextReading);
  if (!waitingForAResponse && nextReading == READ_TEMP) {
    Serial.println("Requesting temp...");
    hc05Module.println("getTemperature");
    waitingForAResponse = true;
  }
  
  if (!waitingForAResponse && nextReading == READ_HUM) {
    Serial.println("Requesting hum...");
    hc05Module.println("getHumidity");
    waitingForAResponse = true;
  }
  
  while (hc05Module.available() > 0) {
    char c = hc05Module.read();
    if (i < 20) {
      request_buffer[i] = c;
    }
    i++;
    
    // We expect to receive single line responses via Bluetooth, ended with \r\n - sender needs to use println.
    // T= 24.60\r\n, T=-24.60\r\n, T=100.00\r\n
    // H= 24.60\r\n, H=100.00\r\n 
    if (c == '\n') {
      parseResponse(request_buffer, sizeof(request_buffer), magnitud, sizeof(magnitud), reading, sizeof(reading));
      Serial.println("Received!");
      Serial.println(magnitud);
      Serial.println(reading);
      if(strcmp("T",magnitud) == 0){
        Serial.println("Received temp!");
        printTempOut(atof(reading));
        nextReading = READ_HUM;
        i = 0;
        memset(request_buffer, 0, 20); // Clear recieved buffer
        waitingForAResponse = false;
        break;
      }
      
      if(strcmp("H",magnitud) == 0){
        Serial.println("Received hum!");
        printHumidityOut(atof(reading));
        nextReading = READ_TEMP;
        i = 0;
        memset(request_buffer, 0, 20); // Clear recieved buffer
        waitingForAResponse = false;
        break;
      } 
    }
   }

  // If we have sent a request, and still are waiting for a response, increase waitTimer.
  if (waitingForAResponse) {
    waitTimer = waitTimer + delayPeriod;
  }
  
  if (waitTimer >= maxTimeOut) {
    if (waitingForAResponse){
      waitingForAResponse = false;
    }
    waitTimer = 0;
  }
  
}

void parseResponse(char *requestBuffer, int maxRequestBuffer, char *rqMethod, int maxRqMethodSize, char *reading, int maxReadingSize) {
  // We expect to receive single line requests via Bluetooth, ended with \r\n - sender needs to use println.
  int magnitudeIdx = getChunkAndPutItInArray(rqMethod, maxRqMethodSize, requestBuffer, maxRequestBuffer, '=', 0);
  int readingIdx = getChunkAndPutItInArray(reading, maxReadingSize, requestBuffer, maxRequestBuffer, '\r', magnitudeIdx);
}

int getChunkAndPutItInArray(char *destinationArray, int destinationArraySize, char *sourceArray, int sourceArraySize, char endChar, int shift) {
  int maxDestinationIdx = destinationArraySize -1;
  int chunkIdx = 0;
  int idx = shift;
  while (true){
    if (idx >= sourceArraySize || sourceArray[idx] == '\0' || sourceArray[idx] == endChar || chunkIdx == maxDestinationIdx) {
      destinationArray[chunkIdx] = '\0';
      chunkIdx++; // increase the traversed positions one more time before exiting.
      break;
    } 
    destinationArray[chunkIdx] = sourceArray[idx];
    chunkIdx++;
    idx++;
  }
  return chunkIdx;
}

void setUpOled(){
  const char* zones[] = {"IN", "OUT"};
  const char* tittles[] = {"Temp.:", "Hum.:"};
  const char* units[] = {"C", "%"}; 
  
  oled.begin(&Adafruit128x64, I2C_ADDRESS);

  oled.setFont(Callibri15);
  oled.clear();
  oled.print("Setup ready!");
  delay(1000);
  oled.clear();

  rHeight = oled.fontRows();
  valueFieldWidth = oled.strWidth("100.0") + 3;

  // First group
  int firstRowXDelta0 = printZones(zones[0], 0);
  firstRowX0 = printLabels(tittles, 2, firstRowXDelta0, 0, rHeight);
  printLabels(units, 2, firstRowX0 + valueFieldWidth, 0, rHeight);
  delay(1000); 

  // Second group
  int secondRowXDelta0 = printZones(zones[1], 2*rHeight);
  secondRowX0 = printLabels(tittles, 2, secondRowXDelta0, 2*rHeight, rHeight);
  printLabels(units, 2, secondRowX0 + valueFieldWidth, 2*rHeight, rHeight);
  delay(1000); 
}

void printTempInt(double temperature){
  printOled(firstRowX0, firstRowX0 + valueFieldWidth, 0, rHeight, temperature);
  // printOled(0, temperature);
}

void printHumidityInt(double humidity){
  printOled(firstRowX0, firstRowX0 + valueFieldWidth, rHeight, rHeight + rHeight, humidity);
  // printOled(rHeight, humidity);
}


void printTempOut(double temperature){
  printOled(secondRowX0, secondRowX0 + valueFieldWidth, 2 * rHeight, 3 * rHeight, temperature);
}

void printHumidityOut(double humidity){
  printOled(secondRowX0, secondRowX0 + valueFieldWidth, 3 * rHeight, 4 * rHeight, humidity);
}

void printOled(int x0, int x1, int y0, int y1, double valueToPrint){
  oled.clear(x0, x1 - 1, y0, y1 - 1);
  oled.print(valueToPrint, 1);
}

int printZones(char *zone, int yDelta){
  oled.setFont(System5x7);
  oled.set2X();
  oled.setCursor(0, yDelta);
  oled.print(zone);
  int x00 = oled.strWidth(zone) +1;
  oled.setFont(Callibri15);
  delay(1000);
  oled.set1X();
  return x00;
}

int printLabel(char *value, int x0, int y0, int yDelta){
  oled.setCursor(x0, y0 + yDelta);
  oled.print(value);
  return oled.strWidth(value) + x0 + 4;
}

int printLabels(char *labels[], int numberOfLabels, int x0, int y0, int rowHeight) {
  int width = 0;
  int maxWidth = 0;
  for (int i = 0; i < numberOfLabels; i++) {
    width = printLabel(labels[i], x0, y0, i*rowHeight);
    maxWidth = maxWidth < width ? width : maxWidth; 
    delay(1000);
  }
  return maxWidth;
}
