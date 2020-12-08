#include <SoftwareSerial.h> 
#include "DHT.h"

// Digital pins used for SoftwareSerial coms with HC-05 module
#define rx 3
#define tx 2
SoftwareSerial hc05Module(rx, tx); // HC-05 RX -> Digital2 | HC-05 TX -> Digital3 

// DHT22 sensor.
#define DHTPIN 7     
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(9600); 
  hc05Module.begin(9600); 
  Serial.println("***Connection Test - I am NANO***"); 
  pinMode(tx, OUTPUT);
  pinMode(rx, INPUT);

  dht.begin();
} 

static const char GET_TEMPERATURE[] = "getTemperature";
static const char GET_HUMIDITY[] = "getHumidity";

// Char buffer used to temporarily store full incoming request
char request_buffer[20] = {0};
int i = 0; // iteration helper idx

// Char buffer that will store the requested resource.
char rqMethod[20] = {0};
void loop() {
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
  bluetoothModule->println("22");
}

void sendHumidity(SoftwareSerial *bluetoothModule) {
  bluetoothModule->println("33");
}
