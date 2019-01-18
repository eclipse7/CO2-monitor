#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include "images.h"

#define MH_Z19_RX 6
#define MH_Z19_TX 5
#define sclk 13
#define mosi 11
#define cs 10
#define rst 9
#define dc 8

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

// command to ask for data
byte askco2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte max1k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x03, 0xE8, 0x7B};
byte max2k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
byte max3k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x0B, 0xB8, 0xA3};
byte max5k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19
Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);

int height = 14;
int width = 0;
int x_offset = 0;
int y_offset = 42;
int prev_digits = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  co2Serial.begin(9600);
  delay(500);
  co2Serial.write(max5k, 9);
  delay(500);
  co2Serial.write(askco2, 9);
  Serial.println("First ask to co2Serial");

  // display
  display.begin(); // инициализация дисплея
  drawLogo();

  Serial.println("Heating");
  delay(120000);
  Serial.println("GO!");
  readCO2();
  readCO2();

  drawStatic();
}

int bat_value;
void loop() {
  int ppm = readCO2();

  bat_value += 1;
  if (bat_value > 11) bat_value = 0;
  
  reDrawValue(ppm);
  reDrawBar(ppm);
  fillBatteryIcon(bat_value);
  drawChargeIcon(bat_value%2);
    
  Serial.println("===================================================");
  Serial.print("CO2: ");
  Serial.print(ppm);
  Serial.println(" ppm");
  delay(5000);
}

void drawLogo(void) {
  display.fillScreen(BLACK);
  display.drawBitmap(16, 5, mops1, 64, 54, BLACK);
  display.drawBitmap(16, 5, mops4, 64, 54, PINK);
  display.drawBitmap(16, 5, mops5, 64, 54, WHITE);
  display.drawBitmap(16, 5, mops2, 64, 54, BROWN);
  display.drawBitmap(16, 5, mops3, 64, 54, YELLOW);
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
  if (prev_digits != digits){
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
  display.fillRect(81+value, 1, 11-value, 5, BLACK);
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
