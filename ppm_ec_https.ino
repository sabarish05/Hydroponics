#include "WiFi.h"
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

char MQTT_SERIAL_SUB_CH[10]  = "ph/ec" ;
char message_buff[100]; //store received MQTT response
char* msgString = ""; // Store the received MSG String FRom MQTT
//Comment or uncomment following line to enable/disable MQTT
#define USE_MQTT
int SenData = 0;

int eeAddress;   //Location we want the data to be put.
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;


//**************PIN definitions***************
#define RXD2 16
#define TXD2 17
//#define ECPIN 32
//#define ECPOWER 27
#define ONE_WIRE_BUS 12
//#define TEMP_POWER 13
#define PH_SENSOR_POWER 15

float ECnew;
String part01="";
String part02="";
String part03="";
String part04="";

String part11 = "";//PH CONVERTED
String part12 = "";//W CONVERTED
String part13 = "";//L CONVERTED
String part14 = "";//T 


#ifdef USE_MQTT

//****************Formatting****************
#define IGNORE_FIRST 3
#define READ_MAX 5

//#define LINE_COUNT 4
//***************WIFI & MQTT *****************
const char* ssid = "iQube";
const char* password =  "itsnewagain";
//const char* mqttServer = "10.1.75.125";
//const char* mqttUser = "mlxvbyoj";
//const char* mqttPassword =  "cGR-9abGf5Bx";
//const int mqttPort = 2123;
//char MQTT_SERIAL_PUBLISH_CH[15]  = "iqkct/ppm" ;
String line;
String GOOGLE_SCRIPT_ID ="AKfycbwYdfVS_D6C6Ank0kcBYQr-TDkUkh1XP9O4UB2Qcqx93FKaWY9s";
#endif
//***************PPM constants****************
int R1 = 1000; //Resistor value
int Ra = 25;  //Resistance of powering Pins
float PPMconversion = 0.5;
float TemperatureCoef = 0.019;
float K = 1.88;
float Vin = 3.3;
//***************Variables********************
char c;
String sd, readString;
int LINE_COUNT=4;

float Temperature = 10;
float EC = 0;
float EC25 = 0;
int ppm = 0;

float raw = 0;
float Vdrop = 0;
float Rc = 0;

//******************Initialization************
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18(&oneWire);

#ifdef USE_MQTT
WiFiClient espClient;
HTTPClient http;
//PubSubClient client(espClient);
#endif


void callback(char* topic, byte* payload, unsigned int length)
{

  Serial.println("");
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  //      Serial.print("Message:");
  int i;
  for (i = 0; i < length; i++)
  { 
    //    Serial.print((char)payload[i]);
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  Serial.println();
  msgString = "";
  msgString = message_buff;
  Serial.println(msgString);
  Serial.println("-----------------------");
  if (strcmp(topic, MQTT_SERIAL_SUB_CH) == 0)
  {
    Serial.print("before EC "); Serial.println(EC);
    Serial.print("before K "); Serial.println(K);
    ECnew = String(msgString).toFloat();
    calibrate(ECnew);
    //    String(K) = String(msgString);

    Serial.print("After EC "); Serial.println(EC);
    Serial.print("After K "); Serial.println(K);
  }

}

//***************Setup************************
void ReadEEprom()
{
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  Serial.println("Reading old EEPROM");

  String eK;
  for (int i = 0; i < 32; ++i)
  {
    eK += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("Stored eK EEPROM: ");
  Serial.println(eK);
  K = eK.toFloat() ;
  K=1.88;
  Serial.print("Stored K Value: ");
  Serial.println(K);
}



void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  ReadEEprom();
  //pinMode(TEMP_POWER , OUTPUT );
  //pinMode(ECPIN, INPUT);
  //pinMode(ECPOWER, OUTPUT);
  pinMode(PH_SENSOR_POWER, OUTPUT);
    
  delay(100);// gives sensor time to settle
  //digitalWrite(ECPOWER, HIGH);
  delay(100);
  //Temperature=25;
  R1 = (R1 + Ra); // Taking into acount Powering Pin Resitance
  Serial.println("Welcome");
#ifdef USE_MQTT
  wificonnect();
#endif
   /*String line = getLine();
  Serial.println(line);*/
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  readPPMData();readSerialData();Serial.flush();Serial2.flush();
  sendData("PH="+part11+"&W="+part12+"&L="+part13+"&T="+part14+"&EC25="+ String(EC25)+"&PPM="+String(ppm)+"&TEMP="+String(Temperature));
  delay(1000); 
    String line = getLine();
    Serial.println(line);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
  Serial.println("over");
  Serial.flush();
  esp_deep_sleep_start();
}

//******************Loop*********************
void loop() {
  
 /* ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  if(bootCount%5==0){ readSerialData();Serial.flush();Serial2.flush();sendData("PH="+sd+"&EC25="+ String(EC25)+"&PPM="+String(ppm)+"&TEMP="+String(Temperature));}
  readPPMData();
  delay(1000); 
    String line = getLine();
    Serial.println(line);
    delay(5000);*/
}

String getLine() { //Return the combined message
  //readSerialData();
  //delay(700);
  //readPPMData();
  //EC=EC25=ppm=Temperature=0;
  String line = " ";
  line += "PH: ";
  line += part11;
  line += "W: ";
  line += part12;
  line += "L: ";
  line += part13;
  line += "T: ";
  line += part14;
  line += "EC: ";
  line += String(EC);
  line += ", EC25: ";
  line += String(EC25);
  line += ", PPM: ";
  line += String(ppm);
  line += ", LiqTemp: ";
  line += String(Temperature);

  return line;
}

void readSerialData() {
  //int PH_SENSOR_POWER =15;
  //pinMode(PH_SENSOR_POWER, OUTPUT);
  //delay(100);// gives sensor time to settle
  while (Serial2.available()) { //Clear Input buffer
    Serial2.read();
  }

  //delay(1000);
  //Read Sensor data from Serial
  //  while (Serial2.available()) {
  //    c = Serial2.read();
  //    readString += c;
  //  }
  //  if (.length() > 0) {
  //    sd = readString.substring(IGNORE_FIRST, IGNORE_FIRST + READ_MAX);
  //    //        sd = readString;
  //    Serial.print("sd ===="); Serial.println(sd);
  //    readString = "";
  //  } else {
  //    sd = "";
  //    Serial.println("Sensor off");
  //  }
  int Line = 0;
  sd = "";
    readString="";
  String ph="";
  while (Line < LINE_COUNT) {
    if (Serial2.available()) {
      String ph = Serial2.readStringUntil('\n');
      Serial.println("Lineno:");
      Serial.print(Line);
      Serial.println(ph);
      //ph1= ph[2][5]
      //for i in ph1:

      if (Line==3) {
        readString += ph;
        //sd = readString.substring(IGNORE_FIRST, IGNORE_FIRST + READ_MAX);
        part01 = getValue(readString,':',1);
        part02 = getValue(readString,':',2);
        part03 = getValue(readString,':',3);
        part04 = getValue(readString,':',4);

        part11 = getValue(part01,',',0);//PH CONVERTED
        part12 = getValue(part02,',',0);//W CONVERTED
        part13 = getValue(part03,',',0);//L CONVERTED
        part14 = getValue(part04,',',0);//T CONVERTED

        part11.replace(" ","0");
        part12.replace(" ","0");
        part13.replace(" ","0");
        part14.replace(" ","0");
        
        Serial.println(part11);
        Serial.println(part12);
        Serial.println(part13);
        Serial.println(part14);
        //if(sd[4]==',')sd[4]='0';
      }
      Line++;
    }
  }
  delay(1000);
}

void readPPMData() {
  //*********Reading Temperature Of Solution *******************//
  int TEMP_POWER =13;
  pinMode(TEMP_POWER , OUTPUT );
  delay(300);
  digitalWrite(TEMP_POWER , HIGH );
  delay(500);
    ds18.begin();
  ds18.requestTemperatures();// Send the command to get temperatures
  Temperature = ds18.getTempCByIndex(0); //Stores Value in Variable
  
  digitalWrite(TEMP_POWER , LOW );
  //************Estimates Resistance of Liquid ****************//
  //delay(500);
  int ECPIN =32;
  int ECPOWER=27;
  pinMode(ECPIN, INPUT);
  pinMode(ECPOWER,OUTPUT);
  //delay(200);
  digitalWrite(ECPOWER, HIGH);
  raw = analogRead(ECPIN);
  raw = analogRead(ECPIN); // This is not a mistake, First reading will be low beause if charged a capacitor
  raw = analogRead(ECPIN);
  digitalWrite(ECPOWER, LOW);
  delay(500);
   //digitalWrite(ECPOWER, LOW);
  //pinMode(ECPOWER,INPUT);//dbt

  //***************** Converts to EC **************************//
  //raw=100;
  //Serial.println(raw);
  Vdrop = (Vin * raw) / 4095.0;
  //Serial.println(Vdrop);
  Rc = (Vdrop * R1) / (Vin - Vdrop);
  Rc = Rc - Ra; //acounting for Digital Pin Resitance
  //Serial.print("Rc"); Serial.println(Rc);
  EC = 1000 / (Rc * K);


  //*************Compensating For Temperaure********************//
  EC25  =  EC / (1 + TemperatureCoef * (Temperature - 25.0));
  ppm = (EC25) * (PPMconversion * 1000);

}
#ifdef USE_MQTT
//*************************WiFI*********************************
int restartss = 0;
void wificonnect()
{
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++restartss >= 10)
    {
      ESP.restart();
      restartss = 0;
    }
  }
}
//void callback(char* topic, byte* payload, unsigned int length) {
//  String received = (char*)payload;
//  float cal = received.toFloat();
//  calibrate(cal);
//}

#endif
//*********************Calibration***************************
void calibrate(float exactEC) {
  EC = exactEC * (1 + TemperatureCoef * (Temperature - 25.0));
  Serial.print("exactEC"); Serial.println(exactEC);
  Serial.print("TemperatureCoef"); Serial.println(TemperatureCoef);
  Serial.print("Temperature "); Serial.print(Temperature); Serial.print(" ");  Serial.print((Temperature - 25.0));  Serial.print(" "); Serial.println(TemperatureCoef * (Temperature - 25.0));
  Serial.print("EC"); Serial.println(EC);
  Serial.print("Rc"); Serial.println(Rc);
  K = (1000 / (float)(Rc * EC));
  Serial.print("K CALCUALTOR "); Serial.println(Rc * EC);
  Serial.print("K CALCUALTOR 1000 "); Serial.println( (1000 / (float)(Rc * EC)) );
  Serial.print(" K calculated "); Serial.println(K);
  String qsid = String(K);
  Serial.print("String qsid K calculated "); Serial.println(K);
  if (qsid.length() > 0)
  {
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i)
    {
      EEPROM.write(i, 0);
    }
    Serial.println(qsid);
    Serial.println("");

    Serial.println("writing eeprom ssid:");
    for (int i = 0; i < qsid.length(); ++i)
    {
      EEPROM.write(i, qsid[i]);
      Serial.print("Wrote: ");
      Serial.println(qsid[i]);
    }
    EEPROM.commit();
  }
}


void sendData(String params) {
   String url="https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+params;
   Serial.print(url);
    Serial.print("Making a request");
    http.begin(url); //Specify the URL and certificate
    http.POST(params);
    int httpCode = http.GET();  
    //http.end();
    Serial.println(": done "+httpCode);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
