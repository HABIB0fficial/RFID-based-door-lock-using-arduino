#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN 7 // Pin connected to the relay
#define BUZZER_PIN 8 // Pin connected to the buzzer

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool accessGranted = false;
int unsuccessfulAttempts = 0;
bool permanentlyLocked = false;

unsigned long programStartTime = 0; // Variable to store program start time

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Scan for RFID tag");

  Serial.println("Scan for RFID tag to read...");

  programStartTime = millis(); // Record the program start time
}

void loop() {
  if (!permanentlyLocked) {
    scanRFID();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Permanently");
    lcd.setCursor(0, 1);
    lcd.print("Locked");

    if (scanAuthorizedCard()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unlock Authorized");
      accessGranted = true;
      unsuccessfulAttempts = 0;
      permanentlyLocked = false;
    }
  }

  if (accessGranted) {
    digitalWrite(RELAY_PIN, LOW);
    delay(2000);
    digitalWrite(RELAY_PIN, HIGH);
    accessGranted = false;

    printPLXData();
  }

  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan for RFID tag");
}

void scanRFID() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String tagUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      tagUID += String(mfrc522.uid.uidByte[i], HEX);
    }

    if (tagUID.substring(1).equalsIgnoreCase("76 6A D1 29")) {
      Serial.println("Tag matched! Access granted.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Granted");
      accessGranted = true;
      unsuccessfulAttempts = 0;
    } else {
      Serial.println("Tag not matched. Access denied.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Denied");
      accessGranted = false;
      unsuccessfulAttempts++;

      if (unsuccessfulAttempts == 3) {
        lcd.setCursor(0, 1);
        lcd.print("Last Attempt!");
      }

      digitalWrite(BUZZER_PIN, HIGH);
      delay(3000);
      digitalWrite(BUZZER_PIN, LOW);

      if (unsuccessfulAttempts >= 4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Permanently");
        lcd.setCursor(0, 1);
        lcd.print("Locked");
        permanentlyLocked = true;
      }
    }
  }
}

bool scanAuthorizedCard() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String tagUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      tagUID += String(mfrc522.uid.uidByte[i], HEX);
    }

    return tagUID.substring(1).equalsIgnoreCase("0C 0D 34 29");
  }
  return false;
}

void printPLXData() {
  String currentDate = getDate();
  String currentTime = getTime();

  Serial.print(getTagUID());
  Serial.print(",");
  Serial.print(currentDate);
  Serial.print(",");
  Serial.print(currentTime);
  Serial.print(",");
  Serial.println(accessGranted ? "Access Granted" : "Access Denied");
}

String getTagUID() {
  String tagUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    tagUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  return tagUID.substring(1);
}

String getDate() {
  unsigned long elapsedSeconds = (millis() - programStartTime) / 1000;
  int days = elapsedSeconds / 86400;
  int years = days / 365;
  int months = (days % 365) / 30;
  int remainingDays = (days % 365) % 30;

  return String(years) + "/" + String(months) + "/" + String(remainingDays);
}

String getTime() {
  unsigned long elapsedSeconds = (millis() - programStartTime) / 1000;
  int hours = (elapsedSeconds % 86400) / 3600;
  int minutes = ((elapsedSeconds % 86400) % 3600) / 60;
  int seconds = ((elapsedSeconds % 86400) % 3600) % 60;

  return String(hours) + ":" + String(minutes) + ":" + String(seconds);
}
