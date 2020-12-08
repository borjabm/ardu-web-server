#include <SoftwareSerial.h> 
SoftwareSerial hc05Module(3, 2); // HC-05 TX -> Digital3 | HC-05 RX -> Digital2
int flag = 0; 
void setup() 
{   
 Serial.begin(38400); 
 hc05Module.begin(38400); 
 pinMode(LED_BUILTIN, OUTPUT); 
 Serial.println("***AT commands mode***"); 
} 
void loop() 
{ 
 if (hc05Module.available()) Serial.write(hc05Module.read());

 if (Serial.available()) hc05Module.write(Serial.read());
   //flag = hc05Module.read(); 
  /** Set to AT MODE **/
  //1. Upload sketch
  //2. Remove power pin from HC-05 module
  //3. Hold reset button in HC-05 module and connect power pin to it.
  //4. Release reset button.
  //5. Open Serial monitor, set baudrate to 38400 baud and Both NL&CR
  //6. Send AT commands
  /** UNO - Master **/
  // AT+ROLE? 
  // AT+ROLE=1    // 1 - Master
  // AT+CMODE?
  // AT+CMODE=1 // 1 - connect any address
  // AT+NAME?
  // AT+NAME=HC-05-UNO
  // AT+ADDR? // 98D3:61:F5F0EE  -> Mac address to which the slave module will connect

  /** Nano - Slave **/
  // AT+ROLE? 
  // AT+ROLE=0    // 0 - Slave
  // AT+CMODE?
  // AT+CMODE=0 // 0 - connect fixed address
  // AT+NAME?
  // AT+NAME=HC-05-NANO
  // AT+ADDR? // 21:13:17EB
  // AT+BIND=98D3,61,F5F0EE
  // AT+BINDR? // 98D3:61:F5F0EE  -> It should ne the Mac address of the master
 /**if (flag == 1) 
 { 
   digitalWrite(LED_BUILTIN, HIGH);  
   Serial.println("LED On"); 
 } 
 else if (flag == 0) 
 { 
   digitalWrite(LED_BUILTIN, LOW); 
   Serial.println("LED Off"); 
   
 } 
 delay(1000);**/
}  
