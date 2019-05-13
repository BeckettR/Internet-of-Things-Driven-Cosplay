#include <string.h>
#include <Arduino.h>
#include <bluefruit.h>

#define PACKET_ACC_LEN                  (15)
#define PACKET_GYRO_LEN                 (15)
#define PACKET_MAG_LEN                  (15)
#define PACKET_QUAT_LEN                 (19)
#define PACKET_BUTTON_LEN               (5)
#define PACKET_COLOR_LEN                (6)
#define PACKET_LOCATION_LEN             (15)
#define READ_BUFSIZE                    (20)

uint8_t packetbuffer[READ_BUFSIZE+1];
unsigned long currentBLEMillis;
unsigned long previousBLEMillis = 0;

float parsefloat(uint8_t *buffer) 
{
  float f;
  memcpy(&f, buffer, 4);
  return f;
}

void printHex(const uint8_t * data, const uint32_t numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++) 
  {
    Serial.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
    {
      Serial.print(F("0"));
      Serial.print(data[szPos] & 0xf, HEX);
    }
    else
    {
      Serial.print(data[szPos] & 0xff, HEX);
    }
    // Add a trailing space if appropriate
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      Serial.print(F(" "));
    }
  }
  Serial.println();
}

uint8_t readPacket(BLEUart *ble_uart, uint16_t timeout) 
{
  uint16_t origtimeout = timeout, replyidx = 0;

  memset(packetbuffer, 0, READ_BUFSIZE);

  currentBLEMillis = millis();

  if (currentBLEMillis - previousBLEMillis < timeout) {
    if (replyidx >= 20);
    if ((packetbuffer[1] == 'A') && (replyidx == PACKET_ACC_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'G') && (replyidx == PACKET_GYRO_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'M') && (replyidx == PACKET_MAG_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'Q') && (replyidx == PACKET_QUAT_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'B') && (replyidx == PACKET_BUTTON_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'C') && (replyidx == PACKET_COLOR_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'L') && (replyidx == PACKET_LOCATION_LEN))
      previousBLEMillis = currentBLEMillis;
    if ((packetbuffer[1] == 'S') && (replyidx == PACKET_LOCATION_LEN))
      previousBLEMillis = currentBLEMillis;
    while (ble_uart->available()) {
      char c =  ble_uart->read();
      if (c == '!') {
        replyidx = 0;
      }
      packetbuffer[replyidx] = c;
      replyidx++;
      timeout = origtimeout;
  }
  }
  else {
    previousBLEMillis = currentBLEMillis;
  }

  packetbuffer[replyidx] = 0;  // null term

  if (!replyidx)  // no data or timeout 
    return 0;
  if (packetbuffer[0] != '!')  // doesn't start with '!' packet beginning
    return 0;
  
  // check checksum!
  uint8_t xsum = 0;
  uint8_t checksum = packetbuffer[replyidx-1];
  
  for (uint8_t i=0; i<replyidx-1; i++) {
    xsum += packetbuffer[i];
  }
  xsum = ~xsum;

  // Throw an error message if the checksum's don't match
  if (xsum != checksum)
  {
    Serial.print("Checksum mismatch in packet : ");
    printHex(packetbuffer, replyidx+1);
    return 0;
  }
  
  // checksum passed!
  return replyidx;
}
