#include <Wire.h>
#include <AP3216_WE.h>
#include <ClosedCube_HDC1080.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

AP3216_WE myAP3216 = AP3216_WE();
ClosedCube_HDC1080 sensor;

SoftwareSerial ss(2,0);
TinyGPS gps;

const char* ssid     = "R2D2-C3P0";
const char* password = "Al1n4y4n3t";
const char* servername = "http://ec2-3-222-231-45.compute-1.amazonaws.com/datos";

const char* host = "ec2-3-222-231-45.compute-1.amazonaws.com";
const uint16_t port = 80;

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
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
  
  WiFiClient client;
  HTTPClient http;
  DynamicJsonDocument doc(1024);

  doc["id"]= id;
  doc["temp"]= temp;
  doc["hum"]= hum;
  doc["lux"]= lux;
  doc["lat"]= lat;
  doc["lon"]= lon;
  doc["alt"]= alt;

  http.begin(client, servername);
  http.addHeader("multipart", "form-data");
  String data;
  serializeJson(doc, data);
  
  int responsecode = http.POST(data);
  Serial.print(responsecode);
}
