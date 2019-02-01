#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include "images.h"
#include "LowPower.h"

#define MH_Z19_RX 6
#define MH_Z19_TX 5
#define sclk 13
#define mosi 11
#define rst 15  // A1 = 15
#define dc 16  // A2 = 16
#define cs 17  // A3 = 17
#define charge_status 18
#define lcd_power 19
#define co2_power 3
#define red_led 9
#define green_led 10
#define button 2

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GRAY 0x7BEF
#define DARK_BLUE 0x351
#define BROWN 0x3145
#define PINK 0xF355

#define STATUS_WORK 0
#define STATUS_WORK_PRESS_KEY 1
#define STATUS_PW_OFF 2
#define STATUS_PW_ON 3

#define MAIN_INIT 0
#define MAIN_HEATING 1
#define MAIN_WORK 2

#define LED_OFF 0
#define LED_GREEN 1
#define LED_YELLOW 2
#define LED_RED 3

// command to ask for data
byte askco2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte max2k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
byte max5k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19
//Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);  // software spi
Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);  // hardware spi

int height = 14;
int width = 0;
int x_offset = 0;
int y_offset = 42;
int prev_digits = 0;

volatile int button_status = 0;
volatile unsigned long time_hold = 0;

void setup() {
  Serial.begin(115200);

  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(co2_power, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  //  pinMode(A5, OUTPUT);

  led(LED_GREEN);

  digitalWrite(co2_power, LOW);
  //  digitalWrite(A5, HIGH);

  pinMode(A4, INPUT_PULLUP);

  display.begin(); // инициализация дисплея
  //  drawLogoMops();
  drawLogoPica();
  // loading bar
  display.drawRect(0, 60, 96, 4, YELLOW);
}

int status_w = STATUS_WORK;
int statusMain = MAIN_INIT;

int logo = 0;
boolean buttonPress = 0;
int barI = 0;
int barX = 1;
unsigned long heatingTimer = 0;
unsigned long buttonTimer = 0;
unsigned long updateTimer = 10000;
unsigned long checkBatTimer = 10000;
boolean red_blink = 0;
boolean red_blink_state = 0;
boolean showDisplay = 1;

void loop() {
  switch (status_w) {
    case STATUS_WORK:
      if (!digitalRead(button)) {
        status_w = STATUS_WORK_PRESS_KEY;
        buttonTimer = millis();
      }
      else {
        switch (statusMain) {
          case MAIN_INIT:

            digitalWrite(co2_power, LOW);
            delay(500);
            co2Serial.begin(9600);
            delay(500);
            co2Serial.write(max5k, 9);
            delay(500);
            co2Serial.write(askco2, 9);
            Serial.println("First ask to co2Serial");

            barI = 0;
            barX = 1;
            heatingTimer = 0;
            statusMain = MAIN_HEATING;
            break;

          case MAIN_HEATING:
            if (buttonPress) {
              logo += 1;
              logo %= 2;
              if (logo == 0) drawLogoPica();
              if (logo == 1) drawLogoMops();
              display.drawRect(0, 60, 96, 4, YELLOW);
              display.drawRect(1, 61, barX, 2, YELLOW);
              buttonPress = 0;
            }
            if ((millis() - heatingTimer) >= 2700) {
              heatingTimer = millis();
              display.drawRect(barX, 61, 2, 2, YELLOW);
              barX += 2;
              barI += 1;

              if (barI >= 47) {
                Serial.println("GO!");
                readCO2();
                readCO2();
                drawStatic();
                statusMain = MAIN_WORK;
              }
            }
            break;

          case MAIN_WORK:
            // on/off display
            if (buttonPress)  {
              if (showDisplay) {
                display.fillScreen(BLACK);
                showDisplay = 0;
              }
              else {
                drawStatic();
                updateTimer = 0;
                checkBatTimer = 0;
                showDisplay = 1;
              }
              buttonPress = 0;
            }

            if ((millis() - updateTimer) >= 5000) {
              readAndUpdate();
              updateTimer = millis();
            }

            if ((millis() - checkBatTimer) >= 1000) {
              if (showDisplay) {
                checkBatAndCharge();
              }
              if (red_blink) {
                if (red_blink_state) {
                  led(LED_OFF);
                  red_blink_state = 0;
                }
                else {
                  led(LED_RED);
                  red_blink_state = 1;
                }
              }
              checkBatTimer = millis();
            }

            break;
        }
      }
      break;

    case STATUS_WORK_PRESS_KEY:
      if (!digitalRead(button)) {
        if ((millis() - buttonTimer) >= 275) {
          // do long press
          status_w = STATUS_PW_OFF;
        }
      }
      else {
        if ((millis() - buttonTimer) >= 30) {
          // do short press
          buttonPress = 1;
          status_w = STATUS_WORK;
        }
        else {
          status_w = STATUS_WORK;
        }
        buttonTimer = millis();
      }
      break;

    case STATUS_PW_OFF:
      Serial.println("poweroff");
      display.fillScreen(BLACK);
      led(LED_OFF);
      digitalWrite(red_led, LOW);
      digitalWrite(green_led, LOW);
      digitalWrite(co2_power, HIGH);

      digitalWrite(sclk, LOW);
      digitalWrite(mosi, LOW);
      digitalWrite(rst, LOW);
      digitalWrite(dc, LOW);
      digitalWrite(cs, LOW);

      digitalWrite(MH_Z19_RX, LOW);
      digitalWrite(MH_Z19_TX, LOW);

      statusMain = MAIN_INIT;
      logo = 0;
      showDisplay = 1;
      while (!digitalRead(button)) delay(10);

      Serial.println("Going to sleep");
      delay(100);
      attachInterrupt(0, wakeUp, LOW);
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);  // sleep
      detachInterrupt(0);

      Serial.println("Wake up");
      buttonTimer = millis();
      status_w = STATUS_PW_ON;
      break;

    case STATUS_PW_ON:
      if (!digitalRead(button)) {
        if ((millis() - buttonTimer) >= 225) {
          Serial.println("poweron");
          //pinMode(A5, OUTPUT);
          //digitalWrite(A5, HIGH);
          display.begin(); // инициализация дисплея
          //  drawLogoMops();
          drawLogoPica();
          // loading bar
          display.drawRect(0, 60, 96, 4, YELLOW);
          while (!digitalRead(button)) delay(10);

          status_w = STATUS_WORK;
        }
      }
      else {
        status_w = STATUS_PW_OFF;
        buttonTimer = millis();
      }
      break;
  }
  delay(20);
}

void wakeUp() {
  // handler for interrupt
}

void led(int color) {
  switch (color) {
    case LED_OFF:
      analogWrite(red_led, 0);
      analogWrite(green_led, 0);
      break;

    case LED_GREEN:
      analogWrite(red_led, 0);
      analogWrite(green_led, 255);
      break;
    case LED_YELLOW:
      analogWrite(red_led, 100);
      analogWrite(green_led, 255);
      break;

    case LED_RED:
      analogWrite(red_led, 255);
      analogWrite(green_led, 0);
      break;
  }
}

void readAndUpdate() {
  int ppm = readCO2();
  if (showDisplay) {
    reDrawValue(ppm);
    reDrawBar(ppm);
  }
  red_blink = 0;
  if (ppm < 800) {
    led(LED_GREEN);
  } else {
    if (ppm < 1200) {
      led(LED_YELLOW);
    } else {
      if (ppm < 2000) {
        led(LED_RED);
      }
      else {
        red_blink = 1;
      }
    }
  }
  Serial.println("===================================================");
  Serial.print("CO2: ");
  Serial.print(ppm);
  Serial.println(" ppm");
}

void checkBatAndCharge() {
  pinMode(A4, INPUT_PULLUP);
  int val = analogRead(A4);
  pinMode(A4, INPUT);
  if (val < 200) drawChargeIcon(1); // val < 181 - charging,  val = 393 - no charging, val > 450 - charging stop
  else drawChargeIcon(0);

  int bat_value = int((readVcc() - 3200) / 800.0 * 11.0);
  fillBatteryIcon(bat_value);
}

long readVcc() {
  long result; // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

void drawLogoMops(void) {
  display.fillScreen(BLACK);
  display.drawBitmap(16, 4, mops1, 64, 54, BLACK);
  display.drawBitmap(16, 4, mops4, 64, 54, PINK);
  display.drawBitmap(16, 4, mops5, 64, 54, WHITE);
  display.drawBitmap(16, 4, mops2, 64, 54, BROWN);
  display.drawBitmap(16, 4, mops3, 64, 54, YELLOW);
}

void drawLogoPica(void) {
  display.fillScreen(BLACK);
  display.drawBitmap(27, 11, pica1, 44, 37, BLACK);
  display.drawBitmap(27, 11, pica_yellow, 44, 37, YELLOW);
  display.drawBitmap(27, 11, pica_white, 44, 37, WHITE);
  display.drawBitmap(27, 11, pica_red, 44, 37, RED);
  display.drawBitmap(27, 11, pica_green, 44, 37, GREEN);
  display.drawBitmap(27, 11, pica_gray, 44, 37, GRAY);
}

void drawStatic(void) {
  // CO2 text
  display.fillScreen(BLACK);
  display.setCursor(0, 0);
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.print("CO");
  display.setCursor(12, 2);
  display.setTextColor(CYAN);
  display.setTextSize(1);
  display.print("2");

  // battery
  display.drawRect(80, 0, 13, 7, CYAN);
  display.drawLine(93, 2, 93, 4, CYAN);

  // bar
  display.drawRoundRect(13, y_offset, 71, height, 4, GRAY);
  // fill green part
  display.fillRoundRect(14, y_offset + 1, 23, height - 2, 3, GREEN);
  display.fillRect(16, y_offset + 1, 21, height - 2, GREEN);
}

void reDrawValue(int value) {
  int digits = 3;
  if (value >= 1000) digits = 4;
  if (prev_digits != digits) {
    display.fillRect(10, 15, 75, 15, BLACK);
    prev_digits = digits;
  }

  // 'ppm' text
  if (digits == 4) display.setCursor(65, 23);  // for 4 digit ppm
  else display.setCursor(61, 23);  // for 3 digit ppm
  display.setTextColor(YELLOW, BLACK);
  display.setTextSize(1);
  display.print("ppm");

  // ppm value
  if (digits == 4) display.setCursor(16, 16);  // for 4 digit
  else display.setCursor(24, 16);  // for 3 digit
  display.setTextColor(YELLOW, BLACK);
  display.setTextSize(2);
  display.print(value);

}

void reDrawBar(int value) {
  if (value >= 1200) {
    // yellow on
    display.fillRect(37, y_offset + 1, 23, height - 2, YELLOW);
    // red on
    display.fillRect(60, y_offset + 1, 6, height - 2, RED);
    display.fillRoundRect(60, y_offset + 1, 23, height - 2, 3, RED);
  }
  if ((value >= 800) && (value < 1200)) {
    // yellow on
    display.fillRect(37, y_offset + 1, 23, height - 2, YELLOW);
    // red off
    display.fillRect(60, y_offset + 1, 6, height - 2, BLACK);
    display.fillRoundRect(60, y_offset + 1, 23, height - 2, 3, BLACK);
  }
  if (value < 800) {
    // yellow off
    display.fillRect(37, y_offset + 1, 23, height - 2, BLACK);
    // red off
    display.fillRect(60, y_offset + 1, 6, height - 2, BLACK);
    display.fillRoundRect(60, y_offset + 1, 23, height - 2, 3, BLACK);
  }
}

void drawChargeIcon(int on) {
  // charge icon
  if (on) {
    display.drawLine(75, 0, 73, 3, YELLOW);
    display.drawLine(73, 3, 76, 3, YELLOW);
    display.drawLine(76, 3, 74, 6, YELLOW);
  }
  else {
    display.drawLine(75, 0, 73, 3, BLACK);
    display.drawLine(73, 3, 76, 3, BLACK);
    display.drawLine(76, 3, 74, 6, BLACK);
  }
}

void fillBatteryIcon(int value) {
  // battery
  if (value > 11) value = 11;
  if (value < 0) value = 0;
  display.fillRect(81, 1, value, 5, CYAN);
  display.fillRect(81 + value, 1, 11 - value, 5, BLACK);
}

int readCO2() {
  byte response[9];
  co2Serial.write(askco2, 9);
  // The serial stream can get out of sync. The response starts with 0xff, try to resync.
  while (co2Serial.available() > 0 && (unsigned char)co2Serial.peek() != 0xFF) {
    co2Serial.read();
  }

  memset(response, 0, 9);
  co2Serial.readBytes(response, 9);

  // print response
  Serial.println(" ");
  Serial.println("response");
  for (int i = 0; i < 9; i++) {
    int r = (int) response[i];
    Serial.print(r);
    Serial.print(" ");
  }
  Serial.println(" ");

  if (response[0] != 0xFF) {
    Serial.print("\n\rWrong starting byte from co2 sensor!");
    Serial.println("response");
    for (int i = 0; i < 9; i++) {
      int r = (int) response[i];
      Serial.print(r);
      Serial.print(" ");
    }
    Serial.println(" ");
    return -1;
  }

  if (response[1] != 0x86) {
    Serial.print("\n\rWrong command from co2 sensor!");
    Serial.println("response");
    for (int i = 0; i < 9; i++) {
      int r = (int) response[i];
      Serial.print(r);
      Serial.print(" ");
    }
    Serial.println(" ");
    return -1;
  }

  byte crc = 0;
  for (int i = 1; i < 8; i++) {
    crc += response[i];
  }
  crc = 255 - crc + 1;

  if (response[8] == crc) {
    int responseHigh = (int) response[2];
    int responseLow = (int) response[3];
    int ppm = (256 * responseHigh) + responseLow;
    return ppm;
  } else {
    Serial.println("CRC error!");
    return -1;
  }
}
