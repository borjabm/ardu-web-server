#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"

#define DHTPIN 7     
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);
EthernetServer server(80);

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
}

float t;
float h;
void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  
  
  h = dht.readHumidity();
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.println(F("% "));
  t = dht.readTemperature();
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    char request_buffer[200] = {0};
    int i = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (i <200){
          request_buffer[i] = c;
        }
        i++;
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          char rqMethod[10] = {0};
          char rqPath[20] = {0};
          char GET[] = "GET";
          char HOME_PATH[] = "/";
          char GET_TEMPERATURE[] = "/getTemperature";
          char GET_HUMIDITY[] = "/getHumidity";
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
    delay(1);
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
  boolean endOfChunk = false;
  int idx = shift;
  while (!endOfChunk){
    if (idx >= maxSize || sourceArray[idx] == '\0' || sourceArray[idx] == endChar ) {
      endOfChunk = true;
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

// HTTP RS Header https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html#sec6
char okHeader[] = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Connection: close\r\n" // the connection will be closed after completion of the response
                  "Refresh: 5\r\n";  // refresh the page automatically every 5 sec

void sendTemp(EthernetClient *client) {
  // float t = dht.readTemperature();
  // send a standard http response header
  sendHeader(client, okHeader);
  // Maximum temp is 100.00. We need a maximum of 6 chars, with 2 decimal positions.
  char temp[13]={0};
  dtostrf(t, 6, 2, temp);
  temp[6] = '&';
  temp[7] = 'd';
  temp[8] = 'e';
  temp[9] = 'g';
  temp[10] = ';';
  temp[11] = 'C';
  sendBody(client, temp);
}

void sendHumidity(EthernetClient *client) {
  // float h = dht.readHumidity();
  // send a standard http response header
  sendHeader(client, okHeader);
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
  
  sendBody(client, humidity);
}

char mainBody[] =  "<!DOCTYPE html>\r\n"
                     "<html>\r\n"
                     "<body>\r\n"
                     "<h1>Home Weather Station</h1>\r\n"
                     "<p>Temperature is -- C </p>\r\n"
                     "</body>\r\n"
                     "</html>\r\n";
void sendHome(EthernetClient *client) {
  // send a standard http response header
  sendHeader(client, okHeader);
  sendBody(client, mainBody);
}

char notFoundHeader[] = "HTTP/1.1 404 NotFound\r\n"
                          "Content-Type: text/html\r\n" 
                          "Connection: close\r\n"; // the connection will be closed after completion of the response

char notFoundBody[] =  "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "<body>\r\n"
                         "<h1>Oops!</h1>"
                         "<h2>404 Not Found</h2>"
                         "<div> Sorry, requested resource not found! </div>"
                         "<div>"
                         "<a href=\"/\">Take Me Home </a>"
                         "</div>";

void sendNotFound(EthernetClient *client) {
  // send Not Found a standard http response header
  sendHeader(client, notFoundHeader);  
  sendBody(client, notFoundBody);
}

void sendHeader (EthernetClient *client, char *header) {
  client->println(header);
}

void sendBody (EthernetClient *client, char *body) {
  client->println(body);
}
