//IPCAM: Build an single shoot remote camera with GPO control and no-ip client with a Arduino UNO, a Adafruit serial JPG camera and a Ethernet shield.
//Thanks to Starting Electronics and the Arduino Ethernet Shield HTML and web service tutorial: http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/
//Thanks to Adafruit for serial JPG camera: https://learn.adafruit.com/ttl-serial-camera/overview and Adafruit Arduino library
//Thanks to doughboy from arduino forum: http://forum.arduino.cc/index.php?topic=95456.15;wap2 for no-ip client code

// To run IPCAM you need connect 
// To tun IPCAM you need to insert a µsdcard in the ethernet shield with a FAT16/32 volume and file at root index.htm


#include <Adafruit_VC0706.h> // from Adafruit, library must be included in your library directory
#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>

#include <SoftwareSerial.h>         

#define chipSelect 4
#define REQ_BUF_SZ   40
#if ARDUINO >= 100
// On Uno: camera TX connected to pin 2, camera RX to pin 3:
SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
// On Mega: camera TX connected to pin 69 (A15), camera RX to pin 3:
//SoftwareSerial cameraconnection = SoftwareSerial(69, 3);
#else
NewSoftSerial cameraconnection = NewSoftSerial(2, 3);
#endif

Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x45, 0x99 }; // Ethernet shield MAC adress
byte ip[] = {192, 168, 0, 20}; // IP address, may need to change depending on network
byte dn[] = { 89, 2, 0, 1}; // DNS from our ISP
EthernetClient noip;  // for no-ip client
EthernetServer server(80);  // create a server at port 80
File webFile;  //to open and send html file from sdcard to internet
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
boolean LED_state[4] = {0}; // stores the states of the LEDs
long timer;  // for main loop no-ip client refresh

void setup() {

  // When using hardware SPI, the SS pin MUST be set to an
  // output (even if not connected or used).  If left as a
  // floating input w/SPI on, this can cause lockuppage.
#if !defined(SOFTWARE_SPI)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if(chipSelect != 53) pinMode(53, OUTPUT); // SS on Mega
#else
  if(chipSelect != 10) pinMode(10, OUTPUT); // SS on Uno, etc.
#endif
#endif

   timer = 0;
   SD.begin(4);  // establish SPI link with eth shield µsdcard
   cam.begin();  // Adafruit cam initialisation 
   Ethernet.begin(mac, ip, dn);  // initialize Ethernet device
   server.begin();           // start to listen for clients
   pinMode(6, OUTPUT);  // GPO 
   pinMode(7, OUTPUT);
   pinMode(8, OUTPUT);
   pinMode(9, OUTPUT);
}

void loop() {
  
  timer = timer + 1;
  //  ***** Here we have a short sequence for no-ip refresh, this sequence is runing each time timer reach 500000, so every 90 sec approx
    if (timer > 500000) {
      timer = 0;
      if (noip.connect("dynupdate.no-ip.com", 80)) {
        noip.println(F("GET /nic/update?hostname=account.no-ip.biz HTTP/1.0"));  // here you put your no-ip account
        noip.println(F("Host: dynupdate.no-ip.com"));
        //encode your username:password (make sure colon is between username and password)
        //to base64 at http://www.opinionatedgeek.com/dotnet/tools/base64encode/
        //and replace the string below after Basic with your encoded string
        //clear text username:password is not accepted
        noip.println(F("Authorization: Basic hereyourencodedusername:password="));   // here you send your log and pass
        noip.println(F("User-Agent: Arduino Sketch/1.0 user@host.com"));
        noip.println();
        } 
    }
    // if the server's disconnected, stop the client:
    if (!noip.connected()) {
        noip.stop();
    }
  
  EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // print HTTP request character to serial monitor
                //Serial.print(c);
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // open requested web page file
                    client.println(F("HTTP/1.1 200 OK"));
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println(F("Content-Type: text/xml"));
                        client.println(F("Connection: keep-alive"));
                        client.println();
                        XML_response(client);
                        SetLEDs();
                        // send XML file containing input states
                        
                    }
                    if (StrContains(HTTP_req, "GET / ") || StrContains(HTTP_req, "GET /index.htm")) {
                        //client.println(F("HTTP/1.1 200 OK"));
                        client.println(F("Content-Type: text/html"));
                        client.println(F("Connnection: keep-alive"));
                        client.println();
                        webFile = SD.open("index.htm");        // open web page file
                        //if (webFile) {
                        while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                        }
                        webFile.close();
                    //}
                    }
                    
                    /*else*/if (StrContains(HTTP_req, "GET /pic.jpg")) {
                      
                      cam.takePicture();
                      //client.println(F("HTTP/1.1 200 OK"));
                      client.println();
                      uint16_t jpglen = cam.frameLength();
                      pinMode(8, OUTPUT);
                      while (jpglen > 0) {
                      uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
                      client.write(cam.readPicture(bytesToRead), bytesToRead);
                      jpglen -= bytesToRead;
                    }
                      cam.resumeVideo();
                    }
                    
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found

void SetLEDs(void)
{
    
    // LED 1 (pin 6)
    if (StrContains(HTTP_req, "LED1=1")) {
        LED_state[0] = 1;  // save LED state
        digitalWrite(6, HIGH);
    }
    else if (StrContains(HTTP_req, "LED1=0")) {
        LED_state[0] = 0;  // save LED state
        digitalWrite(6, LOW);
    }
    // LED 2 (pin 7)
    if (StrContains(HTTP_req, "LED2=1")) {
        LED_state[1] = 1;  // save LED state
        digitalWrite(7, HIGH);
    }
    else if (StrContains(HTTP_req, "LED2=0")) {
        LED_state[1] = 0;  // save LED state
        digitalWrite(7, LOW);
    }  
    // LED 3 (pin 8)
    if (StrContains(HTTP_req, "LED3=1")) {
        LED_state[2] = 1;  // save LED state
        digitalWrite(8, HIGH);
    }
    else if (StrContains(HTTP_req, "LED3=0")) {
        LED_state[2] = 0;  // save LED state
        digitalWrite(8, LOW);
    }
    // LED 4 (pin 9)
    if (StrContains(HTTP_req, "LED4=1")) {
        LED_state[3] = 1;  // save LED state
        digitalWrite(9, HIGH);
    }
    else if (StrContains(HTTP_req, "LED4=0")) {
        LED_state[3] = 0;  // save LED state
        digitalWrite(9, LOW);
    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    int analog_val;            // stores value read from analog inputs
  
    
    
    cl.print(F("<?xml version = \"1.0\" ?>"));
    cl.print(F("<inputs>"));
    // checkbox LED states
    // LED1
    // read analog inputs
    analog_val = analogRead(2);
        cl.print(F("<analog>"));
        cl.print(analog_val);
        cl.println(F("</analog>"));
        analog_val = analogRead(3);
        cl.print(F("<analog>"));
        cl.print(analog_val);
        cl.println(F("</analog>"));
    // button LED states
    // LED1
    cl.print(F("<LED>"));
    if (LED_state[0]) {
        cl.print(F("on"));
    }
    else {
        cl.print(F("off"));
    }
    cl.println(F("</LED>"));
    // LED2
    cl.print(F("<LED>"));
    if (LED_state[1]) {
        cl.print(F("on"));
    }
    else {
        cl.print(F("off"));
    }
    cl.println(F("</LED>"));
    // LED3
    cl.print(F("<LED>"));
    if (LED_state[2]) {
        cl.print(F("on"));
    }
    else {
        cl.print(F("off"));
    }
    cl.println(F("</LED>"));
    // LED4
    cl.print(F("<LED>"));
    if (LED_state[3]) {
        cl.print(F("on"));
    }
    else {
        cl.print(F("off"));
    }
    cl.println(F("</LED>"));
    
    cl.print(F("</inputs>"));
}

char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
