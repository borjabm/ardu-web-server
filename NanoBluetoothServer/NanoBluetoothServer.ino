#include <SoftwareSerial.h> 
#include "DHT.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

// Digital pins used for SoftwareSerial coms with HC-05 module
#define rx 3
#define tx 2
SoftwareSerial hc05Module(rx, tx); // HC-05 RX -> Digital2 | HC-05 TX -> Digital3 

// DHT22 sensor.
// For the Nano, DHT22 seems to need to be connected to 5V
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
  
  Serial.println("***Connection Test - I am NANO***"); 
  
  dht.begin();
  
  hc05Module.begin(9600); 
  pinMode(tx, OUTPUT);
  pinMode(rx, INPUT);
  
  setUpOled();
} 

static const char GET_TEMPERATURE[] = "getTemperature";
static const char GET_HUMIDITY[] = "getHumidity";

// Char buffer used to temporarily store full incoming request
char request_buffer[20] = {0};
int i = 0; // iteration helper idx

// Char buffer that will store the requested resource.
char rqMethod[20] = {0};

// Temperature and humidity variables
float t;
float h;
void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  
  t = dht.readTemperature();
  printTempInt(t);
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  h = dht.readHumidity();
  printHumidityInt(h);
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.println(F("% "));
  
  while (hc05Module.available() > 0)
    {
        char c = hc05Module.read();
        Serial.println(c);
        if (i < 20){
          request_buffer[i] = c;
        }
        i++;

        // We expect to receive single line requests via Bluetooth, ended with \r\n - sender needs to use println.
        if (c == '\n')
        {
          parseRequest(request_buffer, sizeof(request_buffer), rqMethod, sizeof(rqMethod));
          if (strcmp(rqMethod, GET_TEMPERATURE) == 0) {
            Serial.println("Sending temp...");
            sendTemp(&hc05Module);
          }
          
          if (strcmp(rqMethod, GET_HUMIDITY) == 0) {
            Serial.println("Sending hum...");
            sendHumidity(&hc05Module);
          }
          
          i=0;
          memset(rqMethod, 0, 20);
          memset(request_buffer, 0, 20); // Clear recieved buffer
        }
    }
}

void parseRequest(char *requestBuffer, int maxRequestBuffer, char *rqMethod, int maxRqMethodSize) {
  // We expect to receive single line requests via Bluetooth, ended with \r\n - sender needs to use println.
  int rqMethodLineIdx = getChunkAndPutItInArray(rqMethod, maxRqMethodSize, requestBuffer, maxRequestBuffer, '\r', 0);
  
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

void sendTemp(SoftwareSerial *bluetoothModule) {
  // Maximum temp is 100.00. We need a maximum of 6 chars, with 2 decimal positions.
  char temp[6]={0};
  dtostrf(t, 6, 2, temp);
  Serial.println(temp);
  bluetoothModule->write('T');
  bluetoothModule->write('=');
  bluetoothModule->println(temp);
}

void sendHumidity(SoftwareSerial *bluetoothModule) {
  // Maximum humidity is 100.00. We need a maximum of 6 chars, with 2 decimal positions.
  char hum[6]={0};
  dtostrf(h, 6, 2, hum);
  Serial.println(hum);
  bluetoothModule->write('H');
  bluetoothModule->write('=');
  bluetoothModule->println(hum);
}

void setUpOled(){
  const char* zones[] = {""};
  const char* tittles[] = {"Temp.:", "Hum.:"};
  const char* units[] = {"C", "%"}; 
  
  oled.begin(&Adafruit128x32, I2C_ADDRESS);

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
}

void printTempInt(double temperature){
  printOled(firstRowX0, firstRowX0 + valueFieldWidth, 0, rHeight, temperature);
  // printOled(0, temperature);
}

void printHumidityInt(double humidity){
  printOled(firstRowX0, firstRowX0 + valueFieldWidth, rHeight, rHeight + rHeight, humidity);
  // printOled(rHeight, humidity);
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
