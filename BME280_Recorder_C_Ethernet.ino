
/*
 * This version uses an Ethernet/SD card and protobaord with pin D10 remapped to pin D3 and D4 to D2 
 * See LiquidCrystal remapping LiquidCrystal lcd(8, 9, 2, 5, 6, 7);
 * 
 References and many thanks to:
* http://arduino.cc/en/Tutorial/LiquidCrystal
* 
* Single line scroll from Nishant Arora-https://goo.gl/qPGpWB
* 
* Writing SD card data- See toptechboy.com-http://goo.gl/NXFAz0
* 
* Real Time Clock from:
    * Andrew Basterfield, AlarmClock, https://goo.gl/Heigw1
    * Sparkeys Widgets, www.sparkyswidgets.com, https://goo.gl/BUaKya
    * 
* Neil Kenyon for WebServerDataLogger https://goo.gl/GQ3aYO
* 
* Tom Igoe for Ethernet.maintain and IP Print ideas https://github.com/arduino/Arduino/pull/3761/files
* 
* Simon Monk for the timer.h library http://goo.gl/G1XpT, github-https://goo.gl/HPNM38
* 
* DD-WRT wiki for client bridge configuration - https://goo.gl/uWVG16
* 
* And, Cactus.io for Multiple BME280 libraries and plans-https://goo.gl/yWBL5G
* 
* And, random web postings that have helped me figure out how to do this.
*/

// include the library's
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>    // imports the wire library for talking over I2C 
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Timer.h>       //Simon Monk's timer Function Library 
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include "ThingSpeak.h"   //Thingspeak
#include "cactus_io_BME280_I2C.h"


//***************************************************************
//*********************** BME280 SETTINGS ***********************
//***************************************************************
// Create two BME280 instances 
BME280_I2C bme1(0x77); // I2C using address 0x77 
BME280_I2C bme2(0x76); // I2C using address 0x76 

// init the lcd display according to the circuit
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);   //bread board
//LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7);  //sainsmart
//LCD Shield (16x2) Pin are remapped to not conflict with the ethernet sheild. Ethernet shield
//needs pins 10 & 4. Pin 10 becomes Pin 3 for PWM backlight brightness. LCD-Pin4(DB4) remaps to Pin2
LiquidCrystal lcd(8, 9, 2, 5, 6, 7);      // LCD pins for use with Ethernet-SD Shield


int chipSelect = 4;     //SD card pin
int screenWidth = 16;   //LCD Dsiplay
int screenHeight = 2;   //LCD Diaplay
int stringStart, stringStop = 0; //Initialize Scrolling Letter Counter
int scrollCursor = screenWidth;  //Initialize Scrolling Letter variable
int counter = 0;
int avgCounterBME1 = 1;
int avgCounterBME2 = 1;
//int timerh = 2600;


float temperatureBME1 = 0.0;
float temperatureBME2 = 0.0;
float humidityBME1 = 0.0;     //Initialize Temp and Hum as floating point numbers for the demo w/o sensor
float humidityBME2 = 0.0;
float avgTempBME1 = 0.0;
float avgTempBME2 = 0.0;
float tempValuesBME1 = 0.0;
float tempValuesBME2 = 0.0;
float avgHumidBME1 = 0.0;
float avgHumidBME2 = 0.0;
float humidValuesBME1 = 0.0;
float humidvaluesBME2 = 0.0;
float baropressBME1 = 0.0;
float bpValuesBME1 = 0.0;
float avgBpBME1 = 0.0;

char timeDateComma[20];
char timeDateSemiColon[20];
char timeStamp[8];     //thingspeak
char dateStamp[8];     //thingspeak
char charArray[34];    // Dimension this to be size of largest data item + 1 Web write string

//**********************************************************************
//*********************** ThinkSpeak SETTINGS **************************
//**********************************************************************
//unsigned long myChannelNumber = 76895;    //thingspeak Arduino 2*****Arduino 2***** (MGCQuonset.com)
//const char * myWriteAPIKey = "5G3FVHACYQU2K2IT";  //thingspeak Arduino 2*****Arduino 2*****

//unsigned long myChannelNumber = 79378;    //thingspeak Arduino 1 *****ASUS WL-Arduino (myraclegarden.com)
//const char * myWriteAPIKey = "U0RIIBW1U68RE6B9";  //thingspeak Arduino 1*****ASUS WL-Arduino 1 new*****

unsigned long myChannelNumber = 77784;    //thingspeak
const char * myWriteAPIKey = "PGIXBEEVDHXX2P7E";  //

//**********************************************************************
//***********************End ThinkSpeak SETTINGS **************************
//**********************************************************************

boolean endOfLine = false;      // Flag for end of line of data for web write string

String line1 = "BME280-1 ";     //Text for scrolling lcd line one-space required at end of string to clear the screen
String line2 = "Ambient T:";    //Text for scrolling lcd line one
String line3 = "F RH:";         //Text for scrolling lcd line one
String line4 = "% BP:";            //Text for scrolling lcd line one
String line5 = "in Hg ";
String lineP;

Timer t;  // Start simon monk's timer function 

File bme280Sensors;     //Data object for sensor data 
File sdCardWrite;
File myFile;
Sd2Card card;
SdVolume volume;
SdFile root;

//***************************************************************
//***************** MAC ADDRRESS SETTINGS ***********************
//***************************************************************

//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBA };  // MAC Address Arduino #1 & ASUS WL (myraclegarden.com)

byte mac[] = { 0xBA, 0xBA, 0xBE, 0xEF, 0xBA, 0xBA };  // MAC Address Arduino #2 & TP-LINK  (sites.google.com/site/mgcquonset/)

//***************************************************************
//*************** END MAC ADDRESS SETTINGS **********************
//***************************************************************
//
//
//**************************************************************
//*********************** IP SETTINGS **************************
//**************************************************************

//byte ip [] = {192, 168, 1, 55};  // local IP Address Arduino 1 Asus*****Arduino #1*****

byte ip[] = { 192, 168, 1, 50 };  //Local IP Address Arduino 2 TP-Link *****Arduino #2*****

//**************************************************************
//******************* END IP SETTINGS **************************  
//**************************************************************

EthernetServer server(80);        // HTTP port
EthernetClient myClient;          // Current Ethernet client

//**************************************************************
//**********************VOID SETUP******************************
//**************************************************************
void setup() 
{
greeting();     //Run Greeting function: Clear LCD at start up and check IP address

//*******************************************************************
//****************Initialize SD reset Hardware SS********************
//*******************************************************************
  
pinMode(10,OUTPUT);             
digitalWrite(10, HIGH);
SD.begin(chipSelect);

//*******************************************************************
//****Remove data from SD Card Data File by deleting the file********
//*******************************************************************

//SD.remove("HC_DATA.TXT");       //uncomment to delete HC_Data.TXT

//*******************************************************************
//*******************Set up Ethernet connection**********************
//*******************************************************************
//
   Ethernet.begin( mac, ip );      // open the Ethernet connection*****Arduino 1 & 2 with verizon router*****
   
   //Ethernet.begin(mac);          // open the Ethernet connection DHCP
   
//*******************************************************************
//*******************Set up Ethernet connection**********************
//*******************************************************************

   server.begin();                 // start the Ethernet server
   
//*******************************************************************
//***********************Start ThingSpeak****************************
//*******************************************************************   
   ThingSpeak.begin(myClient);     //Thingspeak

//*******************************************************************
//***************Run Function BME280 RecorderBoot********************
//*******************************************************************

   bme280RecorderBoot();     //Run Function to set up SD card, Serial Port and LCD
   
//*******************************************************************
//*********************Set-up Timer Functions************************
//*******************************************************************

   t.every(400, moveLetters);       //Move Characters on LCD Display one block every 400 ms-optimal readability
   t.every(2600, takeReadingBME1);  //Take bme280 Measurement and Display on LCD Line 1 every 2.6 seconds
   t.every(3100, takeReadingBME2);  //Take bme280 Measurement and Display on LCD Line 2 every 3.1 seconds
   t.every(63000, writeThingSpeak); //Thingspeak- Write to ThingSpeak every 63 seconds


   
//*******************************************************************
//*********************Sensor Temp Correction************************
//*******************************************************************

bme1.setTempCal(-3);     // Temp was reading high - subtract 3 degrees 
bme2.setTempCal(-3);   // Temp was reading high - subtract 3 degrees

//*******************************************************************
//*****************End Sensor Temp Correction************************
//*******************************************************************


   
}

//**************************************************************
//**********************END VOID SETUP**************************
//**************************************************************


//**************************************************************
//************************VOID LOOP*****************************
//**************************************************************

void loop()
{
    t.update();     //timer function
      
    listenForClient();    //check for HTTP webclient request
}

//**************************************************************
//************************END VOID LOOP*************************
//**************************************************************

//**************************************************************
//*********Function to move Characters on LCD*******************
//**************************************************************

void moveLetters()     //Function that moves letters across the LCD display
                       //Nishant Arora-https://goo.gl/wppldI
{ 
  //Comment out next two lines of code to remove time and date from lcd line 1
      sprintf(timeDateComma, "%2d:%02d:%02d %02d/%02d ", hour(), minute(), second(), month(), day());
      line1 = (timeDateComma);
  
      lineP = line1 + line2 + temperatureBME1 + char(223) + line3 + humidityBME1 + line4 + baropressBME1 + line5;
      lcd.setCursor(scrollCursor, 0);
      lcd.print(lineP.substring(stringStart,stringStop));
            
      if(stringStart == 0 && scrollCursor > 0)
      {
      scrollCursor--;
      stringStop++;
      } 
      else if (stringStart == stringStop)
      {
      stringStart = stringStop = 0;
      scrollCursor = screenWidth;
      } 
      else if (stringStop == lineP.length() && scrollCursor == 0) 
      {
      stringStart++;
      } 
      else 
      {
      stringStart++;
      stringStop++;
      }       
}
//**************************************************************
//*****END Function to move Characters on LCD*******************
//**************************************************************

//**************************************************************
//****Function to Read the BME1 Sensor and report to LCD****
//**************************************************************
          
void takeReadingBME1()
{
  if (avgCounterBME1 == 24)
  
     {
      //Read BME280-1 Sensor
      bme1.readSensor(); //Read BME280 
      
      //Get BME280-1 Temperature, Humidity and Barometric Pressure from sensor
     temperatureBME1 = bme1.getTemperature_F();
     humidityBME1 = bme1.getHumidity(); 
     baropressBME1 = (bme1.getPressure_MB() * 0.0295301);
     
     
     tempValuesBME1 = tempValuesBME1 + temperatureBME1;
     humidValuesBME1 = humidValuesBME1 + humidityBME1;
     bpValuesBME1 = bpValuesBME1 + baropressBME1;
     
    
     Serial.print("tempValues BME1= ");
     Serial.print(tempValuesBME1);
     Serial.print(" humidValues BME1= ");
     Serial.print(humidValuesBME1);
     Serial.print(" avgCounter BME1= ");
     Serial.print(avgCounterBME1);
     Serial.print(" temp BME1= ");
     Serial.print(temperatureBME1);
     Serial.print(" humidity BME1= ");
     Serial.println(humidityBME1);

     avgTempBME1 = tempValuesBME1 / (avgCounterBME1);
     avgHumidBME1 = humidValuesBME1 / (avgCounterBME1); 
     avgBpBME1 = bpValuesBME1 / (avgCounterBME1);
     
     Serial.print("**Average Temperature BME1= ");
     Serial.print(avgTempBME1);
     Serial.print(" Average Humidity BME1= ");
     Serial.print(avgHumidBME1);
     Serial.print(" avgCounterBME1= ");
     Serial.print(avgCounterBME1);
     Serial.print(" temp BME1= ");
     Serial.print(temperatureBME1);
     Serial.print(" humidity BME1= ");
     Serial.println(humidityBME1);
     
     //serialSDprintBME1();   
     //serialSDprintBME2();
     
     tempValuesBME1 = 0.0;
     humidValuesBME1 = 0.0;
     bpValuesBME1 = 0.0;
     avgCounterBME1 = 1;
     }
else
     {
     //Read BME280-1
     bme1.readSensor();
      
     //Get BME280-1 Temp and Humidity from sensor
     humidityBME1 = bme1.getHumidity();
     temperatureBME1 = bme1.getTemperature_F();
     baropressBME1 = (bme1.getPressure_MB() * 0.02953);

     //Accumulate for average calculation
     tempValuesBME1=tempValuesBME1 + temperatureBME1;
     humidValuesBME1=humidValuesBME1 + humidityBME1;
     bpValuesBME1 = bpValuesBME1 + baropressBME1;
    
     Serial.print("tempValues BME1= ");
     Serial.print(tempValuesBME1);
     Serial.print(" humidValues BME1= ");
     Serial.print(humidValuesBME1);
     Serial.print(" avgCounterBME1= ");
     Serial.print(avgCounterBME1);
     Serial.print(" temp BME1= ");
     Serial.print(temperatureBME1);
     Serial.print(" humidity BME1= ");
     Serial.println(humidityBME1);
     
     avgCounterBME1 = avgCounterBME1 + 1;
     }
}
//**************************************************************
//**END Function to Read the BME1 Sensor and report to LCD**
//**************************************************************

//**************************************************************
//**** Function to Read the BME2 Sensor and report to LCD***
//**************************************************************
    
void takeReadingBME2()
{ 
   if (avgCounterBME2 == 20)   //if the average counter equals 20 (last one for average) execute code below
     {
   bme2.readSensor();      //Read BME280 #2
     humidityBME2 = bme2.getHumidity();
     temperatureBME2 = bme2.getTemperature_F();
    
     lcd.setCursor(0, 1);               //Display readings on LCD (position 0, line 2)
     lcd.print("T:"); 
     lcd.print(temperatureBME2);        //Display BME2 Temperature
     lcd.print((char)223);              //Degree symbol
     lcd.print("F ");
     lcd.print("H:");
     lcd.print(humidityBME2);           //Display BME2 Humidity
     lcd.print("%");
     
     humidvaluesBME2=humidvaluesBME2 + humidityBME2;  // Sum accumulation for average calculation
     tempValuesBME2=tempValuesBME2 + temperatureBME2; // Sum accumulation for average calculation
    
     Serial.print("  tempValues BME2= "); //Display on Serial Monitor
     Serial.print(tempValuesBME2);
     Serial.print(" humidValues BME2= ");
     Serial.print(humidvaluesBME2);
     Serial.print(" avgCounterBME2= ");
     Serial.print(avgCounterBME2);
     Serial.print(" temp BME2= ");
     Serial.print(temperatureBME2);
     Serial.print(" humidity BME2= ");
     Serial.println(humidityBME2);

      
     avgHumidBME2 = humidvaluesBME2 / (avgCounterBME2); // Compute Average Humidity
     avgTempBME2 = tempValuesBME2 / (avgCounterBME2);   // Compute Average Temperature
     
     Serial.print("**Average Temperature BME2= ");  // Display on Serial Monitor
     Serial.print(avgTempBME2);
     Serial.print(" Average Humidity BME2= ");
     Serial.print(avgHumidBME2);
     Serial.print(" avgCounterBME2= ");
     Serial.print(avgCounterBME2);
     Serial.print(" temp BME2= ");
     Serial.print(temperatureBME2);
     Serial.print(" humidity BME2= ");
     Serial.println(humidityBME2);
     
     serialSDprintBME1();   //Run Function to Write Data to Serial Monitor & SD card
     serialSDprintBME2();   //Run Function to Write Data to Serial Monitor & SD card
     
     tempValuesBME2 = 0.0;  //Zero out accumulator for next average
     humidvaluesBME2 = 0.0;  //Zero out accumulator for next average
     avgCounterBME2 = 1;    //Zero out accumulator for next average
     }
     else          // If Average counter for BME-2 is less than 20
     {  
     bme2.readSensor();      //Read BME280 #2 
     humidityBME2 = bme2.getHumidity();
     temperatureBME2 = bme2.getTemperature_F();
    
     lcd.setCursor(0, 1);               //Display readings on LCD (position 0, line 2)
     lcd.print("T:"); 
     lcd.print(temperatureBME2);
     lcd.print((char)223);              //Degree symbol
     lcd.print("F ");
     lcd.print("H:");
     lcd.print(humidityBME2);
     lcd.print("%");
     
     humidvaluesBME2=humidvaluesBME2 + humidityBME2;  // Sum accumulation for average calculation
     tempValuesBME2=tempValuesBME2 + temperatureBME2; // Sum accumulation for average calculation
    
     Serial.print("  tempValues BME2= ");  // Display on Serial Monitor
     Serial.print(tempValuesBME2);
     Serial.print(" humidValues BME2= ");
     Serial.print(humidvaluesBME2);
     Serial.print(" avgCounterBME2= ");
     Serial.print(avgCounterBME2);
     Serial.print(" temp BME2= ");
     Serial.print(temperatureBME2);
     Serial.print(" humidity BME2= ");
     Serial.println(humidityBME2);

      avgCounterBME2 = avgCounterBME2 + 1; //Increment the Average Counter
     }
}
//**************************************************************
//**END Function to Read the BME2 Sensor and report to LCD**
//**************************************************************



//*****************************************************************
//**Function to Write BME1 Averages to Serial Monitor and SD Card**
//*****************************************************************     
void serialSDprintBME1()
    {
     Serial.print("*BME1* ");
     sprintf(timeDateComma, "%2d:%02d:%02d, %02d/%02d/%4d, ", hour(), minute(), second(), month(), day(), year());
     Serial.print(timeDateComma);
     Serial.print(avgTempBME1);
     Serial.print(", ");
     Serial.println(avgHumidBME1); 
    
     writeSDdataBME1(); //Write data to the SD card-See functions below
     }
//**************************************************************
//*END Function to BME1 Write Averages to Serial Monitor and SD Card*
//**************************************************************

//**************************************************************
//********Function to Write BME1 Data to SD Card****************
//**************************************************************
void writeSDdataBME1()     
{
      bme280Sensors = SD.open("HC_Data.txt", FILE_WRITE);  //Write data to SD card  
      if(!bme280Sensors)
      {
      Serial.println("error opening SD card file");
      }
      else
      {
      Serial.println("writing BME1 data to SD card");
      sprintf(timeDateSemiColon, "%2d:%02d:%02d;%02d/%02d/%4d;", hour(), minute(), second(), month(), day(), year());
      bme280Sensors.print(timeDateSemiColon);
      bme280Sensors.print(avgTempBME1);        //write temperature 
      bme280Sensors.print(";");                //write a semicolon
      bme280Sensors.print(avgHumidBME1);           //write rh and end the line (println)
      bme280Sensors.print(";");
      }         
 } 
//**************************************************************
//*********END Function to Write BME1 Data to SD Card***********
//**************************************************************


//*****************************************************************
//**Function to Write BME2 Averages to Serial Monitor and SD Card**
//*****************************************************************      
void serialSDprintBME2()
    {
     Serial.print("*BME2* ");
     sprintf(timeDateComma, "%2d:%02d:%02d, %02d/%02d/%4d, ", hour(), minute(), second(), month(), day(), year());
     Serial.print(timeDateComma);
     Serial.print(avgTempBME2);
     Serial.print(", ");
     Serial.println(avgHumidBME2);
             
     writeSDdataBME2(); //Write data to the SD card-See functions below
     }
//*****************************************************************
//**END Function to Write BME2 Averages to Serial Monitor and SD Card**
//***************************************************************** 
     
//**************************************************************
//********Function to Write BME2 Data to SD Card****************
//**************************************************************
void writeSDdataBME2()     
      {
      Serial.println("writing BME2 data to SD Card");
      bme280Sensors.print(avgTempBME2);        //write temperature 
      bme280Sensors.print(";");                //write a semicolon
      bme280Sensors.print(avgHumidBME2);           //write rh and end the line (println)
      bme280Sensors.print(";");                //write a semicolon
      bme280Sensors.println("");               //
      bme280Sensors.close();                   //close the file         
      }
//**************************************************************
//*******END Function to Write BME1 Data to SD Card*************
//**************************************************************
       
//**************************************************************
//***************Function to Write ThingSpeak*******************
//**************************************************************
void writeThingSpeak()
    {
    Serial.println("writing BME1 & BME2 data to ThingSpeak");
    sprintf(timeStamp, "%2d:%02d:%02d", hour(), minute(), second());
    ThingSpeak.setField(1,timeStamp);
    sprintf(dateStamp, "%02d/%02d/%4d", month(), day(), year());
    ThingSpeak.setField(2,dateStamp);
    ThingSpeak.setField(3,avgTempBME1);
    ThingSpeak.setField(4,avgHumidBME1);
    ThingSpeak.setField(5,avgTempBME2);
    ThingSpeak.setField(6,avgHumidBME2);
    ThingSpeak.setField(7,avgBpBME1);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    }
//**************************************************************
//***********END Function to Write ThingSpeak*******************
//**************************************************************    
     
//**************************************************************
//****Function to connect with browsers and report SD Data******
//**************************************************************

void listenForClient()
{
  myClient = server.available();     // listen for incoming clients
  if (myClient)
  {
    while (myClient.connected())
    {
      if (myClient.available()) // read until no more characters - Chrome does 2 requests!
      {
        char c = myClient.read();
        Serial.write(c);
      }
      else
      {
        Serial.println("sending http response header and SD card contents");
        startHTTP();                   // send http response header
        fileToClient();                //Send contents of SD Card Text file to the client
        myClient.println("</html>");   // end of HTML
        break;
      }
    }
    delay(10);               // give the web browser time to receive the data
    myClient.stop();        // close the HTTP connection:
    Serial.println("client disconnected");
    Serial.println();
  }
}
//**************************************************************
//*********END Function to connect with web browser*************
//**************************************************************

//**************************************************************
//***************Function void startHTTP() *********************
//**************************************************************

void startHTTP() //Respond to the HTTP request and set some parameters
{
  myClient.println("HTTP/1.1 200 OK");
  myClient.println("Content-Type: text/html");
  myClient.println("Connection: close");  // connection closed after completion of the response
  myClient.println("Refresh: 60");        // refresh the page automatically every 30 sec
  myClient.println();
  myClient.println("<!DOCTYPE HTML>");
  myClient.println("<html>");
  myClient.println("<br>");               // Newline in HTML
}
//**************************************************************
//**************End Function void startHTTP() ******************
//**************************************************************



//**************************************************************
//****************Function void fileToClient()******************
//**************************************************************
void fileToClient()
{
  myFile = SD.open("HC_Data.txt");          // Open the SD Card file
  myClient.print("<table>");                // start HTML table
  while (myFile.available())                // Read data from the file until end of file
  {
    myClient.print("<tr>");                 // start HTML table row
    endOfLine = false;
    while (endOfLine == false)              // read and send table data until end of line
    {
      myClient.print("<td>");               // start table data cell
      getData(charArray);                   // extract a data item from current line of file
      myClient.print(charArray);            // write table data cell
      myClient.print("</td>");              // end of table data cell
    }
    myClient.print("</tr>");                // end of HTML table row
  }
  myFile.close();
  myClient.print("</table>");               // end HTML table
  myClient.println("<br/>");       
  myClient.println("</html>");
}

//**************************************************************
//****************End Function void fileToClient()**************
//**************************************************************

//**************************************************************
//***********Function void getData(char *dataItem)**************
//**************************************************************
// Read data from a file and parse into data items as the line is read. SemiColon separator.
// The data item is returned in a character array. No need to know how many data items in
// each line or how long the data items are.

void getData(char *dataItem)
{
  for ( byte i = 0; ; i++)           // read characters from file
  {
    char c = myFile.read();
    if (c != ';')               // char is not a semi colon separator, so must be data
    {
      dataItem[i]  = c;         // add char to character array 
    }
    else                        // must be a semi colon separator - end of data item
    {  
      c = myFile.peek();        // peek at the next character in the file
      
      if (c != 13)              // is the next char a CR - end of line?
      {
      dataItem[i] = ' ';
      dataItem[i+1] = '\0';     // terminate the dataItem
    }
      else
      { 
        dataItem[i] = '\0';            // terminate the dataItem                               
        endOfLine = true;              // YES! Set the end of line flag
        c = myFile.read();             // Dummy read to skip past CR
        c = myFile.read();             // Dummy read to skip past LF
      }
      //
      // If we get here, we have reached the end of the data item, and posiibly the end of the
      // line. So we have finished reading for now. Break out of the Read Loop and return to
      // the caller, with the current data item and the end of line flag set true, if
      // appropriate.
      // The file pointer is pointing at the first character of the next data item, or possibly,
      // we have reached the end of the file
      //     
      break;
    }
  }
}

//**************************************************************
//**********End Function void getData(char *dataItem)***********
//**************************************************************



//**************************************************************
//*************** Function void printIPAddress *****************
//**************************************************************
/*
void printIPAddress()
{
   Serial.print("My IP address: ");
   for (byte thisByte = 0; thisByte < 4; thisByte++) 
   {
     // print the value of each byte of the IP address:
     Serial.print(Ethernet.localIP()[thisByte], DEC);
     Serial.print(".");
   }
  Serial.println();
}         
//**************************************************************
//************* END Function void printIPAddress ***************
//**************************************************************  
 */

 
//**************************************************************
//************* Function void bme280RecorderBoot()**************
//**************************************************************

void bme280RecorderBoot()     

//Dim LCD Backlight on D pin 3 using pwm, 0-255 with 255= full on and 0=off
{ 
       lcd.begin(screenWidth,screenHeight);     //initialize the lcd
       lcd.clear();
       lcd.setCursor(0, 0);
       lcd.print("Dimming display");
       lcd.setCursor(0, 1);
       lcd.print("backlight");
       delay(2000); 
       analogWrite(3,100
       );   //Backlight dimmer command (pin #, PWM-% 0n-time-255 max)
                             //Backlight pin 10 is rewired on breadboard to pin 3 to avoid conflict
       delay (500);

// Open serial communications port and wait for port to open:
  
      //Wire.begin();
      //Serial.begin(9600);
      
      while (!Serial); 
  {
      setSyncProvider(RTC.get);   // the function to get the time from the RTC
      if(timeStatus()!= timeSet) 
      Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");
     sprintf(timeDateComma, "%2d:%02d:%02d %02d/%02d/%04d ", hour(), minute(), second(), month(), day(), year());
     Serial.println(timeDateComma);
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("RS-3231 Time is:");
     lcd.setCursor(0,1);
     lcd.println(timeDateComma);
     delay(2000); 
  }
{
  //
//Check BME 280 Sensors
//
  Serial.println();
  Serial.println("Checking Multi Bosch BME280 Barometric Pressure-Humidity-Temp Sensors"); 

  if (!bme1.begin()) {
    Serial.println("Could not find a First BME280 sensor, check wiring!");
    lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("BME1 wiring error");
    
    while (1);
  }
  if (!bme2.begin()) {
    Serial.println("Could not find a Second BME280 sensor, check wiring!");
    lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("BME2 wiring error");
    while (1);
  }
   Serial.println("BME-1 and BME-2 are connected"); 
   Serial.println();
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("BME1 Responding");
   lcd.setCursor(0, 1);
   lcd.print("BME2 Responding");
   delay(2000);
}

{
    {
    Serial.println("I'm waiting for IP address");
    Serial.print("Local IP Address is: ");
    Serial.println(Ethernet.localIP());
    Serial.println();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IP Address");
    lcd.setCursor(0,1);
    lcd.println(Ethernet.localIP());
    delay(1000);
    }

}
//initialize SD Library, card and chip select pin (4)
    pinMode(10,OUTPUT);
    digitalWrite(10, HIGH);
    SD.begin(chipSelect);   //CS-chipSelect is on pin 4
    Serial.print("Initializing SD card...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing SD card...");
    delay(2000);
  
// we'll use the initialization code from the utility libraries
// since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) 
  {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    lcd.setCursor(0, 1);
    lcd.print("SD init. failed");
    delay(2000);
    lcd.clear();
    return;
  } 
  else 
  {
    Serial.println("Wiring is correct and a card is present.");
     lcd.clear();
     lcd.print("Card initialized.");
     delay(2000);
  }

// print the type of card
      Serial.print("\nCard type: ");
      switch (card.type()) 
      {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
      }

//  open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) 
  {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }
  
// print the type and size of the first FAT-type volume
      uint32_t volumesize;
      Serial.print("\nVolume type is FAT");
      Serial.println(volume.fatType(), DEC);
      Serial.println();
      
      lcd.clear();
      lcd.print("Volume is FAT");
      delay(2000);
  
//Determine Size of SD Card and Serial / SD print
      volumesize = volume.blocksPerCluster();    
      volumesize *= volume.clusterCount();       
      volumesize *= 512;                            
      Serial.print("Volume size (bytes): ");
      Serial.println(volumesize);
      Serial.print("Volume size (Kbytes): ");
      volumesize /= 1024;
      Serial.println(volumesize);
      Serial.print("Volume size (Mbytes): ");
      volumesize /= 1024;
      Serial.println(volumesize);
      Serial.println();
      
      lcd.clear();
      lcd.print("Volume (Mbytes): ");
      lcd.setCursor(0, 1); 
      lcd.print(volumesize);
      delay(2000);

/*Look at files on SD Card and display on Serial Monitor      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Scanning files");
      lcd.setCursor(0, 1);
      lcd.print("on SD card");
      
      Serial.println("Files found on the card (name, date and size in bytes): ");  
      root.openRoot(volume);
      
// list all files on the card with date and size
     // root.ls(LS_R | LS_DATE | LS_SIZE);
*/
      lcd.clear();
      Serial.println();
      Serial.println("Time, Date, Temperature, Humidity");

  //sdCardWrite = SD.open("HC_Data.txt", FILE_WRITE);  //Write data to SD card
  //sdCardWrite.println("Begin New Data Set");
  //sdCardWrite.println("Time, Date, Temperature, Humidity");
  //sdCardWrite.close();
}
//**************************************************************
//**********END Function void bme280RecorderBoot()**************
//**************************************************************

//**************************************************************
//************* Function void greeting()************************
//**************************************************************

void greeting()
{
      Wire.begin();
      Serial.begin(9600);
      Serial.println("***Hello!!!***");
      Serial.println();
      Serial.println("I'm awake");
      Serial.println();
      
      lcd.begin(screenWidth,screenHeight);     //initialize the lcd
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("     Hello!     ");
      lcd.setCursor(0, 1);
      lcd.print("   I'm awake");
      delay(2000);
}


/*
//**************************************************************
//************* Function void keepIPconnected ******************
//**************************************************************
//
// Monitor DHCP
//
void keepIPconnected()
{  
switch (Ethernet.maintain()) 
{
case 0:
Serial.println("IP connected");
Serial.println(Ethernet.localIP());
break;
case 1:
lcd.setCursor(0,0);
lcd.println("IP renew failure");
lcd.setCursor(0,1);
lcd.print(Ethernet.localIP());
lcd.println("    ");
Serial.println("IP Renew Failure");
break;
case 2:
lcd.setCursor(0,0);
lcd.println("   IP renewed   ");
lcd.setCursor(0,1);
lcd.print(Ethernet.localIP());
lcd.println("    ");
Serial.println("IP Renewed");
Serial.println(Ethernet.localIP());
break;
case 3:
lcd.setCursor(0,0);
lcd.println("IP rebind fail  ");
lcd.setCursor(0,1);
lcd.print(Ethernet.localIP());
lcd.println("    ");
Serial.println("IP Rebind Failure");
Serial.println(Ethernet.localIP());
break;
case 4:
lcd.setCursor(0,0);
lcd.println("  IP rebind OK  ");
lcd.setCursor(0,1);
lcd.print(Ethernet.localIP());
lcd.println("    ");
Serial.println("IP Rebind OK");
Serial.println(Ethernet.localIP());
break;
}
}

//**************************************************************
//*********** END Function void keepIPconnected ****************
//**************************************************************
*/
