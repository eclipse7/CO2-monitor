#include <SoftwareSerial.h>

#define MH_Z19_RX 6
#define MH_Z19_TX 5
#define LED 13

// command to ask for data
byte askco2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte max1k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x03, 0xE8, 0x7B};
byte max2k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
byte max3k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x0B, 0xB8, 0xA3};
byte max5k[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19
void setup() {
  Serial.begin(115200);
  delay(500);
  co2Serial.begin(9600);
  delay(500);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  delay(500);
  co2Serial.write(max5k, 9);

  delay(500);
  co2Serial.write(askco2, 9);
  Serial.println("First ask to co2Serial");
  
  readCO2();
  readCO2();
  Serial.println("Heating");
  delay(120000);
  Serial.println("GO!");
}


void loop() {
  // put your main code here, to run repeatedly:
  int ppm = readCO2();
  Serial.println("===================================================");
  Serial.print("CO2: ");
  Serial.print(ppm);
  Serial.println(" ppm");
  delay(5000);
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
