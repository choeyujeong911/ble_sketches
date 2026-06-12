#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SVC_UUID      "a0b82d8f-d5a9-42ae-93dc-698df59ea89b"
#define BTN_CHA_UUID  "5cb3ff02-3a98-4d6e-a366-d29c3d0249cc"
#define LED_CHA_UUID  "32c86189-2864-4606-8010-8a267c12c9d8"

#define BTN_PIN  34
#define LED_PIN  23

BLEServer* server;
BLECharacteristic* btnCharacteristic;
BLECharacteristic* ledCharacteristic;

int lastButtonState = -1;
bool wasConnected = false;

// 앱에서 값을 WRITE했을 때 실행됨
class LedWriteCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) {
    String value = characteristic->getValue();

    if (value.length() > 0) {
      Serial.print("받은 값: ");
      Serial.println(value);

      if (value == "1") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED_PIN: HIGH");
      } else if (value == "0") {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED_PIN: LOW");
      } else {
        Serial.println("알 수 없는 값");
      }
    }
  }
};

void setup() {
  Serial.begin(9600);

  pinMode(BTN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("START");

  BLEDevice::init("ESP32_BleTest");

  server = BLEDevice::createServer();

  BLEService* bleService = server->createService(SVC_UUID);

  // 기존: ESP32 → 앱 송신용 READ + NOTIFY
  btnCharacteristic = bleService->createCharacteristic(
    BTN_CHA_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  btnCharacteristic->addDescriptor(new BLE2902());
  btnCharacteristic->setValue("OFF");

  // 추가: 앱 → ESP32 수신용 WRITE
  ledCharacteristic = bleService->createCharacteristic(
    LED_CHA_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  ledCharacteristic->setCallbacks(new LedWriteCallback());
  ledCharacteristic->setValue("0");

  bleService->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SVC_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("스위치 테스트 BLE 서비스 준비 완료");
}

void loop() {
  bool connected = server->getConnectedCount() > 0;

  if (connected) {
    int currentButtonState = digitalRead(BTN_PIN);

    if (currentButtonState != lastButtonState) {
      if (currentButtonState == HIGH) {
        btnCharacteristic->setValue("ON");
        btnCharacteristic->notify();

        Serial.println("Switch: ON");
      } else {
        btnCharacteristic->setValue("OFF");
        btnCharacteristic->notify();

        Serial.println("Switch: OFF");
      }

      lastButtonState = currentButtonState;
    }
  }

  if (!connected && wasConnected) {
    Serial.println("연결 끊김");
    lastButtonState = -1;

    BLEDevice::startAdvertising();
  }

  wasConnected = connected;

  delay(50);
}