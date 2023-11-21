#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include "Cert.h"
// Replace with your network credentials
const char* ssid = "calculeta";
const char* password = "calculeta";
//const char* apiurl = "https://calculeta.estonoesunaweb.com.ar/api/test";
const String apiregistro = "https://calculeta.estonoesunaweb.com.ar/api/v1/registro";
const String apipiletas = "https://calculeta.estonoesunaweb.com.ar/api/v1/piletas";
String key = "Bearer ";

bool conectadoAlWifi = false;
bool datosEnviados = false;

// Create a list of certificates with the server certificate
X509List cert(IRG_Root_X1);
WiFiClientSecure client;
HTTPClient https;



void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("calculeta");
  EEPROM.begin(512);

  /******/
  //writeStringToEEPROM(100, "cacacacacaca");
  Serial.print("eeprom api: ");
  key = key + readStringFromEEPROM(0, 50);
  key.replace("\"", "");
  Serial.println(key);
  Serial.print("key: ");
  Serial.println(readStringFromEEPROM(100, 7));
  /******/
}

void loop() {
  if (!datosEnviados && conectaCalculetaWifi()) {
    datosEnviados = enviaPiletas();
  }
}

void writeStringToEEPROM(int startAddress, String data) {
  // Write the length of the string first
  int length = data.length();
  EEPROM.put(startAddress, length);

  // Write the string starting from the next address
  for (int i = 0; i < length; ++i) {
    EEPROM.put(startAddress + sizeof(int) + i, data[i]);
  }
  EEPROM.commit();
}

String readStringFromEEPROM(int startAddress, int length) {
  // Read the length of the string

  EEPROM.get(startAddress, length);

  // Read the string starting from the next address
  char buffer[length + 1];
  for (int i = 0; i < length; ++i) {
    EEPROM.get(startAddress + sizeof(int) + i, buffer[i]);
  }
  buffer[length] = '\0';  // Null-terminate the string

  return String(buffer);
}

void setupWifi() {
}

bool conectaCalculetaWifi() {
  // conecta al wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println(F("Buscando un wifi llamado calculeta..."));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println(F("Conectado. Seteando la hora..."));
  // parece que se neceita la hora para el handshake de https
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);

  /*while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
  */
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  return true;
}

bool enviaPiletas() {
  if ((WiFi.status() == WL_CONNECTED)) {
    client.setTrustAnchors(&cert);

    Serial.println("Envia Pileta...\n");

    // ya tenemos api key
    if (readStringFromEEPROM(100, 7).equals("api_key")) {
      Serial.println("Api key...");
      if (https.begin(client, apipiletas)) {  // HTTPS
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        https.addHeader("Authorization", key); 
        String httpRequestData = "pileta=";
        httpRequestData += "{'P': [76, 58, 94], 'D': 99}";
        int httpCode = https.POST(httpRequestData);
        Serial.printf("[HTTPS] REQUEST... code: %d\n", httpCode);
        Serial.printf("[HTTPS] REQUEST... body: %s\n", httpRequestData.c_str());
        String payload = https.getString();
        Serial.println(payload);
        if (httpCode > 0) {
        }
        return true;
      }

    } else {
      if (https.begin(client, apiregistro)) {  // HTTPS
        Serial.print("[HTTPS] REQUEST...\n");

        // Specify content-type header
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String mac = WiFi.macAddress();
        // Data to send with HTTP POST
        String httpRequestData = "name=";
        httpRequestData += mac;
        httpRequestData += "&email=";
        mac.replace(":", "");
        httpRequestData += mac;
        httpRequestData += "@estonoesunaweb.com.ar&password=";
        httpRequestData += mac;
        // Send HTTP POST request
        int httpCode = https.POST(httpRequestData);

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] REQUEST... code: %d\n", httpCode);
          Serial.printf("[HTTPS] REQUEST... body: %s\n", httpRequestData.c_str());
          //Serial.printf("[HTTPS] REQUEST... body: %s\n", https.errorToString(httpCode).c_str());

          String payload = https.getString();
          Serial.println(payload);

          // el server dice que se creó un registro hay que guardar el token en la eeprom
          if (httpCode == 201) {
            writeStringToEEPROM(0, payload);
            writeStringToEEPROM(100, "api_key");
          }

          https.end();
          return true;
        } else {
          Serial.printf("[HTTPS] REQUEST... failed, error: %s\n", https.errorToString(httpCode).c_str());
          https.end();
          return false;
        }
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
        https.end();
        return false;
      }
    }
  }
  return false;
}
