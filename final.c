#include <8052.h>

/* ==========================================================================
 * HARDWARE REGISTERS & PINS (AT89S52)
 * ========================================================================== */
__sfr __at(0xC9) T2MOD;   /* Timer 2 Mode (T2OE for Clock Out) */

/* UART & ESP8266 Boot Protection */
__sbit __at(0xB0) P3_0;   /* Pin 10 (RXD) <- ESP8266 D8 (GPIO15) */
__sbit __at(0xB1) P3_1;   /* Pin 11 (TXD) -> ESP8266 D7 (GPIO13) */

/* Hardware 50.27 kHz Clock Output */
__sbit __at(0x90) P1_0;   /* Pin 1  (T2)  -> ADC0809 CLOCK (Pin 10) */

/* ADC0809 Control Bus */
__sbit __at(0xA0) ADC_A;     /* P2.0 -> ADC0809 ADD A (Pin 25) */
__sbit __at(0xA1) ADC_B;     /* P2.1 -> ADC0809 ADD B (Pin 24) */
__sbit __at(0xA3) ADC_ALE;   /* P2.3 -> ADC0809 ALE   (Pin 22) */
__sbit __at(0xA4) ADC_START; /* P2.4 -> ADC0809 START (Pin 6)  */
__sbit __at(0xA5) ADC_OE;    /* P2.5 -> ADC0809 OE    (Pin 9)  */
__sbit __at(0xA7) JOY_SW;    /* P2.7 -> Joystick Switch (Active LOW) */
#define ADC_BUS   P0         /* Port 0: ADC0809 8-bit Data Bus */

/* Palette Constants */
#define COL_OFF     0
#define COL_GREEN   1
#define COL_RED     2
#define COL_CYAN    3
#define COL_YELLOW  4
#define COL_MAGENTA 5
#define COL_ORANGE  6
#define COL_WHITE   7

/* ==========================================================================
 * MEMORY MAP (EXACTLY 64 BYTES IN ISEG - ZERO LINKER OVERFLOW)
 * ========================================================================== */
__idata unsigned char frameBuffer[64]; /* 128 pixels packed 2 per byte */

/* Shared Console Direct Variables */
unsigned char score = 0;
unsigned char highScore = 0;
unsigned char rng = 0x5D;
unsigned char menuSel = 0;
unsigned char menuAnim = 0;
unsigned char menuHold = 0;
unsigned char menuLastDir = 0;

/* Flat 36-Byte Shared Game Pool */
unsigned char g_mem[36];

/* --- Space Commander Aliases on g_mem --- */
#define MAX_BULLETS           5
#define MAX_ENEMIES           4
#define sp_shipX              g_mem[0]
#define sp_lives              g_mem[1]
#define sp_lastJoyDir         g_mem[2]
#define sp_joyHoldTimer       g_mem[3]
#define sp_bulletMoveTimer    g_mem[4]
#define sp_fireCooldown       g_mem[5]
#define sp_enemyMoveTimer     g_mem[6]
#define sp_maxAllowedEnemies  g_mem[7]
#define sp_enemySpeedTicks    g_mem[8]
#define sp_spawnCooldownTicks g_mem[9]
#define sp_bX                 (&g_mem[10]) /* 10..14 */
#define sp_bY                 (&g_mem[15]) /* 15..19 (255 = inactive) */
#define sp_eActive            (&g_mem[20]) /* 20..23 */
#define sp_eX                 (&g_mem[24]) /* 24..27 */
#define sp_eY                 (&g_mem[28]) /* 28..31 */
#define sp_eTimer             (&g_mem[32]) /* 32..35 */

/* --- Snake Aliases on g_mem --- */
#define DIR_UP                0
#define DIR_DOWN              1
#define DIR_LEFT              2
#define DIR_RIGHT             3
#define snakeBody             (&g_mem[0])  /* 0..19 (20 bytes max) */
#define sn_len                g_mem[20]
#define sn_curDir             g_mem[21]
#define sn_nextDir            g_mem[22]
#define sn_a1X                g_mem[23]
#define sn_a1Y                g_mem[24]
#define sn_a2X                g_mem[25]
#define sn_a2Y                g_mem[26]
#define sn_a2Active           g_mem[27]
#define sn_moveTick           g_mem[28]
#define sn_moveSpeed          g_mem[29]
#define sn_blinkTimer         g_mem[30]

/* --- Tetris Aliases on g_mem --- */
#define tt_curPiece           g_mem[0]
#define tt_curRot             g_mem[1]
#define tt_fallTimer          g_mem[2]
#define tt_fallSpeed          g_mem[3]
#define tt_moveRepeatTimer    g_mem[4]
#define tt_lastBtnState       g_mem[5]
signed char tt_pieceX = 3;
signed char tt_pieceY = 0;

/* ==========================================================================
 * FLASH TABLES (BITMAPS, FONTS & TETROMINOES)
 * ========================================================================== */
__code const unsigned char SKULL_SPRITE[8] = {
    0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0x3C, 0x24, 0x00
};

__code const unsigned char FONT3x5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, /* 0 */
    {0b010, 0b110, 0b010, 0b010, 0b111}, /* 1 */
    {0b111, 0b001, 0b111, 0b100, 0b111}, /* 2 */
    {0b111, 0b001, 0b111, 0b001, 0b111}, /* 3 */
    {0b101, 0b101, 0b111, 0b001, 0b001}, /* 4 */
    {0b111, 0b100, 0b111, 0b001, 0b111}, /* 5 */
    {0b111, 0b100, 0b111, 0b101, 0b111}, /* 6 */
    {0b111, 0b001, 0b010, 0b010, 0b010}, /* 7 */
    {0b111, 0b101, 0b111, 0b101, 0b111}, /* 8 */
    {0b111, 0b101, 0b111, 0b001, 0b111}  /* 9 */
};

__code const signed char PIECES[7][4][4][2] = {
    /* 0: I */
    {{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}, {{-1,1},{0,1},{1,1},{2,1}}, {{1,-1},{1,0},{1,1},{1,2}}},
    /* 1: O */
    {{{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}},  {{0,0},{1,0},{0,1},{1,1}}},
    /* 2: T */
    {{{-1,0},{0,0},{1,0},{0,-1}},{{0,-1},{0,0},{0,1},{1,0}}, {{-1,0},{0,0},{1,0},{0,1}}, {{0,-1},{0,0},{0,1},{-1,0}}},
    /* 3: S */
    {{{0,0},{1,0},{-1,1},{0,1}}, {{0,-1},{0,0},{1,0},{1,1}}, {{0,0},{1,0},{-1,1},{0,1}}, {{0,-1},{0,0},{1,0},{1,1}}},
    /* 4: Z */
    {{{-1,0},{0,0},{0,1},{1,1}}, {{1,-1},{1,0},{0,0},{0,1}}, {{-1,0},{0,0},{0,1},{1,1}}, {{1,-1},{1,0},{0,0},{0,1}}},
    /* 5: J */
    {{{-1,-1},{-1,0},{0,0},{1,0}},{{1,-1},{0,-1},{0,0},{0,1}},{{-1,0},{0,0},{1,0},{1,1}},{{0,-1},{0,0},{0,1},{-1,1}}},
    /* 6: L */
    {{{1,-1},{-1,0},{0,0},{1,0}},{{0,-1},{0,0},{0,1},{1,1}}, {{-1,0},{0,0},{1,0},{1,1}},{{-1,-1},{0,-1},{0,0},{0,1}}}
};

__code const unsigned char PIECE_COLORS[7] = {
    COL_CYAN, COL_YELLOW, COL_MAGENTA, COL_GREEN, COL_RED, COL_ORANGE, COL_WHITE
};

/* ==========================================================================
 * HARDWARE DRIVERS & FRAME BUFFER PIPELINE
 * ========================================================================== */
void delay_ms(unsigned int ms) {
    unsigned int i;
    volatile unsigned char j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 120; j++);
    }
}

unsigned char get_random(void) {
    rng = (unsigned char)((rng * 73) + 19);
    return rng;
}

void init_hw_clock(void) {
    T2MOD |= 0x02;
    T2CON = 0x00;
    RCAP2H = 0xFF;
    RCAP2L = 0xC9;      /* 50.27 kHz @ 11.0592 MHz */
    TH2 = 0xFF;
    TL2 = 0xC9;
    T2CON |= 0x04;
}

void uart_init(void) {
    SCON = 0x50;
    TMOD = 0x20;
    PCON |= 0x80;
    TH1  = 0xFF;
    TL1  = 0xFF;
    TR1  = 1;
    TI   = 0;
    RI   = 0;
}

void uart_write(unsigned char val) {
    SBUF = val;
    while (!TI);
    TI = 0;
}

unsigned char uart_read_timeout(void) {
    unsigned int timeout = 0;
    while (!RI) {
        if (++timeout > 30000) return 0;
    }
    RI = 0;
    return SBUF;
}

unsigned char adc_read(unsigned char channel) {
    unsigned char val;
    volatile unsigned char k;
    ADC_BUS = 0xFF;
    ADC_A = channel & 0x01;
    ADC_B = (channel >> 1) & 0x01;
    for (k = 0; k < 10; k++);

    ADC_ALE = 1;
    for (k = 0; k < 25; k++);
    ADC_ALE = 0;

    ADC_START = 1;
    for (k = 0; k < 35; k++);
    ADC_START = 0;

    delay_ms(2);

    ADC_OE = 1;
    for (k = 0; k < 10; k++);
    val = ADC_BUS;
    ADC_OE = 0;

    return val;
}

void clear_buffer(void) {
    unsigned char i;
    for (i = 0; i < 64; i++) frameBuffer[i] = 0;
}

void set_pixel(unsigned char x, unsigned char y, unsigned char color) {
    unsigned char idx;
    if (x > 7 || y > 15) return;
    idx = (y << 3) | x;
    if (idx & 1) {
        frameBuffer[idx >> 1] = (frameBuffer[idx >> 1] & 0x0F) | (color << 4);
    } else {
        frameBuffer[idx >> 1] = (frameBuffer[idx >> 1] & 0xF0) | (color & 0x0F);
    }
}

unsigned char get_pixel(unsigned char x, unsigned char y) {
    unsigned char idx;
    if (x > 7 || y > 15) return 0;
    idx = (y << 3) | x;
    if (idx & 1) {
        return (frameBuffer[idx >> 1] >> 4) & 0x0F;
    }
    return frameBuffer[idx >> 1] & 0x0F;
}

/* Unpacks 64 bytes into 128 bytes over UART with complete protocol header */
void send_frame(unsigned char mode, unsigned char curScore) {
    unsigned char i, b;
    uart_write(0xA5);
    uart_write(0x5A);
    uart_write(mode);
    uart_write(curScore);
    for (i = 0; i < 64; i++) {
        b = frameBuffer[i];
        uart_write(b & 0x0F);        /* Even pixel */
        uart_write((b >> 4) & 0x0F); /* Odd pixel */
    }
}

void render_digit(unsigned char xStart, unsigned char yStart, unsigned char digit, unsigned char col) {
    unsigned char r, b;
    for (r = 0; r < 5; r++) {
        unsigned char rowBits = FONT3x5[digit][r];
        for (b = 0; b < 3; b++) {
            if (rowBits & (0x04 >> b)) {
                set_pixel(xStart + b, yStart + r, col);
            }
        }
    }
}

void show_game_over(unsigned char gameMode) {
    unsigned char r, c, f;

    clear_buffer();
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            if (SKULL_SPRITE[r] & (0x80 >> c)) {
                set_pixel(c, r + 4, COL_RED);
            }
        }
    }
    for (f = 0; f < 25; f++) {
        send_frame(gameMode, score);
        delay_ms(50);
    }

    uart_write(0xAA);
    uart_write(gameMode);
    uart_write(score);
    highScore = uart_read_timeout();
    if (score > highScore) {
        highScore = score;
    }

    clear_buffer();
    render_digit(1, 2, (highScore / 10) % 10, COL_YELLOW);
    render_digit(5, 2, highScore % 10, COL_YELLOW);

    render_digit(1, 9, (score / 10) % 10, COL_GREEN);
    render_digit(5, 9, score % 10, COL_GREEN);

    for (f = 0; f < 45; f++) {
        send_frame(gameMode, score);
        delay_ms(60);
    }
}

/* ==========================================================================
 * GAME 1: SPACE COMMANDER (EXACT VERIFIED LOGIC)
 * ========================================================================== */
void space_flash_damage(void) {
    unsigned char i;
    for (i = 0; i < 64; i++) frameBuffer[i] = 0x22; /* Red packed */
    send_frame(1, score);
    delay_ms(80);
    clear_buffer();
    send_frame(1, score);
    delay_ms(40);
}

void play_space_commander(void) {
    unsigned char rawX, currentDir, i, j, activeCount, candidateX, overlap, hit;

    sp_shipX = 3;
    sp_lives = 3;
    score = 0;
    sp_enemyMoveTimer = 0;
    sp_bulletMoveTimer = 0;
    sp_fireCooldown = 0;
    sp_lastJoyDir = 0;
    sp_joyHoldTimer = 0;
    sp_maxAllowedEnemies = 1;
    sp_enemySpeedTicks = 16;
    sp_spawnCooldownTicks = 32;

    for (i = 0; i < MAX_BULLETS; i++) sp_bY[i] = 255;
    for (i = 0; i < MAX_ENEMIES; i++) {
        sp_eActive[i] = 0;
        sp_eTimer[i] = (unsigned char)((i + 1) * 20);
    }

    while (1) {
        if (score < 5) {
            sp_maxAllowedEnemies = 1; sp_enemySpeedTicks = 16; sp_spawnCooldownTicks = 32;
        } else if (score < 12) {
            sp_maxAllowedEnemies = 2; sp_enemySpeedTicks = 13; sp_spawnCooldownTicks = 24;
        } else if (score < 22) {
            sp_maxAllowedEnemies = 3; sp_enemySpeedTicks = 10; sp_spawnCooldownTicks = 18;
        } else {
            sp_maxAllowedEnemies = 4; sp_enemySpeedTicks = 7;  sp_spawnCooldownTicks = 12;
        }

        /* 1. Steering */
        rawX = adc_read(0);
        if (rawX < 95)       currentDir = 1; /* Left */
        else if (rawX > 160) currentDir = 2; /* Right */
        else                 currentDir = 0; /* Center (Parked) */

        if (currentDir == 0) {
            sp_lastJoyDir = 0;
            sp_joyHoldTimer = 0;
        } else {
            if (sp_lastJoyDir != currentDir) {
                if (currentDir == 1 && sp_shipX > 1) sp_shipX--;
                else if (currentDir == 2 && sp_shipX < 6) sp_shipX++;
                sp_lastJoyDir = currentDir;
                sp_joyHoldTimer = 7;
            } else {
                if (sp_joyHoldTimer > 0) {
                    sp_joyHoldTimer--;
                } else {
                    if (currentDir == 1 && sp_shipX > 1) sp_shipX--;
                    else if (currentDir == 2 && sp_shipX < 6) sp_shipX++;
                    sp_joyHoldTimer = 3;
                }
            }
        }

        /* 2. Weapons */
        if (sp_fireCooldown > 0) sp_fireCooldown--;
        if (JOY_SW == 0 && sp_fireCooldown == 0) {
            for (i = 0; i < MAX_BULLETS; i++) {
                if (sp_bY[i] == 255) {
                    sp_bX[i] = sp_shipX;
                    sp_bY[i] = 13;
                    sp_fireCooldown = 6;
                    break;
                }
            }
        }

        /* 3. Bullets */
        sp_bulletMoveTimer++;
        if (sp_bulletMoveTimer >= 2) {
            sp_bulletMoveTimer = 0;
            for (i = 0; i < MAX_BULLETS; i++) {
                if (sp_bY[i] != 255) {
                    if (sp_bY[i] > 1) sp_bY[i]--;
                    else sp_bY[i] = 255;
                }
            }
        }

        /* 4. Enemies Movement & Collision */
        sp_enemyMoveTimer++;
        if (sp_enemyMoveTimer >= sp_enemySpeedTicks) {
            sp_enemyMoveTimer = 0;
            for (i = 0; i < MAX_ENEMIES; i++) {
                if (sp_eActive[i]) {
                    sp_eY[i]++;
                    hit = 0;

                    /* Cockpit Hitbox */
                    if ((sp_eY[i] <= 14 && (sp_eY[i] + 1) >= 14) &&
                        (sp_eX[i] <= sp_shipX && (sp_eX[i] + 1) >= sp_shipX)) {
                        hit = 1;
                    }
                    /* Base Hitbox */
                    else if ((sp_eY[i] + 1 >= 15) &&
                             (sp_eX[i] <= (sp_shipX + 1) && (sp_eX[i] + 1) >= (sp_shipX - 1))) {
                        hit = 1;
                    }

                    if (hit) {
                        sp_eActive[i] = 0;
                        sp_eTimer[i] = sp_spawnCooldownTicks;
                        space_flash_damage();
                        if (sp_lives > 0) sp_lives--;
                        if (sp_lives == 0) {
                            show_game_over(1);
                            return;
                        }
                    } else if (sp_eY[i] >= 15) {
                        sp_eActive[i] = 0;
                        sp_eTimer[i] = sp_spawnCooldownTicks;
                    }
                }
            }
        }

        /* 5. Enemy Spawning */
        activeCount = 0;
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (sp_eActive[i]) activeCount++;
        }
        if (activeCount < sp_maxAllowedEnemies) {
            for (i = 0; i < MAX_ENEMIES; i++) {
                if (!sp_eActive[i]) {
                    if (sp_eTimer[i] > 0) {
                        sp_eTimer[i]--;
                    } else {
                        candidateX = get_random() % 7;
                        overlap = 0;
                        for (j = 0; j < MAX_ENEMIES; j++) {
                            if (sp_eActive[j] && sp_eY[j] <= 3) {
                                if ((candidateX <= sp_eX[j] + 1) && (candidateX + 1 >= sp_eX[j])) {
                                    overlap = 1;
                                    break;
                                }
                            }
                        }
                        if (!overlap) {
                            sp_eActive[i] = 1;
                            sp_eX[i] = candidateX;
                            sp_eY[i] = 1;
                            break;
                        }
                    }
                }
            }
        }

        /* 6. Bullets vs Enemies */
        for (i = 0; i < MAX_BULLETS; i++) {
            if (sp_bY[i] == 255) continue;
            for (j = 0; j < MAX_ENEMIES; j++) {
                if (!sp_eActive[j]) continue;
                if ((sp_bX[i] >= sp_eX[j] && sp_bX[i] <= sp_eX[j] + 1) &&
                    (sp_bY[i] >= sp_eY[j] && sp_bY[i] <= sp_eY[j] + 1)) {
                    sp_bY[i] = 255;
                    sp_eActive[j] = 0;
                    sp_eTimer[j] = sp_spawnCooldownTicks;
                    score++;
                    break;
                }
            }
        }

        /* 7. Render */
        clear_buffer();

        /* HUD: Green Lives */
        for (i = 0; i < sp_lives && i < 3; i++) {
            set_pixel(i * 2, 0, COL_GREEN);
        }

        /* Bullets */
        for (i = 0; i < MAX_BULLETS; i++) {
            if (sp_bY[i] != 255) {
                set_pixel(sp_bX[i], sp_bY[i], COL_MAGENTA);
            }
        }

        /* Enemies */
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (sp_eActive[i]) {
                set_pixel(sp_eX[i],     sp_eY[i],     COL_RED);
                set_pixel(sp_eX[i] + 1, sp_eY[i],     COL_RED);
                set_pixel(sp_eX[i],     sp_eY[i] + 1, COL_RED);
                set_pixel(sp_eX[i] + 1, sp_eY[i] + 1, COL_RED);
            }
        }

        /* Ship */
        set_pixel(sp_shipX,     14, COL_WHITE);
        set_pixel(sp_shipX - 1, 15, COL_WHITE);
        set_pixel(sp_shipX,     15, COL_WHITE);
        set_pixel(sp_shipX + 1, 15, COL_WHITE);

        send_frame(1, score);
        delay_ms(25);
    }
}

/* ==========================================================================
 * GAME 2: SNAKE (EXACT VERIFIED LOGIC)
 * ========================================================================== */
unsigned char snake_is_on_body(unsigned char x, unsigned char y) {
    unsigned char i;
    for (i = 0; i < sn_len; i++) {
        if ((snakeBody[i] & 0x07) == x && ((snakeBody[i] >> 3) & 0x0F) == y) {
            return 1;
        }
    }
    return 0;
}

void snake_spawn_a1(void) {
    unsigned char attempts = 0;
    do {
        sn_a1X = get_random() % 8;
        sn_a1Y = get_random() % 16;
        attempts++;
    } while (snake_is_on_body(sn_a1X, sn_a1Y) && attempts < 20);
}

void snake_spawn_a2(void) {
    unsigned char attempts = 0;
    do {
        sn_a2X = get_random() % 7;
        sn_a2Y = get_random() % 15;
        attempts++;
    } while ((snake_is_on_body(sn_a2X, sn_a2Y) || snake_is_on_body(sn_a2X + 1, sn_a2Y)) && attempts < 20);
    sn_a2Active = 1;
}

void play_snake(void) {
    unsigned char joyX, joyY, headX, headY, i, ate;

    sn_len = 3;
    score = 0;
    sn_moveSpeed = 4;
    sn_moveTick = 0;
    sn_blinkTimer = 0;
    sn_curDir = DIR_UP;
    sn_nextDir = DIR_UP;

    snakeBody[0] = (8 << 3) | 3;
    snakeBody[1] = (9 << 3) | 3;
    snakeBody[2] = (10 << 3) | 3;

    snake_spawn_a1();
    sn_a2Active = 0;

    while (1) {
        joyX = adc_read(0);
        joyY = adc_read(1);

        if (joyX < 75) {
            if (sn_curDir != DIR_RIGHT) sn_nextDir = DIR_LEFT;
        } else if (joyX > 180) {
            if (sn_curDir != DIR_LEFT) sn_nextDir = DIR_RIGHT;
        } else if (joyY < 75) {
            if (sn_curDir != DIR_UP) sn_nextDir = DIR_DOWN;
        } else if (joyY > 180) {
            if (sn_curDir != DIR_DOWN) sn_nextDir = DIR_UP;
        }

        sn_moveTick++;
        if (sn_moveTick >= sn_moveSpeed) {
            sn_moveTick = 0;
            sn_curDir = sn_nextDir;

            headX = snakeBody[0] & 0x07;
            headY = (snakeBody[0] >> 3) & 0x0F;

            if (sn_curDir == DIR_RIGHT)     headX = (headX + 1) & 0x07;
            else if (sn_curDir == DIR_LEFT) headX = (headX == 0) ? 7 : (headX - 1);
            else if (sn_curDir == DIR_DOWN) headY = (headY + 1) & 0x0F;
            else if (sn_curDir == DIR_UP)   headY = (headY == 0) ? 15 : (headY - 1);

            for (i = 0; i < (sn_len - 1); i++) {
                if ((snakeBody[i] & 0x07) == headX && ((snakeBody[i] >> 3) & 0x0F) == headY) {
                    show_game_over(2);
                    return;
                }
            }

            ate = 0;
            if (headX == sn_a1X && headY == sn_a1Y) {
                ate = 1;
                score++;
                snake_spawn_a1();
                if (!sn_a2Active && (get_random() % 4 == 0)) {
                    snake_spawn_a2();
                }
            }

            if (sn_a2Active) {
                if ((headX >= sn_a2X) && (headX <= (sn_a2X + 1)) &&
                    (headY >= sn_a2Y) && (headY <= (sn_a2Y + 1))) {
                    ate = 2;
                    score += 3;
                    sn_a2Active = 0;
                }
            }

            if (ate > 0 && sn_len < 20) {
                sn_len++;
            }

            for (i = sn_len - 1; i > 0; i--) {
                snakeBody[i] = snakeBody[i - 1];
            }
            snakeBody[0] = (headY << 3) | headX;

            if (score >= 9)      sn_moveSpeed = 2;
            else if (score >= 4) sn_moveSpeed = 3;
            else                 sn_moveSpeed = 4;
        }

        clear_buffer();
        set_pixel(snakeBody[0] & 0x07, (snakeBody[0] >> 3) & 0x0F, COL_CYAN);
        for (i = 1; i < sn_len; i++) {
            set_pixel(snakeBody[i] & 0x07, (snakeBody[i] >> 3) & 0x0F, COL_GREEN);
        }

        if ((sn_blinkTimer & 0x10) == 0) {
            set_pixel(sn_a1X, sn_a1Y, COL_RED);
        }

        if (sn_a2Active && ((sn_blinkTimer & 0x08) == 0)) {
            set_pixel(sn_a2X,     sn_a2Y,     COL_ORANGE);
            set_pixel(sn_a2X + 1, sn_a2Y,     COL_ORANGE);
            set_pixel(sn_a2X,     sn_a2Y + 1, COL_ORANGE);
            set_pixel(sn_a2X + 1, sn_a2Y + 1, COL_ORANGE);
        }

        sn_blinkTimer++;
        send_frame(2, score);
        delay_ms(25);
    }
}

/* ==========================================================================
 * GAME 3: TETRIS (EXACT VERIFIED LOGIC)
 * ========================================================================== */
unsigned char tetris_check_collision(signed char px, signed char py, unsigned char piece, unsigned char rot) {
    unsigned char i;
    for (i = 0; i < 4; i++) {
        signed char bx = px + PIECES[piece][rot][i][0];
        signed char by = py + PIECES[piece][rot][i][1];

        if (bx < 0 || bx >= 8 || by >= 16) return 1;
        if (by < 0) continue;
        if (get_pixel((unsigned char)bx, (unsigned char)by) != COL_OFF) return 1;
    }
    return 0;
}

void tetris_stamp(signed char px, signed char py, unsigned char piece, unsigned char rot, unsigned char col) {
    unsigned char i;
    for (i = 0; i < 4; i++) {
        signed char bx = px + PIECES[piece][rot][i][0];
        signed char by = py + PIECES[piece][rot][i][1];
        if (bx >= 0 && bx < 8 && by >= 0 && by < 16) {
            set_pixel((unsigned char)bx, (unsigned char)by, col);
        }
    }
}

void tetris_check_lines(void) {
    signed char y, x, sy;
    unsigned char full, cleared = 0;

    for (y = 15; y >= 0; y--) {
        full = 1;
        for (x = 0; x < 8; x++) {
            if (get_pixel(x, (unsigned char)y) == COL_OFF) {
                full = 0; break;
            }
        }

        if (full) {
            cleared++;
            for (x = 0; x < 8; x++) set_pixel(x, (unsigned char)y, COL_WHITE);
            send_frame(3, score);
            delay_ms(50);

            /* Collapse rows above down: 4 bytes per row */
            for (sy = y; sy > 0; sy--) {
                for (x = 0; x < 4; x++) {
                    frameBuffer[(sy << 2) + x] = frameBuffer[((sy - 1) << 2) + x];
                }
            }
            for (x = 0; x < 4; x++) frameBuffer[x] = 0;
            y++;
        }
    }

    if (cleared == 1) score += 1;
    else if (cleared == 2) score += 3;
    else if (cleared == 3) score += 6;
    else if (cleared >= 4) score += 10;

    if (score >= 20 && tt_fallSpeed > 4)      tt_fallSpeed = 5;
    else if (score >= 12 && tt_fallSpeed > 6) tt_fallSpeed = 7;
    else if (score >= 6 && tt_fallSpeed > 8)  tt_fallSpeed = 9;
}

unsigned char tetris_spawn(void) {
    tt_curPiece = get_random() % 7;
    tt_curRot = 0;
    tt_pieceX = 3;
    tt_pieceY = 1;
    return !tetris_check_collision(tt_pieceX, tt_pieceY, tt_curPiece, tt_curRot);
}

void play_tetris(void) {
    unsigned char joyX, joyY, btn, nextRot;

    clear_buffer();
    score = 0;
    tt_fallTimer = 0;
    tt_fallSpeed = 12;
    tt_moveRepeatTimer = 0;
    tt_lastBtnState = 1;

    if (!tetris_spawn()) {
        show_game_over(3);
        return;
    }

    while (1) {
        joyX = adc_read(0);
        joyY = adc_read(1);
        btn  = JOY_SW;

        if (btn == 0 && tt_lastBtnState == 1) {
            nextRot = (tt_curRot + 1) & 0x03;
            if (!tetris_check_collision(tt_pieceX, tt_pieceY, tt_curPiece, nextRot)) {
                tt_curRot = nextRot;
            } else if (!tetris_check_collision(tt_pieceX - 1, tt_pieceY, tt_curPiece, nextRot)) {
                tt_pieceX--; tt_curRot = nextRot;
            } else if (!tetris_check_collision(tt_pieceX + 1, tt_pieceY, tt_curPiece, nextRot)) {
                tt_pieceX++; tt_curRot = nextRot;
            }
        }
        tt_lastBtnState = btn;

        if (joyX < 75) {
            if (tt_moveRepeatTimer == 0) {
                if (!tetris_check_collision(tt_pieceX - 1, tt_pieceY, tt_curPiece, tt_curRot)) {
                    tt_pieceX--;
                }
                tt_moveRepeatTimer = 4;
            } else {
                tt_moveRepeatTimer--;
            }
        } else if (joyX > 180) {
            if (tt_moveRepeatTimer == 0) {
                if (!tetris_check_collision(tt_pieceX + 1, tt_pieceY, tt_curPiece, tt_curRot)) {
                    tt_pieceX++;
                }
                tt_moveRepeatTimer = 4;
            } else {
                tt_moveRepeatTimer--;
            }
        } else {
            tt_moveRepeatTimer = 0;
        }

        if (joyY < 75) {
            tt_fallTimer = tt_fallSpeed;
        }

        tt_fallTimer++;
        if (tt_fallTimer >= tt_fallSpeed) {
            tt_fallTimer = 0;
            if (!tetris_check_collision(tt_pieceX, tt_pieceY + 1, tt_curPiece, tt_curRot)) {
                tt_pieceY++;
            } else {
                tetris_stamp(tt_pieceX, tt_pieceY, tt_curPiece, tt_curRot, PIECE_COLORS[tt_curPiece]);
                tetris_check_lines();
                if (!tetris_spawn()) {
                    show_game_over(3);
                    return;
                }
            }
        }

        tetris_stamp(tt_pieceX, tt_pieceY, tt_curPiece, tt_curRot, PIECE_COLORS[tt_curPiece]);
        send_frame(3, score);
        tetris_stamp(tt_pieceX, tt_pieceY, tt_curPiece, tt_curRot, COL_OFF);
        delay_ms(25);
    }
}

/* ==========================================================================
 * CAROUSEL MENU & ANIMATED PREVIEWS
 * ========================================================================== */
void render_carousel_menu(void) {
    unsigned char frameAnim, pY;
    clear_buffer();

    if (menuSel == 0) {
        set_pixel(3, 11, COL_WHITE);
        set_pixel(2, 12, COL_WHITE);
        set_pixel(3, 12, COL_WHITE);
        set_pixel(4, 12, COL_WHITE);

        if ((menuAnim & 0x02) == 0) set_pixel(3, 13, COL_CYAN);
        else                        set_pixel(3, 13, COL_YELLOW);

        pY = 2 + ((menuAnim >> 2) % 6);
        set_pixel(3, pY,     COL_RED);
        set_pixel(4, pY,     COL_RED);
        set_pixel(3, pY + 1, COL_RED);
        set_pixel(4, pY + 1, COL_RED);

        set_pixel(3, 10 - ((menuAnim * 2) % 7), COL_MAGENTA);

    } else if (menuSel == 1) {
        frameAnim = (menuAnim >> 2) % 8;
        if (frameAnim == 0) {
            set_pixel(4, 5, COL_CYAN);  set_pixel(3, 5, COL_GREEN); set_pixel(2, 5, COL_GREEN); set_pixel(2, 6, COL_GREEN);
        } else if (frameAnim == 1) {
            set_pixel(5, 5, COL_CYAN);  set_pixel(4, 5, COL_GREEN); set_pixel(3, 5, COL_GREEN); set_pixel(2, 5, COL_GREEN);
        } else if (frameAnim == 2) {
            set_pixel(5, 6, COL_CYAN);  set_pixel(5, 5, COL_GREEN); set_pixel(4, 5, COL_GREEN); set_pixel(3, 5, COL_GREEN);
        } else if (frameAnim == 3) {
            set_pixel(5, 7, COL_CYAN);  set_pixel(5, 6, COL_GREEN); set_pixel(5, 5, COL_GREEN); set_pixel(4, 5, COL_GREEN);
        } else if (frameAnim == 4) {
            set_pixel(4, 7, COL_CYAN);  set_pixel(5, 7, COL_GREEN); set_pixel(5, 6, COL_GREEN); set_pixel(5, 5, COL_GREEN);
        } else if (frameAnim == 5) {
            set_pixel(3, 7, COL_CYAN);  set_pixel(4, 7, COL_GREEN); set_pixel(5, 7, COL_GREEN); set_pixel(5, 6, COL_GREEN);
        } else if (frameAnim == 6) {
            set_pixel(2, 7, COL_CYAN);  set_pixel(3, 7, COL_GREEN); set_pixel(4, 7, COL_GREEN); set_pixel(5, 7, COL_GREEN);
        } else {
            set_pixel(2, 6, COL_CYAN);  set_pixel(2, 7, COL_GREEN); set_pixel(3, 7, COL_GREEN); set_pixel(4, 7, COL_GREEN);
        }

        if ((menuAnim & 0x08) == 0) set_pixel(3, 6, COL_RED);

    } else {
        set_pixel(0, 13, COL_ORANGE); set_pixel(1, 13, COL_ORANGE);
        set_pixel(6, 13, COL_GREEN);  set_pixel(7, 13, COL_GREEN);
        set_pixel(0, 12, COL_ORANGE);

        pY = 2 + ((menuAnim >> 2) % 9);
        set_pixel(3, pY,     COL_MAGENTA);
        set_pixel(2, pY + 1, COL_MAGENTA);
        set_pixel(3, pY + 1, COL_MAGENTA);
        set_pixel(4, pY + 1, COL_MAGENTA);
    }

    set_pixel(2, 15, (menuSel == 0) ? COL_WHITE : COL_CYAN);
    set_pixel(4, 15, (menuSel == 1) ? COL_WHITE : COL_CYAN);
    set_pixel(6, 15, (menuSel == 2) ? COL_WHITE : COL_CYAN);

    send_frame(0, 0);
}

/* ==========================================================================
 * MAIN ENTRY POINT & SYSTEM DISPATCHER
 * ========================================================================== */
void main(void) {
    unsigned char joyX;

    ADC_ALE = 0;
    ADC_START = 0;
    ADC_OE = 0;
    JOY_SW = 1;

    /* ESP8266 Cold Boot Clamp */
    P3_0 = 0;
    delay_ms(600);
    P3_0 = 1;

    init_hw_clock();
    uart_init();

    menuSel = 0;
    menuAnim = 0;
    menuHold = 0;
    menuLastDir = 0;

    while (1) {
        joyX = adc_read(0);

        if (joyX < 75) {
            if (menuLastDir != 1) {
                if (menuSel == 0) menuSel = 2;
                else menuSel--;
                menuLastDir = 1;
                menuHold = 8;
            } else if (menuHold > 0) {
                menuHold--;
            } else {
                if (menuSel == 0) menuSel = 2;
                else menuSel--;
                menuHold = 5;
            }
        } else if (joyX > 180) {
            if (menuLastDir != 2) {
                if (menuSel == 2) menuSel = 0;
                else menuSel++;
                menuLastDir = 2;
                menuHold = 8;
            } else if (menuHold > 0) {
                menuHold--;
            } else {
                if (menuSel == 2) menuSel = 0;
                else menuSel++;
                menuHold = 5;
            }
        } else {
            menuLastDir = 0;
            menuHold = 0;
        }

        if (JOY_SW == 0) {
            delay_ms(150);
            while (JOY_SW == 0); /* Wait for switch release */
            delay_ms(100);

            if (menuSel == 0)      play_space_commander();
            else if (menuSel == 1) play_snake();
            else if (menuSel == 2) play_tetris();

            menuLastDir = 0;
            menuHold = 0;
        }

        menuAnim++;
        render_carousel_menu();
        delay_ms(30);
    }
}





    
