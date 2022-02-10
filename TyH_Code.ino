#include <ClosedCube_HDC1080.h>
#include <Wire.h>

ClosedCube_HDC1080 sensor;

void setup() {
  sensor.begin(0x40);
  Serial.begin(9600);
}

void loop() {

  double temperatura = 0;
  double humedad = 0;

  for(int i=0;i<20;i++){
    temperatura = temperatura + sensor.readTemperature();
    humedad = humedad + sensor.readHumidity();
  }
  
  Serial.print("Temperatura = ");
  Serial.print(temperatura/20);
  Serial.print(" °C Humedad ");
  Serial.print(humedad/20);
  Serial.println("%");
  delay(2000);
}
