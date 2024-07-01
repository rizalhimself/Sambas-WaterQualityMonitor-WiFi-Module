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
#define recieveInterval 500L
#define ledTimeout 1000L

// ThingSpeak account
unsigned long myChannelNumber = 2563638;
const char *myWriteAPIKey = "AWNAQ8XKBYBX9B71";

// wifi information
const char *ssid = "RizalNet-Brobot";
const char *password = "blackbird16111992";

// variables used
int tdsValue, tempValue;
float phValue;
JsonDocument doc;
int ledDataStatus = 0;
String message2;

// prototype function
void recieveData();
void sendData();

// define task for multitasking
Task taskReceveData(recieveInterval, TASK_FOREVER, &recieveData);
Task taskSendData(sendInterval, TASK_FOREVER, &sendData);
const unsigned long interval = 100;
unsigned long previousTime = 0;

// recieve data from serial
void recieveData()
{
  // String message;
  // if (pinSerial.available())
  // {
  //   message = pinSerial.readString();
  // }
  // deserializeJson(doc, message);
  // tdsValue = doc["tds"];
  // temperature = doc["temp"];
  // Serial.println("Pesan1 :" + message);
  // doc.clear();
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
void sendData()
{
  if (tdsValue != 0 && tempValue != 0 && phValue != 0)
  {
    ThingSpeak.setField(1, tdsValue);
    ThingSpeak.setField(2, tempValue);
    ThingSpeak.setField(3, phValue);
    int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (status == 200)
    {
      Serial.println("Channel update successful.");
      ledDataStatus = 2;
    }
    else
    {
      ledDataStatus = 3;
      Serial.println("Problem updating channel. HTTP error code " + String(status));
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
  ThingSpeak.begin(client);
  userScheduler.addTask(taskReceveData);
  userScheduler.addTask(taskSendData);
  taskReceveData.enable();
  taskSendData.enable();
}

void loop()
{
  // put your main code here, to run repeatedly:
  userScheduler.execute();

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

  if (pinSerial.available())
  {
    message2 = pinSerial.readString();
    Serial.println("Pesan2 :" + message2);
  }
  deserializeJson(doc, message2);
  tdsValue = doc["tds"];
  tempValue = doc["temp"];
  phValue = doc["ph"];
  doc.clear();
}