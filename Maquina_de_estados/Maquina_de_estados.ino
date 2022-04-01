#include <Wire.h>
#include <AP3216_WE.h>
#include <ClosedCube_HDC1080.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Hash.h>

AP3216_WE myAP3216 = AP3216_WE();
ClosedCube_HDC1080 sensor;

ESP8266WiFiMulti WiFiMulti;

SoftwareSerial ss(2,0);
TinyGPS gps;

const char* STASSID     = "R2D2-C3P0";
const char* STAPSK = "Al1n4y4n3t";
const char* SERVER_IP = "https://54.236.223.22/sensor_send_data";

const char* host = "https://34.198.69.187";
const uint8_t fingerprint[20] = {0x86, 0xEA, 0xD6, 0xFD, 0xFC, 0xD6, 0x61, 0xF7, 0x4C, 0xEA, 0x71, 0xB9, 0x70, 0xA1, 0x57, 0xEB, 0x65, 0xAB, 0x02, 0xDB};
const uint16_t port = 443;
char* key = "-/-///-//-";

int id = 366423;
double temp;
double hum;
double lux;
float lat;
float lon;
float alt;

void setup() {

  Serial.begin(115200);
  ss.begin(9600);
  Wire.begin();
  myAP3216.init();
  myAP3216.setMode(AP3216_ALS);
  myAP3216.setLuxRange(RANGE_20661);
  sensor.begin(0x40);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);

  /*while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());*/

  /*if (client.verify(fingerprint, host)) {
    Serial.println("Fingerprint matches");
  } else {
    Serial.println("Fingerprint doesn't match");
  }*/
}

void loop() {
  smartdelay(15000);
  tempan();
  Humidity();
  luxes();
  Serial.print("Temperatura: ");
  Serial.print(temp);
  Serial.println(" C°");
  Serial.print("Humedad: ");
  Serial.print(hum);
  Serial.println(" %");
  Serial.print("Lux: ");
  Serial.println(lux);
  Serial.print("Latitud: ");
  Serial.println(lat,5);
  Serial.print("Longitud: ");
  Serial.println(lon,5);
  Serial.print("Altitud: ");
  Serial.println(alt,5);
  sendata();
}

void smartdelay(int param){

  unsigned long start = millis();
    do{
        while(ss.available()){
          leerGPS();
        }
    }while(millis() - start < param);
}

char* XORENC(char* in, char* key){
  int insize = strlen(in);
  int keysize = strlen(key);
  int x = 0;
  for(int i=0; i<insize; i++){
    in[i]=(in[i]^key[x]);
    if(x==keysize-1){
      x=0;
    }else{
      x++;
    }
  }
  return in;
}

void tempan(){

  temp = 0;

  for(int i=0;i<20;i++){
    temp = temp + sensor.readTemperature();
  }
  temp=temp/20;
  smartdelay(0);
}

void Humidity(){

  hum = 0;

  for(int i=0;i<20;i++){
    hum = hum + sensor.readHumidity();
  }
  hum=hum/20;
  smartdelay(0);

}

void luxes(){

  lux = 0;
  for(int i=0;i<20;i++){
    lux = lux + myAP3216.getAmbientLight();
  }
  lux=lux/20;
  smartdelay(0);
}

void leerGPS(){

  int c = ss.read();
  if(gps.encode(c)){
    gps.f_get_position(&lat, &lon);
    alt = gps.f_altitude();
  }
}

void sendata(){

  //WiFiClient client;
  //HTTPClient http;
  DynamicJsonDocument doc(1024);
  DynamicJsonDocument doc2(1024);

  doc["temp"]= temp;
  doc["hum"]= hum;
  doc["lux"]= lux;
  doc["lat"]= lat;
  doc["lon"]= lon;
  doc["alt"]= alt;
  
  //http.begin(client, servername);
  //http.addHeader("multipart", "form-data");
  String data;
  String data2;
  serializeJson(doc, data);

  char checksum[sha1(data).length()];
  sha1(data).toCharArray(checksum, (sha1(data).length() + 5));
  char* checksumencrypted = XORENC(checksum,key); 

  char temp[5];
  char texto[strlen(checksumencrypted)*3];

  for(int i = 0; i < strlen(checksumencrypted); i++){
    if(i == strlen(checksumencrypted-1)){
      sprintf(temp, "%x" , checksumencrypted[i]);
    }else{
      sprintf(temp, "%x:" , checksumencrypted[i]);
    }
    strcat(texto, temp);
  }

  Serial.println(texto);

  doc2["id"] = id;
  doc2["data"] = data;
  doc2["checksum"] = String(texto);
  serializeJson(doc2, data2);

  /*if (client.connect(host, port)) {
    client.println("POST /datos HTTP/1.1");
    client.print("HOST: "); client.println(host);
    client.println("Content-Type: application/json;");
    client.print("Content-Length: "); client.println(data.length());
    client.println();
    client.println(data);
  }else{
    Serial.println(" no ");
  }*/
  
  //int responsecode = http.POST(data);
  //Serial.print(responsecode);

  if ((WiFiMulti.run() == WL_CONNECTED)) {
  
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  
      client->setFingerprint(fingerprint);
      //  Or, if you happy to ignore the SSL certificate, then use the following line instead:
      //  client->setInsecure();
  
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\n");
      https.begin(*client, SERVER_IP);
      https.addHeader("Content-Type", "application/json");
  
      Serial.print("[HTTPS] POST...\n");
      //  Start connection and send HTTP header
      int httpCode = https.POST(data2);
      //  HttpCode will be negative on error
      if (httpCode > 0) {
        //  HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
    //Serial.println("Wait 10s before next round...");
    //delay(10000);


  
}
