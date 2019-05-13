#include <bluefruit.h> // Need the bluefruit library, as this is a Bluetooth comptatible device

#include <Adafruit_DotStar.h> // Need the DotStar library, as that is the rane of LED strip being utilised
#include <SPI.h>

#define NUMPIXELS 48 // Defines the total number of LEDs

#define DATAPIN    16 // Defines which pin is utilised for Data
#define CLOCKPIN   15 // Defines which pin is utilised for Clock

Adafruit_DotStar strip = Adafruit_DotStar(
                           NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG); // Defines the strip

BLEUart bleuart;

// Function prototypes for packetparser.cpp
uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

// Predefined colour, utilised on initialisation
uint32_t paleBlue = strip.Color(241, 66, 244);
// Variable that holds the current colour, initalises with 'Pale Blue'
uint32_t currentColour = paleBlue;

// Utilised in tracking the time
unsigned long currentMillis = millis();
// Used to remember when last event occured
unsigned long previousMillis = millis();

// Variable utilised in determining the brightness of the LEDs
int strength = 0;

// Variable utilised in detemining the speed of the animations
int speedFactor = 4;

// Variables to store the individual RGB components of a colour
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

// Arrays that define the LEDs associates with each step of the animation
int state1[9] = {5, 6, 7, 8, 10, 16, 24, 28, 32};
int state2[21] = {1, 2, 4, 9, 11, 15, 17, 20, 23, 25, 27, 29, 31, 33, 36, 39, 40, 44, 45, 46, 47};
int state3[18] = {0, 3, 12, 13, 14, 18, 19, 21, 22, 26, 30, 34, 35, 37, 38, 41, 42, 43};

// Packet buffer
extern uint8_t packetbuffer[];

void setup(void)
{
  // Anything involving Serial is for monitoring purposes, they can be viewed in the Serial Monitor
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println(F("Adafruit Bluefruit52 Controller App Example"));
  Serial.println(F("-------------------------------------------"));

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bluefruit52");

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

void startAdv(void)
{
  // Code that controls the advertisment of the device
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(bleuart);

  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
  return color & 0xFF;
}

// Darkens a colour value based on the given 'strength' variable
uint32_t Darken(uint32_t maxColor, int strength)
{
  // Finds the red value in the max colour, then decreases the value by a percentage
  uint8_t red = (Red(maxColor) / 100) * (100 - strength);

  // Finds the green value in the max colour, then decreases the value by a percentage
  uint8_t green = (Green(maxColor) / 100) * (100 - strength);

  // Finds the blue value in the max colour, then decreases the value by a percentage
  uint8_t blue = (Blue(maxColor) / 100) * (100 - strength);

  // Adds the colours back together
  uint32_t newColor = strip.Color(red, green, blue);
  return newColor;
}

// Brightens a colour value based on the given 'strength' variable
uint32_t Brighten(uint32_t maxColor, int strength)
{
  // Finds the red value in the max colour, then increases the value by a percentage
  uint8_t red = (Red(maxColor) / 100) * strength;

  // Finds the green value in the max colour, then increases the value by a percentage
  uint8_t green = (Green(maxColor) / 100) * strength;

  // Finds the blue value in the max colour, then increases the value by a percentage
  uint8_t blue = (Blue(maxColor) / 100) * strength;

  // Adds the colours back together
  uint32_t newColor = strip.Color(red, green, blue);
  return newColor;
}

// Defines the first stage of the animation, brightens the first set, darkens the thrid
void firstAnimation(int strength)
{
  for (int i = 0; i <= 8; i++) {
    strip.setPixelColor(state1[i], Brighten(currentColour, strength));
  }
  for (int j = 0; j <= 20; j++) {
    strip.setPixelColor(state2[j], 0, 0, 0);
  }
  for (int k = 0; k <= 17; k++) {
    strip.setPixelColor(state3[k], Darken(currentColour, strength));
  }
  strip.show();
}

// Defines the second stage of the animation, brightens the second set, darkens the first
void secondAnimation(int strength)
{
  for (int i = 0; i <= 8; i++) {
    strip.setPixelColor(state1[i], Darken(currentColour, strength));
  }
  for (int j = 0; j <= 20; j++) {
    strip.setPixelColor(state2[j], Brighten(currentColour, strength));
  }
  for (int k = 0; k <= 17; k++) {
    strip.setPixelColor(state3[k], 0, 0, 0);
  }
  strip.show();
}

// Defines the thrid stage of the animation, brightens the third set, darkens the second
void thirdAnimation(int strength)
{
  for (int i = 0; i <= 8; i++) {
    strip.setPixelColor(state1[i], 0, 0, 0);
  }
  for (int j = 0; j <= 20; j++) {
    strip.setPixelColor(state2[j], Darken(currentColour, strength));
  }
  for (int k = 0; k <= 17; k++) {
    strip.setPixelColor(state3[k], Brighten(currentColour, strength));
  }
  strip.show();
}

// Controls the animation, decides which stage is currently playing
void animationUpdate()
{
  // Finds the current time
  currentMillis = millis();

  // If the time is in the first third of the available window then the first stage of the animation is run
  // This window is determined by the speedFactor * 100 milliseconds
  if (currentMillis - previousMillis < speedFactor * 100) {
    // Strength is a percentage determined by the elapsed time
    strength = (currentMillis - previousMillis) / speedFactor;
    firstAnimation(strength);
  }
  
  // If the time is in the second third of the available window then the second stage of the animation is run
  else if ((currentMillis - previousMillis >= speedFactor * 100) && (currentMillis - previousMillis < speedFactor * 200)) {
    strength = ((currentMillis - previousMillis) - speedFactor * 100) / speedFactor;
    secondAnimation(strength);
  }
  
  // If the time is in the final third of the available window then the third stage of the animation is run
  else if ((currentMillis - previousMillis >= speedFactor * 200) && (currentMillis - previousMillis < speedFactor * 300)) {
    strength = ((currentMillis - previousMillis) - speedFactor * 200) / speedFactor;
    thirdAnimation(strength);
  }
  
  //If the time is greater than the time given for the animation to elapse, reset
  else {
    previousMillis = currentMillis;
  }
}

// Looks for, then reads the received communication
void packetUpdate()
{
  Serial.println("packetUpdate");

  // readPacket is a function in the 'packetParser.cpp' file
  uint8_t len = readPacket(&bleuart, 500);
}

// This is the main loop, is is constantly repeated
void loop(void)
{
  // Reduces maximum capable brightness to 1/5th
  strip.setBrightness(255 / 5);

  // Calls the function that controls the aniamtions, decides on the current state
  animationUpdate();

  // Calls the function that controls the communications
  packetUpdate();

  // If the packet simply sent a new colour value
  if (packetbuffer[1] == 'C') {
    uint8_t red = packetbuffer[2];
    uint8_t green = packetbuffer[3];
    uint8_t blue = packetbuffer[4];

    currentColour = strip.Color(green, red, blue);
  }

  // If the packet sent a new speed value
  if (packetbuffer[1] == 'S') {
    uint8_t speedVal = packetbuffer[2] - '0';
    speedFactor = speedVal;
  }

  // If the packet sent accelerometer data
  if (packetbuffer[1] == 'A') {
    float x, y, z;
    x = parsefloat(packetbuffer + 2);
    y = parsefloat(packetbuffer + 6);
    z = parsefloat(packetbuffer + 10);

    // Reduces the coordinates by a factor of 10, mostly placing them between 1 - 0
    x = x / 10;
    y = y / 10;
    z = z / 10;

    // If the value somehow manages to be greater than 1, limit to 1
    if (abs(x) > 1) {
      x = 1.0;
    }

    if (abs(y) > 1) {
      y = 1.0;
    }

    if (abs(z) > 1) {
      z = 1.0;
    }

    // Multiply the colours by the magnitude of the coordinates
    uint8_t red = 255 * abs(x);
    uint8_t green = 255 * abs(y);
    uint8_t blue = 255 * abs(z);

    currentColour = strip.Color(green, red, blue);
  }

  // If the packet sent magnetometer data
  if (packetbuffer[1] == 'M') {
    float x, y, z;
    x = parsefloat(packetbuffer + 2);
    y = parsefloat(packetbuffer + 6);
    z = parsefloat(packetbuffer + 10);
  }

  // If the packet sent gyroscope data
  if (packetbuffer[1] == 'G') {
    float x, y, z;
    x = parsefloat(packetbuffer + 2);
    y = parsefloat(packetbuffer + 6);
    z = parsefloat(packetbuffer + 10);

    // If a rotational vector is large enough, change to a set colour
    if (x > 3) {

      red = 255;
      green = 0;
      blue = 0;

    }

    else if (x < -3) {

      red = 0;
      green = 0;
      blue = 0;

    }

    else if (abs(y) > 3) {

      red = 0;
      green = 255;
      blue = 0;

    }

    else if (abs(z) > 3) {

      red = 0;
      green = 0;
      blue = 255;

    }

    currentColour = strip.Color(green, red, blue);

  }

  // If the packet sent quaternion data
  if (packetbuffer[1] == 'Q') {
    float x, y, z, w;
    x = parsefloat(packetbuffer + 2);
    y = parsefloat(packetbuffer + 6);
    z = parsefloat(packetbuffer + 10);
    w = parsefloat(packetbuffer + 14);
  }
}
