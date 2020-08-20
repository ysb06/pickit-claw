#include <LoRa.h>

byte channel = 48;

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Starting LoRa...");
  if (!LoRa.begin(915E6)) //한국 주파수 
  {
    Serial.println("Failed!");
  }
  Serial.println("Success!");
}

void loop() {


}
