#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Arduino.h>
#include <FS.h>
#include "Kalibrierung.h"
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <string.h>

const char *SSID = "******";
const char *PSW = "****";
//hier die nr. des Sensors eintragen um eine einfachere handhabung bei der ip eingabe zu haben
const char *Nummnerierung_der_server = "Beet-vorm-fenster";
const char *version = "1.0";

bool restart = false;
Kalibrierung kalibrierung;
kali_dat dat;
ESP8266WebServer server(80);

void handle_restart()
{
  server.send(200, "text / plain", "neustart nachdem alle bewaesserungen ausgefuehrt wurden");
  restart = true;
}

void handle_version()
{
  String ausgabe = String(version);
  server.send(200, "text / plain", ausgabe);
}

void handle_root()
{
  String ausgabe = "zum kalibrieren /kalibrierung?typ=<0 fuer trocken 1 fuer nass>   zum neustart /restart zum abfragen /abfrage      version abfrage /version . aktuell installierte version: ";
  ausgabe += String(version);

  server.send(200, "text / plain", ausgabe);
}

void handle_kali()
{
  bool probl = false;
  String message = "";
  dat = kalibrierung.laden();
  if (server.arg("typ") == "")
  {
    message = "ERROR";
  }
  else
  {
    if (server.arg("typ") == "0")
    {
      try
      {
        analogRead(A0);
      }
      catch (...)
      {
        probl = true;
      }

      if (!probl)
      {
        dat.trocken = analogRead(A0);
        kalibrierung.speichern(dat);
        message += "gespeichert";
        Serial.println("Trocken gespeichert auf abrfage");
      }
      else
      {
        dat.trocken = 1024;
        kalibrierung.speichern(dat);
        message += "Fehler, ist A0 richtig verbunden?";
        Serial.println("error");
      }
    }
    if (server.arg("typ") == "1")
    {
      try
      {
        analogRead(A0);
      }
      catch (...)
      {
        probl = true;
      }
      if (!probl)
      {

        dat = kalibrierung.laden();
        dat.nass = analogRead(A0);
        Serial.print("dat.nass nach rechnung: )");
        Serial.println(dat.nass);
        kalibrierung.speichern(dat);
        message += "gespeichert";
        Serial.println("nass gespeichert auf abrfage");
      }
      else
      {
        dat.nass = 0;
        kalibrierung.speichern(dat);
        message += "Fehler, ist A0 richtig verbunden?";
        Serial.println("error");
      }
    }
  }
  server.send(200, "text / plain", message);
}

void handle_abfrage()
{
  String message = "";
  bool probl = false;

  try
  {
    analogRead(A0);
  }
  catch (...)
  {
    probl = true;
  }

  if (probl)
  {
    message = "112";
  }
  else
  {
    dat = kalibrierung.laden();
    Serial.print("dat.trocken=");
    Serial.println(dat.trocken);
    Serial.print("dat.nass=");
    Serial.println(dat.nass);
    float max = dat.trocken - dat.nass;
    Serial.print("max:");
    Serial.println(max);
    float akt_wert = analogRead(A0) - dat.nass;
    Serial.print(" Messung Aktuell");
    Serial.println(analogRead(A0) - dat.nass);
    float akt_wert_help = (1 - akt_wert / max) * 100;
    int help = akt_wert_help;
    Serial.print("aktueller wert:= ");
    Serial.println(help);
    message += String(help);
  }

  server.send(200, "text / plain", message);
}

void setup()
{
  EEPROM.begin(10);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSW);
  Serial.print("Waiting to connect");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/kalibrierung", handle_kali);
  server.on("/abfrage", handle_abfrage);
  server.on("/", handle_root);
  server.on("/restart", handle_restart);
  server.on("/version", handle_version);
  server.begin();
  char *name = new char[strlen("feuchteserver") + strlen(Nummnerierung_der_server)];
  strcpy(name, "feuchteserver");
  strcat(name, Nummnerierung_der_server);
  Serial.print("dns: ");
  Serial.print(name);
  Serial.println(".local");
  if (!MDNS.begin(name))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  Serial.println("Server listening");
}

void loop()
{
  MDNS.update();
  server.handleClient();
  if (restart)
  {
    ESP.restart();
  }
}
