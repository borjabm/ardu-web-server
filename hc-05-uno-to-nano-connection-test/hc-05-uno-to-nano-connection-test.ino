#include <SoftwareSerial.h> 
#define rx 3
#define tx 2

SoftwareSerial hc05Module(rx, tx); // HC-05 RX -> Digital2 | HC-05 TX -> Digital3 
int flag = 0; 
void setup() 
{   
 Serial.begin(9600); 
 hc05Module.begin(9600); 
 Serial.println("***Connection Test - I am UNO***"); 
 pinMode(tx, OUTPUT);
 pinMode(rx, INPUT);
} 
bool waitResponse = false;
char request_buffer[20] = {0};
int i = 0;
void loop()
{
  if (!waitResponse) {
    hc05Module.println("getTemperature");
    waitResponse = true;
  } 
  Serial.println("Waiting");
  /**if(hc05Module.available()>0)
  {
    Serial.println("Avail!");
    Serial.println(hc05Module.read());
  }**/
  
  while (hc05Module.available() > 0)
    {
        char c = hc05Module.read();
        Serial.println(c);
        if (i < 20){
          request_buffer[i] = c;
        }
        i++;

        if (c == '\n')
        {
         
          
            Serial.println("Uno Received: ");
            Serial.println(request_buffer);
            i=0;
            memset(request_buffer, 0, 20); // Clear recieved buffer
            waitResponse = false;
        }
        }
  delay(4000);
}
