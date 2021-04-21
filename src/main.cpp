#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "Kalibrierung.h"
#include <EEPROM.h>
#include <string.h>

AsyncWebServer server(80);

const char *ssid = "SSID";
const char *password = "PSW";
const char *version = "1.2";
bool restart = false;
Kalibrierung kalibrierung;
kali_dat dat;

const char *OTA_INDEX PROGMEM = R"=====(<!DOCTYPE html><html><head><meta charset=utf-8><title>OTA</title></head><body><div class="upload"><form method="POST" action="/ota" enctype="multipart/form-data"><input type="file" name="data" /><input type="submit" name="upload" value="Upload" title="Upload Files"></form></div></body></html>)=====";

const char *PARAM_MESSAGE = "message";

void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    Serial.printf("UploadStart: %s\n", filename.c_str());
    // calculate sketch space required for the update, for ESP32 use the max constant
#if defined(ESP32)
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
#else
    const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace))
#endif
    {
      // start with max available size
      Update.printError(Serial);
    }
#if defined(ESP8266)
    Update.runAsync(true);
#endif
  }

  if (len)
  {
    Update.write(data, len);
  }

  // if the final flag is set then this is the last frame of data
  if (final)
  {
    if (Update.end(true))
    {
      // true to set the size to the current progress
      Serial.printf("Update Success: %ub written\nRebooting...\n", index + len);
      ESP.restart();
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

void handle_restart(String &message)
{
  message = "neustart starten";
  restart = true;
}

void handle_version(String &message)
{
  message = String(version);
}

void handle_root(String &message)
{
  message = "zum kalibrieren /kalibrierung?typ=<0 fuer trocken 1 fuer nass>   zum neustart /restart zum abfragen /abfrage      version abfrage /version . aktuell installierte version: ";
  message += String(version);
}

void handle_kali(String &message)
{
  bool probl = false;
  dat = kalibrierung.laden();
  if (message == "0")
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
      message = String(dat.trocken);
      message += " bits für Trocken gespeichert";
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
  if (message == "1")
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
      message = String(dat.nass);
      message += " bits für nass gespeichert";
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

void handle_abfrage(String &message)
{
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
    message = String(help);
  }
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  EEPROM.begin(125);
  Serial.begin(115200);
  dat = kalibrierung.laden();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    timeout++;
    if (timeout >= 20)
    {
      ESP.restart();
    }
  }
  Serial.println(" ");
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_root(message);
    request->send(200, "text/plain", message);
  });
  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_version(message);
    request->send(200, "text/plain", message);
  });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_restart(message);
    request->send(200, "text/plain", message);
  });
  server.on("/abfrage", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_abfrage(message);
    request->send(200, "text/plain", message);
  });
  server.on("/kalibrierung", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("typ"))
    {
      message = request->getParam("typ")->value();
      handle_kali(message);
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, GET: " + message);
  });
  server.on(
      "/ota",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        request->send(200);
      },
      handleOTAUpload);

  server.on("/ota",
            HTTP_GET,
            [](AsyncWebServerRequest *request) {
              AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", OTA_INDEX);
              request->send(response);
            });
  server.onNotFound(notFound);

  server.begin();
}

void wifi_reconnect()
{
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 5)
  {
    wifi_retry++;
    Serial.println("WiFi not connected. Try to reconnect");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    delay(500);
  }
  if (wifi_retry >= 5)
  {
    Serial.println("\nReboot");
    ESP.restart();
  }
};

void loop()
{
  wifi_reconnect();
  if (restart)
  {
    ESP.restart();
  }
}
