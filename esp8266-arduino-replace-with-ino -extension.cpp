#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define LED_PIN        4
#define STATUS_LED     LED_BUILTIN
#define SERIAL_ACTIVE_MS 500
#define STATUS_BLINK_MS 500
#define NUM_LEDS       128   /* 8x16 Display */
#define EEPROM_MAGIC   0x5C
#define EEPROM_ADDR    1
#define EEPROM_SNAKE_ADDR 2
#define EEPROM_TETRIS_ADDR 3
#define EEPROM_LAYOUT_ADDR 4
#define EEPROM_LAYOUT_MAGIC 0xA7

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

/* CIE 1931 / Gamma 2.4 Correction Table */
const uint8_t PROGMEM gammaTable[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
    6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
   12,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,
   20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,  30,
   30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
   43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,  56,
   57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,
   73,  74,  76,  77,  78,  79,  80,  81,  83,  84,  85,  86,  87,  89,  90,  91,
   92,  94,  95,  96,  97,  99, 100, 101, 103, 104, 105, 107, 108, 109, 111, 112,
  114, 115, 116, 118, 119, 121, 122, 124, 125, 127, 128, 130, 131, 133, 134, 136,
  138, 139, 141, 142, 144, 146, 147, 149, 151, 152, 154, 156, 157, 159, 161, 163,
  164, 166, 168, 170, 171, 173, 175, 177, 179, 181, 183, 184, 186, 188, 190, 192,
  194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 215, 217, 219, 221, 223, 225,
  228, 230, 232, 234, 237, 239, 241, 243, 246, 248, 250, 253, 255, 255, 255, 255
};

/* Color Palette RGB Definitions */
const uint8_t paletteRGB[8][3] = {
    {  0,   0,   0}, /* 0: OFF */
    {  0, 255,  50}, /* 1: Green (Snake Body & Run Score) */
    {255,   0,   0}, /* 2: Red (Apple 1 & Skull) */
    {  0, 240, 255}, /* 3: Cyan (Snake Head) */
    {255, 210,   0}, /* 4: Bright Yellow (High Score) */
    {255,   0, 255}, /* 5: Magenta */
    {255,  45,   0}, /* 6: Deep Amber Orange (Apple 2 - clearly distinct from Yellow) */
    {255, 255, 255}  /* 7: White */
};

enum ParseState {
    WAIT_HDR1,
    WAIT_HDR2,
    READ_MODE,
    READ_SCORE,
    READ_PAYLOAD,
    WAIT_EEPROM_MODE,
    WAIT_EEPROM_SCORE
};

ParseState currentState = WAIT_HDR1;
uint8_t payloadBuffer[128];
uint16_t payloadIndex = 0;
uint8_t currentGameMode = 0;
uint8_t currentScore = 0;
uint8_t storedHighScores[4] = {0, 0, 0, 0};
uint8_t pendingHighScoreMode = 0;
unsigned long lastByteTime = 0;
unsigned long lastValidFrameTime = 0;
bool hasRecentValidFrame = false;
bool waitingHeartShown = false;

uint16_t mapCoordinates(uint8_t x, uint8_t y) {
    if (x >= 8 || y >= 16) return 0;
    uint8_t panel = y / 8;
    uint8_t localY = y % 8;
    return (panel * 64) + (localY * 8) + x;
}

void loadHighScores(void) {
    EEPROM.begin(64);
    if (EEPROM.read(0) != EEPROM_MAGIC) {
        EEPROM.write(0, EEPROM_MAGIC);
        EEPROM.write(EEPROM_ADDR, 0);
        EEPROM.write(EEPROM_SNAKE_ADDR, 0);
        EEPROM.write(EEPROM_TETRIS_ADDR, 0);
        EEPROM.write(EEPROM_LAYOUT_ADDR, EEPROM_LAYOUT_MAGIC);
        EEPROM.commit();
    } else if (EEPROM.read(EEPROM_LAYOUT_ADDR) != EEPROM_LAYOUT_MAGIC) {
        /* Keep the existing game 1 score; initialize the new game score slots. */
        EEPROM.write(EEPROM_SNAKE_ADDR, 0);
        EEPROM.write(EEPROM_TETRIS_ADDR, 0);
        EEPROM.write(EEPROM_LAYOUT_ADDR, EEPROM_LAYOUT_MAGIC);
        EEPROM.commit();
    }

    storedHighScores[1] = EEPROM.read(EEPROM_ADDR);
    storedHighScores[2] = EEPROM.read(EEPROM_SNAKE_ADDR);
    storedHighScores[3] = EEPROM.read(EEPROM_TETRIS_ADDR);
}

uint8_t updateHighScore(uint8_t gameMode, uint8_t s) {
    uint8_t address;
    if (gameMode == 1) address = EEPROM_ADDR;
    else if (gameMode == 2) address = EEPROM_SNAKE_ADDR;
    else if (gameMode == 3) address = EEPROM_TETRIS_ADDR;
    else return 0;

    if (s > storedHighScores[gameMode]) {
        storedHighScores[gameMode] = s;
        EEPROM.write(address, s);
        EEPROM.commit();
    }
    return storedHighScores[gameMode];
}

void drawWaitingHeart(void) {
    const uint8_t heartRows[8] = {0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00};
    matrix.clear();
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            if (heartRows[y] & (0x80 >> x)) {
                matrix.setPixelColor(mapCoordinates(x, y + 4), matrix.Color(255, 0, 35));
            }
        }
    }
    matrix.show();
    waitingHeartShown = true;
}

void updateStatusIndicator(void) {
    if (hasRecentValidFrame && (millis() - lastValidFrameTime <= SERIAL_ACTIVE_MS)) {
        digitalWrite(STATUS_LED, LOW);  /* NodeMCU built-in LED is active LOW */
    } else {
        hasRecentValidFrame = false;
        if (!waitingHeartShown) drawWaitingHeart();
        bool ledOn = ((millis() / STATUS_BLINK_MS) & 1) != 0;
        digitalWrite(STATUS_LED, ledOn ? LOW : HIGH);
    }
}

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);  /* Off during startup */

    matrix.begin();
    matrix.setBrightness(45);
    matrix.show();
    drawWaitingHeart();

    loadHighScores();

    /* Hardware UART mapped to D7 (RX) and D8 (TX) */
    Serial.begin(57600);
    Serial.swap();
}
void renderBuffer(void) {
    for (uint8_t y = 0; y < 16; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            uint8_t colorIdx = payloadBuffer[(y * 8) + x] & 0x07;
            uint16_t ledIdx = mapCoordinates(x, y);
           
            uint8_t r = pgm_read_byte(&gammaTable[paletteRGB[colorIdx][0]]);
            uint8_t g = pgm_read_byte(&gammaTable[paletteRGB[colorIdx][1]]);
            uint8_t b = pgm_read_byte(&gammaTable[paletteRGB[colorIdx][2]]);
           
            matrix.setPixelColor(ledIdx, matrix.Color(r, g, b));
        }
    }
    matrix.show();
}

void loop() {
    if (currentState != WAIT_HDR1 && (millis() - lastByteTime > 50)) {
        currentState = WAIT_HDR1;
    }

    while (Serial.available() > 0) {
        uint8_t inByte = Serial.read();
        lastByteTime = millis();

        switch (currentState) {
            case WAIT_HDR1:
                if (inByte == 0xA5) {
                    currentState = WAIT_HDR2;
                } else if (inByte == 0xAA) {
                    currentState = WAIT_EEPROM_MODE;
                }
                break;

            case WAIT_EEPROM_MODE:
                pendingHighScoreMode = inByte;
                currentState = WAIT_EEPROM_SCORE;
                break;

            case WAIT_EEPROM_SCORE:
                Serial.write(updateHighScore(pendingHighScoreMode, inByte));
                currentState = WAIT_HDR1;
                break;

            case WAIT_HDR2:
                if (inByte == 0x5A) currentState = READ_MODE;
                else currentState = WAIT_HDR1;
                break;

            case READ_MODE:
                currentGameMode = inByte;
                currentState = READ_SCORE;
                break;

            case READ_SCORE:
                currentScore = inByte;
                payloadIndex = 0;
                currentState = READ_PAYLOAD;
                break;

            case READ_PAYLOAD:
                if (payloadIndex < sizeof(payloadBuffer)) {
                    payloadBuffer[payloadIndex++] = inByte;
                }
                if (payloadIndex >= sizeof(payloadBuffer)) {
                    renderBuffer();
                    lastValidFrameTime = millis();
                    hasRecentValidFrame = true;
                    waitingHeartShown = false;
                    currentState = WAIT_HDR1;
                }
                break;
        }
    }

    updateStatusIndicator();
}
