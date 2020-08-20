#include <Servo.h>
#include <LoRa.h>

const int SERVO_PIN = 9;
const int MOTOR_DI_PIN = 5;
const int MOTOR_PWR_PIN = 6;

//LoRa
byte channel = 48;

//Servo
Servo microServo;


void setup()
{
  Serial.begin(115200);   // 시리얼 초기화

  // LoRa 초기화
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Starting LoRa...");
  if (!LoRa.begin(915E6)) //한국 주파수 
  {
    Serial.println("Failed!");
  }
  Serial.println("Success!");

  // 서보모터 초기화
  microServo.attach(SERVO_PIN);
}

void loop() {
    servoAngle = microServo.read();

    
}
