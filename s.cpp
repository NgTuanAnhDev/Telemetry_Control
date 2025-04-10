#include <Wire.h>
#include "Adafruit_TCS34725.h"

// TCS34725
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// Servo điều khiển bằng PWM
#define SERVO_PIN 18
#define SERVO_CH  0
#define PWM_FREQ  50      // 50Hz
#define PWM_RES   16      // 16-bit PWM

float r_norm, g_norm, b_norm;
String mau = "Khong xac dinh";

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL

  // Khởi động cảm biến
  if (!tcs.begin()) {
    Serial.println("❌ Không tìm thấy TCS34725!");
    while (1);
  }

  // Cấu hình PWM cho servo
  ledcSetup(SERVO_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(SERVO_PIN, SERVO_CH);
  setServoAngle(90); // bắt đầu ở giữa
}

void loop() {
  readColor();

  if (mau == "Vang") {
    setServoAngle(180);
  } else if (mau == "Trang") {
    setServoAngle(0);
  } else {
    setServoAngle(90);
  }

  delay(1000);
}

// ========================
// Hàm quay servo bằng PWM
// ========================
void setServoAngle(int angle) {
  int pulse_us = map(angle, 0, 180, 500, 2500); // micro giây
  int pwm_val = (pulse_us * 65535) / 20000;     // 16-bit
  ledcWrite(SERVO_CH, pwm_val);
}

// ========================
// Hàm đọc và nhận diện màu
// ========================
void readColor() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  if (c == 0) c = 1;

  r_norm = (float)r / c;
  g_norm = (float)g / c;
  b_norm = (float)b / c;

  float total = r_norm + g_norm + b_norm;
  float diff_rg = abs(r_norm - g_norm);
  float diff_rb = abs(r_norm - b_norm);
  float diff_gb = abs(g_norm - b_norm);

  if (c < 2000 || total < 0.85) {
    mau = "Khong xac dinh";
  }
  else if (diff_rg < 0.13 && diff_rb < 0.25 && diff_gb < 0.2 && total > 0.9) {
    mau = "Trang";
  }
  else if (r_norm > 0.42 && g_norm > 0.3 && b_norm < 0.2) {
    mau = "Vang";
  }
  else {
    mau = "Khong xac dinh";
  }

  Serial.print("R: "); Serial.print(r_norm, 3);
  Serial.print(" G: "); Serial.print(g_norm, 3);
  Serial.print(" B: "); Serial.print(b_norm, 3);
  Serial.print(" C: "); Serial.print(c);
  Serial.print(" → Mau: "); Serial.println(mau);
}