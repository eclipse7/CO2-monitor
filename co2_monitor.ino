#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>

#define MH_Z19_RX 6
#define MH_Z19_TX 5

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define sclk 13
#define mosi 11
#define cs 10
#define rst 9
#define dc 8

// command to ask for data
byte askco2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte max1k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x03, 0xE8, 0x7B};
byte max2k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
byte max3k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x0B, 0xB8, 0xA3};
byte max5k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19
Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);

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
  display.fillScreen(BLACK); // заливка черным цветом
  lcdTestPattern();

  readCO2();
  readCO2();
  Serial.println("Heating");
  delay(120000);
  Serial.println("GO!");
}


void loop() {
  // put your main code here, to run repeatedly:
  int ppm = readCO2();
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextColor(YELLOW);
  display.setTextSize(2);
  display.print(ppm);

  Serial.println("===================================================");
  Serial.print("CO2: ");
  Serial.print(ppm);
  Serial.println(" ppm");
  delay(5000);
}

void lcdTestPattern(void) {
  uint32_t i, j;
//  display.drawLine(0, 0, 20, 20, YELLOW);
  display.setCursor(0, 0);
  display.setTextColor(YELLOW);
  display.setTextSize(2);
  display.print("Heating");
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
