#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include "Cert.h"
// Replace with your network credentials
#define LARGO_API_KEY 64
#define TAMANIO_EEPROM 512
const char* ssid = "calculeta";
const char* password = "calculeta00";
const char* borrado = "x";
//const char* apiurl = "https://calculeta.estonoesunaweb.com.ar/api/test";
const String apiUrlRegistro = "https://calculeta.estonoesunaweb.com.ar/api/v1/registro";
const String apiUrlPiletas = "https://calculeta.estonoesunaweb.com.ar/api/v1/piletas";
// String key = "Bearer ";
String key;


int a = 0;
int value;


bool conectadoAlWifi = false;
bool datosEnviados = false;

X509List cert(IRG_Root_X1); // Create a list of certificates with the server certificate
WiFiClientSecure client;
HTTPClient https;

void setup() {
  Serial.begin(115200);
  Serial.println();
  // Serial.println("calculeta");
  // Serial.print("Flash chip size: ");
  // Serial.println(ESP.getFlashChipSize());

  // borraEEPROM();
  //
  // escribeEEPROM(0, "DATOS DATOS DATOS");
  // Serial.print("0 a 64: ");
  Serial.println(leeEEPROM(0, LARGO_API_KEY));
  // Serial.print("100 a 102: ");
  Serial.println(leeEEPROM(100, 2));
  // Serial.println("FIN.");
}

void loop() {
  if(!tengoApiKey()){
    registraCalculeta();
  }
  // if (!datosEnviados && conectaCalculetaWifi()) {
  //   datosEnviados = enviaPiletas();
  // }

  // EEPROM.begin(TAMANIO_EEPROM);
  // value = EEPROM.read(a);
  // Serial.print(a);
  // Serial.print("\t");
  // Serial.print(value);
  // Serial.println();
  //
  // a++;
  // delay(50);
  // if (a == 512) delay(1000 * 60 * 60);
  
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

bool tengoApiKey(){
  String api_key = leeEEPROM(100, 2);
  Serial.println("***");
  Serial.println(api_key);
  Serial.println("***");
  if (leeEEPROM(100, 2).equals("OK")) {
    key = "Bearer "+ leeEEPROM(0, LARGO_API_KEY);
    key.replace("\"", ""); // saco las comillas que aparecen magimagicamente
    key.replace(".", ""); // saco los puntos que se agregan al guardar la api
    Serial.println("API KEY OK");
    return true;
  }
  else {
    Serial.println("NO API KEY");
    return false;
  }
}

void setupWifi() {
}

void conectaCalculetaWifi() {
  // conecta al wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Buscando un wifi llamado calculeta..."));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(F("Conectado. Seteando la hora..."));
  // parece que se neceita la hora para el handshake de https
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
}

bool enviaPiletas() {
  if ((WiFi.status() == WL_CONNECTED)) {
    client.setTrustAnchors(&cert);

    Serial.println("Enviandos datos ...\n");

    if (https.begin(client, apiUrlPiletas)) {  // HTTPS
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

    // } else
  } else {
    Serial.printf("No está conectado al WiFi");
    return false;
  }
  return false;
}

bool registraCalculeta(String apiUrlRegistro){
  if (https.begin(client, apiUrlRegistro)) {  // HTTPS
    String mac = WiFi.macAddress();

    https.addHeader("Content-Type", "application/x-www-form-urlencoded"); // content-type header

    String httpRequestData = "name="; // request
    httpRequestData += mac;
    httpRequestData += "&email=";
    mac.replace(":", "");
    httpRequestData += mac;
    httpRequestData += "@estonoesunaweb.com.ar&password=";
    httpRequestData += mac;

    int httpCode = https.POST(httpRequestData); // HTTP POST request

    Serial.printf("[HTTPS] REQUEST... code: %d\n", httpCode);
    Serial.printf("[HTTPS] REQUEST... body: %s\n", httpRequestData.c_str());

    String payload = https.getString();

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
