#include <ESP8266WiFi.h>
#include <espnow.h>
#include "InputDebounce.h"

uint8_t senderMac[] = {0xA0, 0x20, 0xA6, 0x17, 0x72, 0xF8};
uint8_t receiverMac[] = {0x5C, 0xCF, 0x7F, 0xC1, 0x30, 0xC3};
bool senderMode = false;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;

int greenPin = 12;
int bluePin = 13;
int redPin = 15;
int buttonPin = 4;

static InputDebounce button;

void OnDataSent(uint8_t *macAddr, uint8_t sendStatus) {
  if (sendStatus == 0){
    Serial.println("Delivery success");
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, LOW);
  }
  else{
    Serial.println("Delivery fail");
    digitalWrite(greenPin, LOW);
    digitalWrite(redPin, HIGH);
  }
}

void OnDataRecv(uint8_t * macAddr, uint8_t *incomingData, uint8_t len) {
  static bool toggle = false;
  Serial.println("Packet received");
  if (toggle) {
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, HIGH);
  } else {
    digitalWrite(greenPin, HIGH);
    digitalWrite(bluePin, LOW);
  }
  toggle = !toggle;
}
 
void setup() {
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin, OUTPUT); 
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);
  digitalWrite(redPin, LOW);

  button.setup(buttonPin, DEFAULT_INPUT_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  Serial.print("MAC:  ");
  Serial.println(WiFi.macAddress());
  uint8_t myMac[6]{};
  WiFi.macAddress(myMac);

  WiFi.disconnect();
  if (esp_now_init() != 0) {
    Serial.println("ESP NOW init failed");
    return;
  }
  
  if (0 == memcmp(myMac, receiverMac, sizeof(myMac))) {
    Serial.println("Receiver mode");
    senderMode = false;
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
  } else {
    Serial.println("Sender mode");
    senderMode = true;
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  }
}


void loop() {
  static unsigned int countTotal = 0;
  if (senderMode) {
    unsigned long now = millis();
    if ((now - lastTime) > timerDelay) {
      lastTime = millis();
    }
    button.process(now);
    unsigned int count = button.getStatePressedCount();
    if(countTotal != count) {
      countTotal = count;
      digitalWrite(bluePin, !digitalRead(bluePin));
      Serial.println("Button pressed, sending...");
      uint8_t test[]{1, 2, 3, 4};
      esp_now_send(0, test, sizeof(test));
    }
  } else {

  }
}
