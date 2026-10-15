// ESP8266 receiver for the 8051 game console (display driver only).
// Packet from the 8051:  0xAA, N, N x (pos, colour), XOR checksum, 0x55
//   pos = y * 8 + x (0..127), colour = palette index (0..9)
//   checksum = XOR of N and all pos/colour bytes
// Wiring: 8051 TXD (P3.1) -> 1k/2k divider -> ESP8266 GPIO13 (D7). Common GND.

#include <Adafruit_NeoPixel.h>

#define DATA_PIN 4        // GPIO4 (D2) -> WS2812 DIN
#define NUM_LEDS 128
#define SERPENTINE 0      // 0 = every row runs left to right (your panels)

Adafruit_NeoPixel matrix(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

uint32_t PAL[10];   //
uint8_t frameBuf[NUM_LEDS];   // colour index of every pixel
uint8_t pend[256];            // pairs of the packet being received , pending payload buffer
// pend = [pos0,col0,pos1,col1,pos2,col2,pos3,col3,pos4,col4....]

enum { S_START, S_COUNT, S_DATA, S_SUM, S_END };
// start = wait for 0xAA our start header
//count , will read N 
//data , read position and pixel
//sum , xor checksum
//end , 0x55 end footer

uint8_t st = S_START, n = 0, sum = 0;
uint16_t got = 0;             // 16-bit: a full frame is 256 data bytes
bool sumOk = false;

void render()
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t x = i & 7, y = i >> 3;  //x = i & 7 , we will get our x and then divide the rest by 8 we will get our y
    uint16_t led = (SERPENTINE && (y & 1)) ? (y * 8 + 7 - x) : (y * 8 + x);   //all my lif e, ive been trying to keep it up 
    matrix.setPixelColor(led, PAL[frameBuf[i]]);
  }
  matrix.show();            // once per packet, never once per pixel
}

void applyPacket()
{
  for (uint8_t i = 0; i < n; i++)
  {
    uint8_t pos = pend[2 * i], col = pend[2 * i + 1];  // pos is position whihc is on the even index and col is the colour which is on the odd index
    if (pos < NUM_LEDS && col < 10) frameBuf[pos] = col;  // and we load like only valid values if pos < 127 and colour is less that 10 because we are using only 10 colours
  }
  render(); //render will demodulate the position and will load into the frameBuff
  digitalWrite(LED_BUILTIN, HIGH);          // good packet: onboard LED off
}

void feed(uint8_t b)
{
  switch (st)
  {
    case S_START:
      if (b == 0xAA) st = S_COUNT;
      break;
    case S_COUNT:
      if (b > NUM_LEDS) { st = S_START; break; }
      n = b; got = 0; sum = b;
      st = n ? S_DATA : S_SUM;
      break;
    case S_DATA:
      pend[got++] = b; sum ^= b;
      if (got == (uint16_t)(2 * n)) st = S_SUM;
      break;
    case S_SUM:
      sumOk = (b == sum);
      st = S_END;
      break;
    case S_END:
      if (sumOk && b == 0x55) applyPacket();   // bad packets are dropped
      st = S_START;
      break;
  }
}

void setup() {
  Serial.begin(74880);
  Serial.println();
  Serial.println(ESP.getResetInfo());   // why did it reboot?
  Serial.flush();
  Serial.end();
  Serial.setRxBufferSize(512);
  Serial.begin(57600);      // must match the 8051
  Serial.swap();            // RX on GPIO13 (D7)

  matrix.begin();
  matrix.setBrightness(10);
  PAL[0] = matrix.Color(0, 0, 0);        // off
  PAL[1] = matrix.Color(255, 0, 0);      // red
  PAL[2] = matrix.Color(0, 255, 0);      // green
  PAL[3] = matrix.Color(0, 0, 255);      // blue
  PAL[4] = matrix.Color(0, 255, 255);    // cyan
  PAL[5] = matrix.Color(0, 10, 128);     // dark blue
  PAL[6] = matrix.Color(89, 89, 255);    // light blue
  PAL[7] = matrix.Color(255, 255, 0);    // yellow
  PAL[8] = matrix.Color(50, 50, 50);     // dim white
  PAL[9] = matrix.Color(255, 0, 255);    // magenta
  matrix.clear();
  matrix.show();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);          // onboard LED off (active low)

  // Start-up self test: sweep one dim white dot through all 128 LEDs
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    matrix.clear();
    matrix.setPixelColor(i, matrix.Color(40, 40, 40));
    matrix.show();
    delay(10);
  }
  matrix.clear();
  matrix.show();
}

void loop() {
  while (Serial.available()) {
    digitalWrite(LED_BUILTIN, LOW);         // a byte arrived: onboard LED on
    feed(Serial.read());
  }
}


//y &1 will check if odd and reverse the order for x 
