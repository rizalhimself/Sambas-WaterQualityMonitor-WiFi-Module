#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <TaskScheduler.h>
#include <SoftwareSerial.h>

#define RXpin D1
#define TXpin D2
#define pinLedSinyal D5
#define pinLedData D6

Scheduler userScheduler;
WiFiClient client;
SoftwareSerial pinSerial(RXpin, TXpin);

// task interval
#define sendInterval 15000L
#define sendDataInterval 2000L
#define ledTimeout 1000L

// ThingSpeak account
unsigned long myChannelNumber = 2589091;
const char *myWriteAPIKey = "POIF24AL3SG369PE";

// wifi information
const char *ssid = "RizalNet-Brobot";
const char *password = "blackbird16111992";

// variables used
int tdsValue, tempValue;
float phValue;
JsonDocument docRec, docSend;
int ledDataStatus = 0, thingspeakStatus;
String message2;

// prototype function
void sendData();
void sendDataToInternet();

// define task for multitasking
Task tasksendData(sendDataInterval, TASK_FOREVER, &sendData);
Task taskSendDataToInternet(sendInterval, TASK_FOREVER, &sendDataToInternet);
const unsigned long interval = 100;
unsigned long previousTime = 0;

// send data to software serial
void sendData()
{
  docSend["sig"] = WiFi.RSSI();
  docSend["thstat"] = thingspeakStatus;
  serializeJson(docSend, pinSerial);
  docSend.clear();
}

// recieve data from software serial
void recieveData()
{
  if (pinSerial.available())
  {
    message2 = pinSerial.readString();
    Serial.println("Pesan2 :" + message2);
  }
  deserializeJson(docRec, message2);
  tdsValue = docRec["tds"];
  tempValue = docRec["temp"];
  phValue = docRec["ph"];
  docRec.clear();
}

// connect wifi function
void connectWifi()
{
  WiFi.setOutputPower(20.5);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
    digitalWrite(pinLedSinyal, !digitalRead(pinLedSinyal));
  }
  Serial.println();
  Serial.print("terkoneksi pada :");
  Serial.println(WiFi.SSID());
  Serial.print("kekuatan sinyal :");
  Serial.println(WiFi.RSSI());
  Serial.print("Alamat IP :");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// wifi on disconnect handler
WiFiEventHandler wifiDisconnectHandler;
void onWifiDisconnect(const WiFiEventStationModeDisconnected)
{
  digitalWrite(pinLedSinyal, HIGH);
  WiFi.disconnect();
  connectWifi();
}

// send data to ThingSpeak
void sendDataToInternet()
{
  if (tdsValue != 0 && tempValue != 0 && phValue != 0)
  {
    ThingSpeak.setField(2, tdsValue);
    ThingSpeak.setField(3, tempValue);
    ThingSpeak.setField(1, phValue);
    thingspeakStatus = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (thingspeakStatus == 200)
    {
      Serial.println("Channel update successful.");
      ledDataStatus = 2;
    }
    else
    {
      ledDataStatus = 3;
      Serial.println("Problem updating channel. HTTP error code " + String(thingspeakStatus));
      ledDataStatus = 1;
    }
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(RXpin, INPUT);
  pinMode(TXpin, OUTPUT);
  pinMode(pinLedSinyal, OUTPUT);
  pinMode(pinLedData, OUTPUT);
  digitalWrite(pinLedData, LOW);
  digitalWrite(pinLedSinyal, HIGH);
  pinSerial.begin(9600);
  WiFi.mode(WIFI_STA);
  connectWifi();
  digitalWrite(pinLedSinyal, HIGH);
  wifiDisconnectHandler;
  WiFi.onStationModeDisconnected(onWifiDisconnect);
  Serial.print("Waiting for serial connection ..");
  while (!pinSerial.available())
  {
    Serial.print('.');
    delay(500);
    digitalWrite(pinLedData, !digitalRead(pinLedData));
  }
  Serial.println("Device Connected!");
  digitalWrite(pinLedData, LOW);
  ThingSpeak.begin(client);
  userScheduler.addTask(tasksendData);
  userScheduler.addTask(taskSendDataToInternet);
  taskSendDataToInternet.enable();
  tasksendData.enable();
}

void loop()
{
  // put your main code here, to run repeatedly:
  userScheduler.execute();
  recieveData();

  if (ledDataStatus == 1)
  {
    unsigned long current = millis();

    if (current - previousTime >= interval)
    {
      digitalWrite(pinLedData, !digitalRead(pinLedData));
      previousTime = current;
    }
  }
  else if (ledDataStatus == 2)
  {
    unsigned long current = millis();
    if (current - previousTime >= interval)
    {
      digitalWrite(pinLedData, HIGH);
    }
  }
  else if (ledDataStatus == 3)
  {
    digitalWrite(pinLedData, LOW);
  }
}