// Compiled with: Arduino 1.8.13

/*
  Battery Guage setup for BQ34Z100-G1

ISP
---
PD0     RX
PD1     TX
RESET#  through 50M capacitor to RST#

SDcard
------
DAT3   SS   4 B4
CMD    MOSI 5 B5
DAT0   MISO 6 B6
CLK    SCK  7 B7

ANALOG
------
+      A0  PA0
-      A1  PA1
RESET  0   PB0

LED
---
LED_red  23  PC7         // LED for Dasa



                     Mighty 1284p
                      +---\/---+
           (D 0) PB0 1|        |40 PA0 (AI 0 / D24)
           (D 1) PB1 2|        |39 PA1 (AI 1 / D25)
      INT2 (D 2) PB2 3|        |38 PA2 (AI 2 / D26)
       PWM (D 3) PB3 4|        |37 PA3 (AI 3 / D27)
    PWM/SS (D 4) PB4 5|        |36 PA4 (AI 4 / D28)
      MOSI (D 5) PB5 6|        |35 PA5 (AI 5 / D29)
  PWM/MISO (D 6) PB6 7|        |34 PA6 (AI 6 / D30)
   PWM/SCK (D 7) PB7 8|        |33 PA7 (AI 7 / D31)
                 RST 9|        |32 AREF
                VCC 10|        |31 GND
                GND 11|        |30 AVCC
              XTAL2 12|        |29 PC7 (D 23)
              XTAL1 13|        |28 PC6 (D 22)
      RX0 (D 8) PD0 14|        |27 PC5 (D 21) TDI
      TX0 (D 9) PD1 15|        |26 PC4 (D 20) TDO
RX1/INT0 (D 10) PD2 16|        |25 PC3 (D 19) TMS
TX1/INT1 (D 11) PD3 17|        |24 PC2 (D 18) TCK
     PWM (D 12) PD4 18|        |23 PC1 (D 17) SDA
     PWM (D 13) PD5 19|        |22 PC0 (D 16) SCL
     PWM (D 14) PD6 20|        |21 PD7 (D 15) PWM
                      +--------+
*/


#include "wiring_private.h"
#include <Wire.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define LED_red   23   // PC7
#define RESET     0    // PB0
#define GPSpower  26   // PA2
#define SDpower1  1    // PB1
#define SDpower2  2    // PB2
#define SDpower3  3    // PB3
#define SS        4    // PB4
#define MOSI      5    // PB5
#define MISO      6    // PB6
#define SCK       7    // PB7
#define INT       20   // PC4

#define BQ34Z100 0x55


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t count = 0;
String dataString;

// read words by standard commands
int16_t readBat(int8_t regaddr)
{
  Wire.beginTransmission(BQ34Z100);
  Wire.write(regaddr);
  Wire.endTransmission();

  Wire.requestFrom(BQ34Z100,1);

  unsigned int low = Wire.read();

  Wire.beginTransmission(BQ34Z100);
  Wire.write(regaddr+1);
  Wire.endTransmission();

  Wire.requestFrom(BQ34Z100,1);

  unsigned int high = Wire.read();

  unsigned int high1 = high<<8;

  return (high1 + low);
}

// Read one byte from battery guage flash
uint8_t ReadFlashByte(uint8_t fclass, uint8_t foffset)
{
  Wire.beginTransmission(BQ34Z100);
  Wire.write(0x61);                   // start access to flash memory
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(BQ34Z100);   // memory Subclass
  Wire.write(0x3E);
  Wire.write(fclass);
  Wire.endTransmission();
  Wire.beginTransmission(BQ34Z100);   // memory 32 bytes page
  Wire.write(0x3F);
  Wire.write(foffset / 32);
  Wire.endTransmission();

  uint16_t fsum = 0;                  // compute checksum of 32 bytes RAM block
  Wire.beginTransmission(BQ34Z100);
  Wire.write(0x40);
  Wire.endTransmission();
  Wire.requestFrom(BQ34Z100,32);
  for (uint8_t addr=0; addr<32; addr++)
  {
    uint8_t tmp = Wire.read();
    fsum += tmp;
  }
  fsum = (0xFF^fsum) & 0xFF;          // invert bits

  Wire.beginTransmission(BQ34Z100);   // read specific byte
  Wire.write(0x40+(foffset % 32));
  Wire.endTransmission();
  Wire.requestFrom(BQ34Z100,1);
  uint8_t value = Wire.read();

  Wire.beginTransmission(BQ34Z100);   // read of chcecksum of RAM block
  Wire.write(0x60);
  Wire.endTransmission();
  Wire.requestFrom(BQ34Z100,1);
  uint8_t v = Wire.read();

  return (value);
}

// Write one byte to battery guage flash
void WriteFlashByte(uint8_t fclass, uint8_t foffset, uint8_t fbyte)
{
  for(uint8_t xx=0; xx<4; xx++) // I do not now why
  {
    Wire.beginTransmission(BQ34Z100);
    Wire.write(0x61);                   // start access to flash memory
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.beginTransmission(BQ34Z100);
    Wire.write(0x3E);                   // memory Subclass
    Wire.write(fclass);
    Wire.endTransmission();
    Wire.beginTransmission(BQ34Z100);
    Wire.write(0x3F);                   // memory 32 bytes page
    Wire.write(foffset / 32);
    Wire.endTransmission();


    uint16_t fsum = 0;                  // Compute chcecksum of readed block
    Wire.beginTransmission(BQ34Z100);
    Wire.write(0x40);
    Wire.endTransmission();
    Wire.requestFrom(BQ34Z100,32);
    for (uint8_t addr=0; addr<32; addr++)
    {
      uint8_t tmp = Wire.read();
      fsum += tmp;
    }
    fsum = (0xFF^fsum) & 0xFF;

    Wire.beginTransmission(BQ34Z100);   // Rewrite specific byte in readed block
    Wire.write(0x40+(foffset % 32));
    Wire.write(fbyte);
    Wire.endTransmission();

    fsum = 0;                               // Compute new chcecksum
    Wire.beginTransmission(BQ34Z100);
    Wire.write(0x40);
    Wire.endTransmission();
    Wire.requestFrom(BQ34Z100,32);
    for (uint8_t addr=0; addr<32; addr++)
    {
      uint8_t tmp = Wire.read();
      fsum += tmp;
    }
    fsum = (0xFF^fsum) & 0xFF;

    delay(100);
    Wire.beginTransmission(BQ34Z100);   // Write new checksum and those rewrite flash
    Wire.write(0x60);
    Wire.write(fsum);
    Wire.endTransmission();
    delay(100);
  }
}

// Reset battery guage (0x0041)
void ResetGuage()
{
  Wire.beginTransmission(BQ34Z100);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(BQ34Z100);
  Wire.write(0x01);
  Wire.write(0x41);
  Wire.endTransmission();
}

int8_t readb(int8_t command)
{
  Wire.beginTransmission(BQ34Z100);
  Wire.write(command);
  Wire.endTransmission();
  Wire.requestFrom(BQ34Z100,1);
  uint8_t low = Wire.read();

}

int16_t I;
float capacity = 0;
  
void PrintBatteryStatus()
{
  dataString = "$GUAGE, ";

  uint16_t U = readBat(0x8);
  I = readBat(0xa);
  capacity += float(I) / 3600;
  int16_t A = readBat(0x4);
  float t = readBat(0xc) * 0.1 - 273.15;
  
  dataString += String(count);   // charging time
  dataString += " s, ";
  dataString += String(U);   // mV - U
  dataString += " mV, ";
  dataString += String(I);  // mA - I
  dataString += " mA, ";
  dataString += String(capacity);   // mAh - transferred charge
  dataString += " mAh, ";
  dataString += String(t);   // temperature
  dataString += " C";

  digitalWrite(LED_red, HIGH);  // Blink for Dasa
  Serial.println(dataString);   // print to terminal (additional 700 ms in DEBUG mode)
  digitalWrite(LED_red, LOW);

  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  if (I == -257)
  {
    display.setCursor(10, 20);     // Start at top-left corner
    display.print("Insert");
    display.setCursor(10, 48);     // Start at top-left corner
    display.print("Cell");
    capacity = 0;
  }
  else
  {
    display.setCursor(0, 0);     // Start at top-left corner
    display.print(String(U) + " mV");
    display.setCursor(0, 18);     // Start at top-left corner
    if( I == 0)
    {
      display.print("* Finish *");            
    }
    else
    {
      display.print(String(I) + " mA");      
    }
    display.setCursor(0, 36);     // Start at top-left corner
    display.print(String(round(capacity)) + " mAh");
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setCursor(16, 57);     // Start at top-left corner
    display.print(String(t));
    display.setCursor(0, 53);     // Start at top-left corner
    display.print("o");
    display.setCursor(6, 57);     // Start at top-left corner
    display.print("C");
    display.setCursor(70, 57);     // Start at top-left corner
    display.print(String(count) + " s");
  }
  display.display();    
}


void setup()
{
  Wire.setClock(1000);

  // Open serial communications
  Serial.begin(9600);
  Serial1.begin(9600);

  Serial.println("#Cvak...");

  pinMode(LED_red, OUTPUT);
  digitalWrite(LED_red, LOW);
  digitalWrite(RESET, LOW);

  for(int i=0; i<5; i++)
  {
    delay(50);
    digitalWrite(LED_red, HIGH);  // Blink for Dasa
    delay(50);
    digitalWrite(LED_red, LOW);
  }

  Serial.println("#Hmmm...");

}

void loop()
{
  //if(!display.begin(SSD1306_EXTERNALVCC,SSD1306_SWITCHCAPVCC, 0x3C)) { 
  if(!display.begin(SSD1306_EXTERNALVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }

  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...
  display.clearDisplay();

  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.

  display.setCursor(40, 0);     // Start at top-left corner
  display.print("LiIon");
  display.setCursor(30, 18);     // Start at top-left corner
  display.print("CHARGER");
  display.setCursor(50, 36);     // Start at top-left corner
  display.print("MLAB");
  display.display();  
  delay(2000);

  PrintBatteryStatus();
  Serial.println();
  /* old version BQ34Z100
  dataString = "$FLASH,";
  dataString += String(ReadFlashByte(48, 68),HEX);
  dataString += String(ReadFlashByte(48, 69),HEX);
  dataString += String(ReadFlashByte(48, 70),HEX);
  dataString += String(ReadFlashByte(48, 71),HEX);
  dataString += ",";
  dataString += String(readb(0x63),HEX);
  dataString += String(readb(0x64),HEX);
  dataString += String(readb(0x65),HEX);
  dataString += String(readb(0x66),HEX);
  dataString += String(readb(0x67),HEX);
  dataString += String(readb(0x68),HEX);
  dataString += String(readb(0x69),HEX);
  dataString += String(readb(0x6a),HEX);
  Serial.println(dataString);
  */

  dataString = "$FLASH,";
  for (uint8_t n=32; n<43; n++) dataString += char(ReadFlashByte(48, n));   // Part type
  dataString += ",";
  for (uint8_t n=44; n<55; n++) dataString += char(ReadFlashByte(48, n));   // Manufacturer
  dataString += ",";
  for (uint8_t n=56; n<60; n++) dataString += char(ReadFlashByte(48, n));   // Chemistry

  digitalWrite(LED_red, HIGH);  // Blink for Dasa
  Serial.println(dataString);   // print to terminal (additional 700 ms in DEBUG mode)
  digitalWrite(LED_red, LOW);

  Serial.print("#LED CONF: ");
  Serial.println(ReadFlashByte(64,4), HEX);
  Serial.print("#Design Capacity: ");
  Serial.println(ReadFlashByte(48,11)*256 + ReadFlashByte(48,12));
  Serial.print("#Design Energy: ");
  Serial.println(ReadFlashByte(48,13)*256 + ReadFlashByte(48,14));
  Serial.print("#Bat. low alert: ");
  Serial.print("#Cell BL Set Volt Threshold: ");
  Serial.println(ReadFlashByte(49,8)*256 + ReadFlashByte(49,9));
  Serial.print("#Cell BL Clear Volt Threshold: ");
  Serial.println(ReadFlashByte(49,11)*256 + ReadFlashByte(49,12));
  Serial.print("#Cell BL Set Volt Time: ");
  Serial.println(ReadFlashByte(49,10));
  Serial.print("#Cell BH Set Volt Threshold: ");
  Serial.println(ReadFlashByte(49,13)*256 + ReadFlashByte(49,14));
  Serial.print("#Cell BH Volt Time: ");
  Serial.println(ReadFlashByte(49,15));
  Serial.print("#Cell BH Clear Volt Threshold: ");
  Serial.println(ReadFlashByte(49,16)*256 + ReadFlashByte(49,17));
  Serial.print("#Cycle Delta: ");
  Serial.println(ReadFlashByte(49,21));

/*
  for(uint8_t i=0; i<128; i+=10) 
  {
    display.drawPixel(i, 63, WHITE);
  }
  
  for(uint8_t i=0; i<128; i+=1) 
  {
    display.drawLine(i,60, i, 61, WHITE);
  }
*/  
  display.display();  

  // ResetGuage();
 
  while(true)
  {
    PrintBatteryStatus();  
    if(I>0) count++;  
    if(I<0) count = 0;
    delay(1000);
  }
}
