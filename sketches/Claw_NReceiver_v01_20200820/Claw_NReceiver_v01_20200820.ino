#include <Servo.h>
#include <LoRa.h>

const long RECEIVE_WAITING_TIME = 1000;
const int CHANNEL_PACKET_POSITION = 2;
const int BUTTON_PACKET_POSITION = 3;
const int SERVO_PIN = 9;
const int MOTOR_DI_PIN = 5;
const int MOTOR_PWM_PIN = 6;
const int CLAW_ANGLE_START = 150;
const int CLAW_ANGLE_GRAB = 175;
const int CLAW_ANGLE_FREE = 120;
enum ClawState {
  Idle,           //평상시
  Grab,           //집게 내리는 중
  GrabFinished,   //집게 내리기 완료
  Free,           //집게로 잡고 올리는 중
  FreeFinished    //집게 올리기 완료
};

//LoRa
byte channelNum = 48;
bool listeningAvailable = false;  //유효 데이터 수신여부
long listeningStartTime = 0;      //데이터 수신 대기시간 측정
byte packet[6] = {0, };
int packetCount = 0;

//Servo
Servo microServo;
int servoAngle;
ClawState clawState = Idle;
long actionStartTime = 0;
bool isActionStart = false;


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
  pinMode(MOTOR_PWM_PIN, OUTPUT);

  microServo.write(CLAW_ANGLE_START);
}

void loop() {
  servoAngle = microServo.read();

  //loop안의 아래 코드는 LoRa 모듈이 데이터를 완전히 받았는지 여부 체크 후 switchState호출
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Receiving...");   //데이터가 loop마다 나뉘는지 여부 확인
    while (LoRa.available()) {
      byte raw = LoRa.read();
      Serial.print(raw);
      Serial.print(' ');
      switch (raw) {
        case 0x02:  // STX
          activatePacket(); //데이터 수신 시작
          break;
        case 0x03:  // ETX
          Serial.println();
          if (listeningAvailable) {
            packet[packetCount] = raw;  //마지막 ETX까지 배열에 추가
            packetCount++;
            processButtonInput();   //버튼에 따른 상태 업데이트
          }
          deactivatePacket();   //데이터 수신 종료
          break;
        default:
          break;
      }

      if (listeningAvailable && packetCount < sizeof(packet)) { // STX보면 일단 데이터 넣고 봄
        packet[packetCount] = raw;
        packetCount++;
      }
    }
    Serial.print("Result: [valid: ");
    Serial.print(listeningAvailable);
    Serial.print(", timeVal: ");
    Serial.print(millis() - actionStartTime);
    Serial.print(", State: ");
    Serial.print(clawState);
    Serial.print(", angle: ");
    Serial.print(servoAngle);
    Serial.println("]");
    Serial.println("-----");
  }

  //서보 제어
  switch (clawState) {
    case Grab:
      grabClaw();
      break;
    case Free:
      freeClaw();
      break;
    case GrabFinished:    // 마지막 단계
      clawState = Idle;   // FreeFinished가 의미가 없음.
      break;
  }
  //서보 제어 끝

  if (listeningAvailable) {
    if (millis() - listeningStartTime > RECEIVE_WAITING_TIME) {
      deactivatePacket();
      Serial.println("Comm Err: Receiving data timed out");
    }
  }
  delay(1);
}

///----- 데이터 수신 -----///

/// <summary>
/// 패킷 수신 활성화 및 변수 초기화
/// </summary>
void activatePacket() {
  listeningAvailable = true;
  listeningStartTime = millis();
  packetCount = 0;
}

/// <summary>
/// 패킷 수신을 중지
/// </summary>
void deactivatePacket() {
  listeningAvailable = false;
  listeningStartTime = 0;
}


///----- 버튼 데이터 처리 -----///

/// <summary>
/// 버튼 상태에 따라 State 스위칭 (Idle --> Grab, GrabFinished --> Free)
/// </summary>
void processButtonInput() {
  bool isDataValid = calcChecksum(packet);

  if (isDataValid &&
      packet[CHANNEL_PACKET_POSITION] == channelNum &&
      packet[BUTTON_PACKET_POSITION] == 1) {
    switch (clawState) {
      case Idle:
        switchState(Free);   //Grab으로 상태 변경
        break;
      case FreeFinished:
        switchState(Grab);   //Free로 상태 변경
        break;
      default:
        break;
    }
  }
}

/// <summary>
/// 패킷 수신을 위한 변수 초기화
/// </summary>
bool calcChecksum(byte target[]) {
  Serial.print("Data: [");
  for (int i = 0; i < sizeof(packet); i++) {
    Serial.print(packet[i]);
    Serial.print(", ");
  }
  Serial.println("]...");

  //체크섬 알고리즘
  return true;
}

void switchState(ClawState state) {
  clawState = state;    //잡기
  isActionStart = false;
}

///----- 서보 모터 제어 -----///
/// <summary>
/// 현재 State가 Grab일 때 수행하는 동작. Claw 내려서 잡기
/// </summary>
void grabClaw() {
  long currentTime = millis() - actionStartTime;
  if (isActionStart == false) {
    microServo.write(CLAW_ANGLE_GRAB); //놓기
    actionStartTime = millis();   // 동작 수행
    isActionStart = true;
  } else {
    if (currentTime > 1000 && currentTime <= 6100) {
      analogWrite(MOTOR_DI_PIN, 255);   //감아 올리기
    } else if (currentTime > 6100) {
      analogWrite(MOTOR_DI_PIN, 0);       //감아 올리기 중지
      switchState(GrabFinished);       //Grab 끝난 상태로 변경
    }
  }
}

void freeClaw() {
  long currentTime = millis() - actionStartTime;
  if (isActionStart == false) {
    microServo.write(CLAW_ANGLE_FREE); //놓기
    actionStartTime = millis();   // 동작 수행
    isActionStart = true;
  } else {
    if (currentTime > 0 && currentTime <= 3800) {
      analogWrite(MOTOR_PWM_PIN, 255);   //풀어 내리기
    } else if (currentTime > 3800) {
      analogWrite(MOTOR_PWM_PIN, 0);    //풀어 내리기 중지
      switchState(FreeFinished);     //Free끝난 상태로 변경
    }
  }
}
