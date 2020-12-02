#include <avr/pgmspace.h>
#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"

#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

#define DHTPIN 7     
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiAvrI2c oled;

/** Oled specific fields to help move cursor around **/
int valueFieldWidth = 0;
int firstRowX0 = 0;  // First value column
int secondRowX0 = 0;  // First value column
int rHeight = 0;      // Rows per line.


byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);
EthernetServer server(80);

// HTTP RS Header https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html#sec6
const char okHeader[] PROGMEM = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Connection: close\r\n"; // the connection will be closed after completion of the response
                  
const char okHeaderRefresh[] PROGMEM = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Connection: close\r\n" // the connection will be closed after completion of the response
                  "Refresh: 5\r\n";  // refresh the page automatically every 5 sec
const char notFoundHeader[] PROGMEM = "HTTP/1.1 404 NotFound\r\n"
                          "Content-Type: text/html\r\n" 
                          "Connection: close\r\n"; // the connection will be closed after completion of the response
const char okBody[] PROGMEM = "<!DOCTYPE html>\r\n"  
                              "<html>\r\n"
                              "  <body>\r\n"
                              "    <h1>Home Weather Station</h1>\r\n"
                              "    <div>\r\n"
                              "     <div style=\"display: inline-block\">\r\n"
                              "        <p>Temperature is </p>\r\n"
                              "      </div>\r\n"
                              "      <div id=\"temperature\" style=\"display: inline-block\">\r\n"
                              "        <p>--</p>\r\n"
                              "      </div>\r\n"
                              "      <div style=\"display: inline-block\">\r\n"
                              "        <p> &deg;C</p>\r\n"
                              "      </div>\r\n"
                              "    </div>\r\n"
                              "    <div>\r\n"
                              "      <div style=\"display: inline-block\">\r\n"
                              "        <p>Humidity is </p>\r\n"
                              "      </div>\r\n"
                              "      <div id=\"humidity\" style=\"display: inline-block\">\r\n"
                              "        <p>--</p>\r\n"
                              "      </div>\r\n"
                              "      <div style=\"display: inline-block\">\r\n"
                              "        <p> &percnt;</p>\r\n"
                              "      </div>\r\n"
                              "    </div>\r\n"
                              "<script>\r\n"
                              "function getTemperature() {\r\n"
                              "  var xhttp = new XMLHttpRequest();\r\n"
                              "  xhttp.onreadystatechange = function() {\r\n"
                              "    if (this.readyState == 4 && this.status == 200) {\r\n"
                              "      document.getElementById(\"temperature\").innerHTML = this.responseText;\r\n"
                              "    }\r\n"
                              "  };\r\n"
                              "  xhttp.open(\"GET\", \"getTemperature\", true);\r\n"
                              "  xhttp.send();\r\n"
                              "}\r\n"
                              "function getHumidity() {\r\n"
                              "  var xhttp = new XMLHttpRequest();\r\n"
                              "  xhttp.onreadystatechange = function() {\r\n"
                              "    if (this.readyState == 4 && this.status == 200) {\r\n"
                              "      document.getElementById(\"humidity\").innerHTML = this.responseText;\r\n"
                              "    }\r\n"
                              "  };\r\n"
                              "  xhttp.open(\"GET\", \"getHumidity\", true);\r\n"
                              "  xhttp.send();\r\n"
                              "}\r\n"
                              "setInterval(function(){getTemperature()}, 5000);\r\n"
                              "setInterval(function(){getHumidity()}, 5000);\r\n"
                              "</script>\r\n"
                              "  </body>\r\n"
                              "</html>\r\n";                  
const char notFoundBody[] PROGMEM =  "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "<body>\r\n"
                         "<h1>Oops!</h1>\r\n"
                         "<h2>404 Not Found</h2>\r\n"
                         "<div> Sorry, requested resource not found! </div>\r\n"
                         "<div>\r\n"
                         "<a href=\"/\">Take Me Home </a>\r\n"
                         "</div>\r\n";

const char *const string_table[] PROGMEM = {okHeader, okHeaderRefresh, notFoundHeader, okBody, notFoundBody};

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Ethernet WebServer Example");
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  dht.begin();

  setUpOled();
}

static const char GET[] = "GET";
static const char HOME_PATH[] = "/";
static const char GET_TEMPERATURE[] = "/getTemperature";
static const char GET_HUMIDITY[] = "/getHumidity";

char rqMethod[10] = {0};
char rqPath[20] = {0};

char request_buffer[200] = {0};

float t;
float h;
void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  
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
  

  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("new client"));
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    int i = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (i < 200){
          request_buffer[i] = c;
        }
        i++;
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          parseRequest(request_buffer, sizeof(request_buffer), rqMethod, 10, rqPath, 20); 
          Serial.println(rqPath);
          if (strcmp(rqPath, HOME_PATH) == 0) {
            if (strcmp(rqMethod, GET) == 0) {
              sendHome(&client);
              break;
            }
          } 
          if (strcmp(rqPath, GET_TEMPERATURE) == 0) {
            if (strcmp(rqMethod, GET) == 0) {
              sendTemp(&client);
              break;
            }
          } 
          if (strcmp(rqPath, GET_HUMIDITY) == 0) {
            if (strcmp(rqMethod, GET) == 0) {
              sendHumidity(&client);
              break;
            }
          } 
          sendNotFound(&client);
          break; 
          
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(2);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void parseRequest(char *request_buffer, int maxBufferSize, char *rqMethod, int maxRqMethodSize, char *rqPath, int maxRqPathSize) {
  /**
  * We know a HTTP RQ looks like: https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html
    GET / HTTP/1.1\r\n
    Host: 192.168.0.177\r\n
    Connection: keep-alive\r\n
    Cache-Control: max-age=0\r\n
    Upgrade-Insecure-Requests: 1\r\n
    User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.83 Safari/537.36\r\n
    Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng, /*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n
    Accept-Encoding: gzip, deflate\r\n
    Accept-Language: en-GB,en;q=0.9,nb-NO;q=0.8,nb;q=0.7,es-ES;q=0.6,es;q=0.5,en-US;q=0.4,pt;q=0.3\r\n
    \r\n
    It ends with an empty line
  */
  
  // we get first line of the RQ
  char rqMethodLine[100] = {0};
  int rqMethodLineIdx = getChunkAndPutItInArray(rqMethodLine, request_buffer, '\n', 0, maxBufferSize);
 
  // we get RQ method
  int rqMethodIdx = getChunkAndPutItInArray(rqMethod, rqMethodLine, ' ', 0, sizeof(rqMethodLine));
 
  // we get RQ path
  int rqPathIdx = getChunkAndPutItInArray(rqPath, rqMethodLine, ' ', rqMethodIdx, sizeof(rqMethodLine));

  // we get RQ HTTP Version
  // char rqHttpVersion[10] = {0};
  // int rqHttpVersionIdx = getChunkAndPutItInArray(rqHttpVersion, rqMethodLine, '\n', rqMethodIdx+rqPathIdx, sizeof(rqMethodLine));
}

int getChunkAndPutItInArray(char *destinationArray, char *sourceArray, char endChar, int shift, int maxSize) {
  int chunkIdx = 0;
  int idx = shift;
  while (true){
    if (idx >= maxSize || sourceArray[idx] == '\0' || sourceArray[idx] == endChar ) {
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



void sendTemp(EthernetClient *client) {
  // float t = dht.readTemperature();
  // send a standard http response header 
  sendOkHeader(client);
  // Maximum temp is 100.00. We need a maximum of 6 chars, with 2 decimal positions.
  char temp[13]={0};
  dtostrf(t, 6, 2, temp);
  temp[6] = '&';
  temp[7] = 'd';
  temp[8] = 'e';
  temp[9] = 'g';
  temp[10] = ';';
  temp[11] = 'C';
  sendDataArrayCharByChar(client, temp);
}

void sendHumidity(EthernetClient *client) {
  // float h = dht.readHumidity();
  // send a standard http response header
  sendOkHeader(client);
  // Maximum humidity is 100.00. We need a maximum of 6 chars, with 2 decimal positions.
  char humidity[15]={0};
  dtostrf(h, 6, 2, humidity);
  humidity[6] = '&';
  humidity[7] = 'p';
  humidity[8] = 'e';
  humidity[9] = 'r';
  humidity[10] = 'c';
  humidity[11] = 'n';
  humidity[12] = 't';
  humidity[13] = ';';
  sendDataArrayCharByChar(client, humidity);
}


void sendHome(EthernetClient *client) {
  // send a standard http response header
  sendOkHeaderNoRefresh(client);
  sendHomeBody(client);
}


void sendNotFound(EthernetClient *client) {
  // send Not Found a standard http response header
  sendNotFoundHeader(client);
  sendNotFoundBody(client);
}

void sendNotFoundHeader(EthernetClient *client) {
  sendData_P(client, 2);
}

void sendNotFoundBody(EthernetClient *client) {
  sendData_P(client, 4);
}

void sendHomeBody(EthernetClient *client) {
  sendData_P(client, 3);
}

void sendOkHeaderNoRefresh(EthernetClient *client) {
  sendData_P(client, 0);
}

void sendOkHeader(EthernetClient *client) {
  sendData_P(client, 1);
}

void sendDataArray(EthernetClient *client, char *src) {
  client->println(src);
}

void sendDataArrayCharByChar(EthernetClient *client, char *src) {
  int idx = 0;
  do{
    client->print(src[idx]);
    idx++;
  } while(src[idx] != '\0');
  client->print("\r\n");
}

void sendData_P(EthernetClient *client, int index) {
  unsigned int flash_address = pgm_read_word(&string_table[index]);
  char c = 0;
  while (true){
    c = (char) pgm_read_byte(flash_address);
    if (c == '\0') {
      break;
    } 
    client->print(c);
    Serial.print(c);
    flash_address++;
  }
  client->print("\r\n");
  Serial.print("\r\n");
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
