#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include "Cert.h"

#define LARGO_API_KEY 64
#define TAMANIO_EEPROM 512

const char* ssid = "calculeta";
const char* password = "calculeta00";

const char* apiUrlTest = "https://calculeta.estonoesunaweb.com.ar/api/test";
const char* apiUrlRegistro = "https://calculeta.estonoesunaweb.com.ar/api/v1/registro";
const char* apiUrlPiletas = "https://calculeta.estonoesunaweb.com.ar/api/v1/piletas";

bool setupWifi = true;
bool conectadoWifi = false;
bool wifi = true;
String key;

X509List cert(IRG_Root_X1); // Create a list of certificates with the server certificate
WiFiClientSecure client;
HTTPClient https;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // borraEEPROM();
  Serial.println(leeEEPROM(0, LARGO_API_KEY));
}

void loop() {
  if (wifi) {
    if(!conectadoWifi) conectarWifi(setupWifi);
    if(!tengoApiKey()) registraCalculeta(apiUrlRegistro); 
    else enviaDatos("test", apiUrlPiletas, key);
  }
}

bool tengoApiKey(){
  // String api_key = leeEEPROM(100, 2);
  if (leeEEPROM(100, 2).equals("OK")) {
    key = "Bearer " + leeEEPROM(0, LARGO_API_KEY);
    key.replace(".", ""); // saco los puntos que se agrego al guardar la api
    Serial.println(F("ApiKey OK"));
    return true;
  } else {
    Serial.println(F("NO hay ApiKey"));
    Serial.println(F("se debe registrar la calculeta con la web"));
    return false;
  }
}

void conectarWifi(bool setupWifi) {
  if (setupWifi){
    WiFi.mode(WIFI_STA); // conecta al wifi
    WiFi.begin(ssid, password);
    client.setTrustAnchors(&cert); // linkea el cliente al certificado
    setupWifi = false;
    conectadoWifi = true;
  }

  Serial.print(F("esperando WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }
  Serial.println(F("\nconectado.\nseteando la hora"));
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // parece que se necesita la hora para el handshake de https
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  // if (https.begin(client, apiUrlTest)) {  // HTTPS
  //   int httpCode = https.GET();
  //   Serial.printf("[HTTPS] TEST REQUEST CODE: %d\n", httpCode);
  //   String payload = https.getString();
  //   Serial.println(payload);
  // }
}

void enviaDatos(String datos, String url, String llave) {
  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.println(F("enviandos datos"));

    if (https.begin(client, url)) {  // HTTPS
      https.addHeader("Content-Type", "application/x-www-form-urlencoded");
      https.addHeader("Authorization", llave); 
      String httpRequestData = "pileta=";
      httpRequestData += datos;
      // httpRequestData += "{'P': [76, 58, 94], 'D': 99}";
      int httpCode = https.POST(httpRequestData);
      Serial.printf("[HTTPS] REQUEST... code: %d\n", httpCode);
      Serial.printf("[HTTPS] REQUEST... body: %s %s\n", url.c_str(), httpRequestData.c_str());
      String payload = https.getString();
      Serial.println(payload);
      // if (httpCode > 0) {
      // }
      if (httpCode == 201) { // codigo 201 significa que se escribió en la DB
        WiFi.mode(WIFI_OFF);
        wifi = false;
        Serial.println(F("apagando WiFi"));
        return;
      } else {
        return;
      }
    } else {
      return;
    }
  } else {
    conectadoWifi = false;
    setupWifi = true;
    Serial.println(F("sin WiFi"));
    return;
  }
  return;
}

bool registraCalculeta(String url){
  if (https.begin(client, url)) {  // HTTPS
    String mac = WiFi.macAddress();
    mac.replace(":", "");

    https.addHeader("Content-Type", "application/x-www-form-urlencoded"); // content-type header

    String httpRequestData = "name="; // request
    httpRequestData += mac;
    httpRequestData += "&email=";
    httpRequestData += mac;
    httpRequestData += "@estonoesunaweb.com.ar&password=";
    httpRequestData += mac;

    int httpCode = https.POST(httpRequestData); // HTTP POST request

    Serial.printf("[HTTPS] REQUEST... code: %d\n", httpCode);
    Serial.printf("[HTTPS] REQUEST... body: %s\n", httpRequestData.c_str());
    Serial.printf("[HTTPS] REQUEST... codigo https: %s\n", https.errorToString(httpCode).c_str());

    String payload = https.getString();
    payload.replace("\"", ""); // saco las comillas que vienen de la respuesta

    if (httpCode == 201) { // codigo 201 significa que se escribió en la DB

      Serial.println(payload);

      while(payload.length() < LARGO_API_KEY) {
        payload += ".";
        Serial.println(payload);
      }
      escribeEEPROM(0, payload);
      escribeEEPROM(100, "OK");

      https.end();
      return true;
    } else {
      Serial.printf("[HTTPS] REQUEST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      Serial.printf("[HTTPS] payload: %s\n", payload);
      https.end();
      return false;
    }
  }
  return false;
}

void escribeEEPROM(int startAddress, String data) {
  EEPROM.begin(TAMANIO_EEPROM);
  int largo = data.length();
  EEPROM.put(startAddress, largo);

  for (int i = 0; i < largo; ++i) {
    EEPROM.put(startAddress + sizeof(int) + i, data[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

String leeEEPROM(int startAddress, int largo) {
  EEPROM.begin(TAMANIO_EEPROM);
  EEPROM.get(startAddress, largo);
  char buffer[largo + 1];
  for (int i = 0; i < largo; ++i) {
    EEPROM.get(startAddress + sizeof(int) + i, buffer[i]);
  }
  buffer[largo] = '\0';  // Null-terminate the string
  EEPROM.end();
  return String(buffer);
}

void borraEEPROM(){
  String valor = "x";
  EEPROM.begin(TAMANIO_EEPROM);
  for (int i = 0; i < TAMANIO_EEPROM; i++) {
   EEPROM.write(i, 0);
  }
  EEPROM.end();

  EEPROM.begin(TAMANIO_EEPROM);
  EEPROM.put(0, LARGO_API_KEY);
  for (int i = 0; i < LARGO_API_KEY; ++i) {
    EEPROM.put(0 + sizeof(int) + i, valor[0]);
  }
  EEPROM.commit();
  EEPROM.end();

  EEPROM.begin(TAMANIO_EEPROM);
  EEPROM.put(100, 2);
  for (int i = 0; i < 2; ++i) {
    EEPROM.put(100 + sizeof(int) + i, valor[0]);
  }
  EEPROM.commit();
  EEPROM.end();
}

