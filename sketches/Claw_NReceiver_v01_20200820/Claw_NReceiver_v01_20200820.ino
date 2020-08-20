#include <Servo.h>
#include <LoRa.h>

const long RECEIVE_WAITING_TIME = 1000;
const int SERVO_PIN = 9;
const int MOTOR_DI_PIN = 5;
const int MOTOR_PWR_PIN = 6;
const int CLAW_ANGLE_START = 150;
const int CLAW_ANGLE_GRAB = 175;
const int CLAW_ANGLE_FREE = 120;
enum ClawStatus {
  Idle,
  Grab,
  Free
};

//LoRa
byte channel = 48;
bool listeningAvailable = false;  //유효 데이터 수신여부
long listeningStartTime = 0;      //데이터 수신 대기시간 측정
byte packet[5] = {0, };
int packetCount = 0;

//Servo
Servo microServo;
int servoAngle;
ClawStatus clawStatus = Idle;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT); // LED
  Serial.begin(115200);   // 시리얼 초기화

  // LoRa 초기화
  Serial.print("Starting LoRa...");
  if (!LoRa.begin(915E6)) //한국 주파수 
  {
    Serial.println("Failed!");
  }
  Serial.println("Success!");
  deactivatePacket();

  // 서보모터 초기화
  microServo.attach(SERVO_PIN);
  pinMode(MOTOR_DI_PIN, OUTPUT);
  pinMode(MOTOR_PWR_PIN, OUTPUT);

  microServo.write(CLAW_ANGLE_START);
}

void loop() {
  servoAngle = microServo.read();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      byte data = LoRa.read();
      Serial.print("Receiving...");   //데이터가 loop마다 나뉘는지 여부 확인
      switch(data) {
        case 0x02:  //데이터 수신 시작
          activatePacket();
          break;
        case 0x03:
          checkAwake();
          deactivatePacket();
          break;
        default:
          break;
      }
      
      if(listeningAvailable && packetCount < 5) {  // STX보면 일단 데이터 넣고 봄
        packet[packetCount] = data;
        packetCount++;
      }
    }
  }

  //서보 제어
  switch(clawStatus) {
    
  }
  //서보 제어 끝
  
  if(listeningAvailable) {
    if(millis() - listeningStartTime > RECEIVE_WAITING_TIME) {
      deactivatePacket();
      Serial.println("Comm Err: Receiving data timed out");
    }
  }
  delay(1);
}

void checkAwake() {
  Serial.print("Data: [");
  for(int i = 0; i < 5; i++) {
    Serial.print(packet[i]);
    Serial.print(", ");
  }
  Serial.print("]...");

  //clawStatus가 Idle이 아닌 경우에만...
  //channel 매칭 여부
  //data가 1인지 여부
}


void activatePacket() {
  listeningAvailable = true;
  listeningStartTime = millis();
  packetCount = 0;
}

void deactivatePacket() {
  listeningAvailable = false;
  listeningStartTime = 0;
  Serial.println();
}
