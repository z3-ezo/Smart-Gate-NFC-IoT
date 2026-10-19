#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- بيانات الشبكة ---
const char* ssid = "Amjad Salim";
const char* password = "3qazZAQ3";

// --- بيانات تلجرام ---
String botToken = "*****************************************"; 
String chatID   = "561135817";  // Chat ID حقك

// --- PIN تعريفات ---
#define SS_PIN    5
#define RST_PIN   22
#define LED_PIN   13
#define BUZZER_PIN 16
#define IR_PIN    14
#define SERVO_PIN 21

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo gateServo;

// --- UID البطاقة المسموح بها ---
byte allowedUID[4] = {0x03, 0x44, 0xA2, 0x15}; 

// --- إعداد ---
void setup() {
  Serial.begin(115200);


  mfrc522.PCD_Init();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  WiFi.begin(ssid, password);
  Serial.print("الاتصال بالشبكة...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("✅ متصل بالواي فاي");
}

// --- دوال مساعدة ---
bool checkUID(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] != allowedUID[i]) return false;
  }
  return true;
}

void openGate() {
  gateServo.write(90);
  digitalWrite(LED_PIN, HIGH);
  delay(5000);
  gateServo.write(0);
  digitalWrite(LED_PIN, LOW);
}

void denyAccess() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, 1);
    delay(200);
    digitalWrite(BUZZER_PIN, 0);
    delay(200);
  }
}

// --- دالة ترميز النصوص (URL Encode) ---
String urlEncode(String str) {
  String encoded = "";
  char c;
  char buf[4];
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

// --- دالة إرسال إشعار تلجرام ---
void sendTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + urlEncode(message);
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.println("❌ فشل إرسال إشعار تلجرام, الكود: " + String(httpCode));
    }
    http.end();
  }
}

// --- الحلقة الرئيسية ---
void loop() {
  if (digitalRead(IR_PIN) == LOW) {
    Serial.println("🚨 جسم أمام البوابة بدون بطاقة!");
    denyAccess();
    sendTelegram("🚨 محاولة دخول بدون بطاقة!");
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidStr += String(mfrc522.uid.uidByte[i], HEX);
  }

  if (checkUID(mfrc522.uid.uidByte)) {
    Serial.println("✅ بطاقة صحيحة - فتح البوابة");
    openGate();
    sendTelegram("✅ بطاقة صحيحة - UID: " + uidStr);
  } else {
    Serial.println("❌ بطاقة غير صحيحة");
    denyAccess();
    sendTelegram("❌ بطاقة خاطئة - UID: " + uidStr);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
