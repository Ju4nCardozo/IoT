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

AP3216_WE myAP3216 = AP3216_WE(); //Declaración sensor de radiación solar
ClosedCube_HDC1080 sensor; // Declaración sensor de temperatra
ESP8266WiFiMulti WiFiMulti; // Declaración WiFi
SoftwareSerial ss(2,0); // Comunicación serial I2C
TinyGPS gps; // Libreria de GPS

const char* STASSID = "R2D2-C3P0"; //Nombre de red
const char* STAPSK = "Al1n4y4n3t"; //Contraseña de red
const char* SERVER_IP = "https://54.236.223.22/sensor_send_data"; //Ip del servidor en el cual se van a enviar los datos
const uint8_t fingerprint[20] = {0x86, 0xEA, 0xD6, 0xFD, 0xFC, 0xD6, 0x61, 0xF7, 0x4C, 0xEA, 0x71, 0xB9, 0x70, 0xA1, 0x57, 0xEB, 0x65, 0xAB, 0x02, 0xDB}; //Huella digital para que se acepte la conexión segura
const uint16_t port = 443; //Puerto del HTTPS
char* key = "-/-///-//-"; //Llave de encriptación XOR (Para no repudio)

int id = 366423; //Id del estudiante
double temp; //Variable de temperatura
double hum; //Variable de humedad
double lux; //Variable de radiación solar
float lat; //Variable de latitud
float lon; //Variable de longitud
float alt; //Variable de altitud

void setup() {

  //Se realiza toda la configuración necesaria para el funcionamiento de los sensores, GPS y WiFi
  Serial.begin(115200);
  ss.begin(9600);
  Wire.begin();
  myAP3216.init();
  myAP3216.setMode(AP3216_ALS);
  myAP3216.setLuxRange(RANGE_20661);
  sensor.begin(0x40);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);
}

void loop() {

  //Se utilizan los métodos implementados para el funcionamiento de los sensores y GPS para luego imprimir el valor de cada valor medido en Monitor Serie
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
  
  //Implementación de programación sincrona y asincrona para el funcionamiento de los sensores y el GPS, recibe un parametro en milisegundos para sensarlos cada 15 segundos en este caso. 
  unsigned long start = millis();
    do{
        while(ss.available()){
          leerGPS();
        }
    }while(millis() - start < param);
}

char* XORENC(char* in, char* key){
  
  int insize = strlen(in); //Tamaño de la variable a codificar
  int keysize = strlen(key); //Tamaño de la llave para codificar
  int x = 0; //Posición
  for(int i=0; i<insize; i++){ //Ciclo en el cual se le realiza la operación XOR a cada caracter de la variable de entrada con la llave
    in[i]=(in[i]^key[x]); //Operación XOR
    if(x==keysize-1){ //Condicional que verifica si el ciclo está en la posición final
      x=0;
    }else{
      x++;
    }
  }
  return in; //Retorna la variable codificada
}

void tempan(){

  temp = 0; //Se inicializa la variable temperatura en 0

  //Proceso de prunning de la temperatura
  for(int i=0;i<20;i++){ //Ciclo para censar 20 veces la temperatura 
    temp = temp + sensor.readTemperature();
  }
  temp=temp/20; //Se divide la temperatura por el número de veces que fue censado
  smartdelay(0); //Se utiliza el método de smartdelay() para que este método funcione en conjunto con el GPS
}

void Humidity(){

  hum = 0;//Se inicializa la variable humedad en 0

  //Proceso de prunning de la temperatura
  for(int i=0;i<20;i++){ //Ciclo para censar 20 veces la humedad 
    hum = hum + sensor.readHumidity();
  }
  hum=hum/20; //Se divide la humedad por el número de veces que fue censado
  smartdelay(0); //Se utiliza el método de smartdelay() para que este método funcione en conjunto con el GPS

}

void luxes(){

  lux = 0; //Se inicializa la variable radiación solar en 0
  for(int i=0;i<20;i++){ //Ciclo para censar 20 veces la radiación solar 
    lux = lux + myAP3216.getAmbientLight();
  }
  lux=lux/20; //Se divide la radiación solar por el número de veces que fue censado
  smartdelay(0); //Se utiliza el método de smartdelay() para que este método funcione en conjunto con el GPS
}

void leerGPS(){

  int c = ss.read(); //Se lee la información en el serial
  if(gps.encode(c)){ //Se codifica el mensaje
    gps.f_get_position(&lat, &lon); //Se obtiene la posición (Longitud y latitud)
    alt = gps.f_altitude(); //Se obtiene la altitud (Longitud y latitud)
  }
}

void sendata(){
  
  DynamicJsonDocument doc(1024); //Variable tipo llave y valor (Json) dinámica que almacena los datos censados
  DynamicJsonDocument doc2(1024); //Variable tipo llave y valor (Json) dinámica que almacena la información siguiente información de la trama: id, datos censados y checksum 

  doc["temp"]= temp; //Se almacena en la llave 'temp' (temperatura) el valor censado y promediado de la temperatura
  doc["hum"]= hum; //Se almacena en la llave 'hum' (humedad) el valor censado y promediado de la humedad
  doc["lux"]= lux; //Se almacena en la llave 'lux' (radiación solar) el valor censado y promediado de la radiación solar
  doc["lat"]= lat; //Se almacena en la llave 'lat' (latitud) el valor censado y promediado de la latitud
  doc["lon"]= lon; //Se almacena en la llave 'lon' (longitud) el valor censado y promediado de la longitud
  doc["alt"]= alt; //Se almacena en la llave 'alt' (altitud) el valor censado y promediado de la altitud

  String data; //Variable que contiene los datos censados
  String data2; //Variable que contiene parte de la trama, es decir, el id, los datos censados y el checksum
  serializeJson(doc, data); //Se serializa el Json 'doc' y se almacena en 'data' como String

  char checksum[sha1(data).length()]; //Variable que contiene el checksum, realizado con el algoritmo has sha1()
  sha1(data).toCharArray(checksum, (sha1(data).length() + 5));//Se convierte el checksum con el hash generado por el algoritmo sha1() de una cadena de caracteres a un array de los mismos
  char* checksumencrypted = XORENC(checksum,key); //Se realiza la operación XOR al checksum anterior y la llave y se almacena en esta variable

  //En esta sección de código se convierte cada caracter del checksum encriptado con XOR en bytes
  //Esto es necesario para que no se pierdan datos
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

  doc2["id"] = id; //Se almacena en la llave 'id' (identificación delestudiante)
  doc2["data"] = data; //Se almacena los datos censados de tipo string en formato Json 
  doc2["checksum"] = String(texto); //Se almacena el checksum (Sha1()) encriptado con XOR
  serializeJson(doc2, data2); //Se serializa el Json 'doc2' y se almacena en 'data2' como String

  //Conexión segura (https) con las librerias WiFiMulti y WiFiClientSecure
  if ((WiFiMulti.run() == WL_CONNECTED)) {
  
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure); //Se declara el objeto que maneja la conexión WiFi con el servidor
  
      client->setFingerprint(fingerprint); //Huella digital del certificado SSL para generar una conexión segura
  
      HTTPClient https; //Se declara un cliente https
  
      Serial.print("[HTTPS] begin...\n");
      https.begin(*client, SERVER_IP); //Se inicia la conexión https con el servidor en AWS
      https.addHeader("Content-Type", "application/json"); //Cabecera de la trama HTTPS
  
      Serial.print("[HTTPS] POST...\n");
      //  Start connection and send HTTP header
      int httpCode = https.POST(data2); //Se realiza una petición POST con los datos de la variable data2
      //  HttpCode será negativo en caso de error
      if (httpCode > 0) {
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end(); //Se finaliza la petición/conexión
    }
}
