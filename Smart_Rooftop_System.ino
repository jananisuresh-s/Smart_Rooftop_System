#include <BluetoothSerial.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// Bluetooth
BluetoothSerial SerialBT;

// DHT Sensor
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// LCD Display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Rain Sensor
#define RAIN_SENSOR_PIN 34

// Limit Switch (LOW = Roof Closed)
#define LIMIT_SWITCH_PIN 35

// Motor Relays (Active LOW)
#define MOTOR_FORWARD_PIN 5   // Close roof
#define MOTOR_BACKWARD_PIN 18 // Open roof

// Timing
const unsigned long TOTAL_MOVE_TIME = 28500; // Full open/close time (ms)
unsigned long motorStartTime = 0;

// State variables
bool isMotorRunning = false;
bool motorDirection = false; // false=open, true=close

enum MotorState {
  STOPPED,
  CLOSING,
  OPENING
};
MotorState currentMotorState = STOPPED;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SmartRoofController");
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Roof System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(LIMIT_SWITCH_PIN, INPUT); // GND when active
  pinMode(MOTOR_FORWARD_PIN, OUTPUT);
  pinMode(MOTOR_BACKWARD_PIN, OUTPUT);

  stopMotor();

  delay(2000);
  lcd.clear();
  Serial.println("Setup complete. Ready!");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int rainVal = analogRead(RAIN_SENSOR_PIN);
  bool isRaining = (rainVal < 2000); // Adjust threshold
  bool roofClosed = (digitalRead(LIMIT_SWITCH_PIN) == LOW);

  handleBluetoothCommands();

  // Auto-close if rain or hot
  if (!isMotorRunning) {
    if ((isRaining || (!isnan(temp) && temp > 35)) && !roofClosed) {
      Serial.println("Auto: Closing roof due to rain/temp");
      closeRoofFully();
    }
  }

  // Stop motor if closed
  if (isMotorRunning && currentMotorState == CLOSING && roofClosed) {
    Serial.println("Limit switch triggered, roof closed.");
    stopMotor();
  }

  updateDisplay(temp, hum, rainVal, isRaining, roofClosed);

  delay(500);
}

// ===================== CONTROL LOGIC =====================

void closeRoofFully() {
  if (!isMotorRunning && digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
    Serial.println("Closing roof fully...");
    setMotorDirection(true);
    unsigned long start = millis();
    while (millis() - start < TOTAL_MOVE_TIME && digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
      delay(10);
    }
    stopMotor();
  } else {
    Serial.println("Roof already closed.");
  }
}

void openRoofFully() {
  if (!isMotorRunning) {
    Serial.println("Opening roof fully...");
    setMotorDirection(false);
    unsigned long start = millis();
    while (millis() - start < TOTAL_MOVE_TIME) {
      delay(10);
    }
    stopMotor();
  }
}

void closeRoofPercentage(int percentage) {
  if (!isMotorRunning && digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
    unsigned long runTime = (TOTAL_MOVE_TIME * percentage) / 100;
    Serial.println("Closing roof " + String(percentage) + "%");
    setMotorDirection(true);
    unsigned long start = millis();
    while (millis() - start < runTime && digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
      delay(10);
    }
    stopMotor();
  } else {
    Serial.println("Roof already closed.");
  }
}

void openRoofPercentage(int percentage) {
  if (!isMotorRunning) {
    unsigned long runTime = (TOTAL_MOVE_TIME * percentage) / 100;
    Serial.println("Opening roof " + String(percentage) + "%");
    setMotorDirection(false);
    delay(runTime);
    stopMotor();
  }
}

void setMotorDirection(bool close) {
  if (close) {
    digitalWrite(MOTOR_FORWARD_PIN, LOW);   // Active LOW
    digitalWrite(MOTOR_BACKWARD_PIN, HIGH);
    currentMotorState = CLOSING;
  } else {
    digitalWrite(MOTOR_FORWARD_PIN, HIGH);
    digitalWrite(MOTOR_BACKWARD_PIN, LOW);  // Active LOW
    currentMotorState = OPENING;
  }
  isMotorRunning = true;
  motorStartTime = millis();
}

void stopMotor() {
  digitalWrite(MOTOR_FORWARD_PIN, HIGH);   // deactivate both
  digitalWrite(MOTOR_BACKWARD_PIN, HIGH);
  isMotorRunning = false;
  currentMotorState = STOPPED;
}

// ===================== BLUETOOTH CONTROL =====================

void handleBluetoothCommands() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readString();
    cmd.trim();
    cmd.toLowerCase();

    Serial.println("BT Command: " + cmd);

    if (cmd == "close" || cmd == "close fully") closeRoofFully();
    else if (cmd == "open" || cmd == "open fully") openRoofFully();
    else if (cmd.startsWith("close")) {
      int pct = extractPercentage(cmd);
      if (pct > 0) closeRoofPercentage(pct);
    }
    else if (cmd.startsWith("open")) {
      int pct = extractPercentage(cmd);
      if (pct > 0) openRoofPercentage(pct);
    }
    else if (cmd == "stop") stopMotor();
    else if (cmd == "status") sendStatus();
    else SerialBT.println("Unknown command: " + cmd);
  }
}

int extractPercentage(String command) {
  String digits = "";
  for (int i = 0; i < command.length(); i++) {
    if (isDigit(command[i])) digits += command[i];
  }
  if (digits.length() > 0) {
    int pct = digits.toInt();
    return constrain(pct, 1, 100);
  }
  return 0;
}

// ===================== DISPLAY & STATUS =====================

void updateDisplay(float temp, float hum, int rainVal, bool isRaining, bool closed) {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (!isnan(temp) && !isnan(hum)) {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print(" H:");
    lcd.print(hum, 0);
  } else {
    lcd.print("Sensor Err");
  }

  lcd.setCursor(0, 1);
  lcd.print(isRaining ? "Rain:Y " : "Rain:N ");
  lcd.print(closed ? "Roof:Cls" : "Roof:Opn");
}

void sendStatus() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int rainVal = analogRead(RAIN_SENSOR_PIN);
  bool isRaining = (rainVal < 2000);
  bool closed = (digitalRead(LIMIT_SWITCH_PIN) == LOW);

  String msg = "=== Roof Status ===\n";
  msg += "Temp: " + String(temp) + "°C\n";
  msg += "Hum: " + String(hum) + "%\n";
  msg += "Rain: " + String(isRaining ? "Yes" : "No") + "\n";
  msg += "Limit Switch: " + String(closed ? "Closed" : "Open") + "\n";
  msg += "Motor: " + String(isMotorRunning ? (currentMotorState == CLOSING ? "Closing" : "Opening") : "Stopped");
  SerialBT.println(msg);
}
