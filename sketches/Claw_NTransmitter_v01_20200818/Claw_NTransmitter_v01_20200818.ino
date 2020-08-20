#include <LoRa.h>

const int BUTTON_PIN = 7;
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

void loop()
{
  byte reading = digitalRead(BUTTON_PIN);  //버튼 눌렸는지 검사
  
  byte packet[6] = {0, };
  packet[0] = 0x02;     // STX
  packet[1] = 0x00;     // 제어 코드 예약, 현재는 의미 없음
  packet[2] = channel;  // 채널
  packet[3] = reading;  // 버튼 데이터
  packet[4] = 100;      // 체크섬
  packet[5] = 0x03;     // ETX
  
  Serial.print("Sending: [chan: ");
  Serial.print(packet[2]);
  Serial.print(", data: ");
  Serial.print(packet[3]);
  Serial.print(", checksum: ");
  Serial.print(packet[4]);
  Serial.print("]...");
  
  LoRa.beginPacket();
  LoRa.write(packet, sizeof(packet));
  int success = LoRa.endPacket();

  if(success == 1) {
    Serial.println("Ok");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.println("Failed");
    digitalWrite(LED_BUILTIN, LOW);
  }
  delay(1);
}
