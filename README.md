# game-girl-advanced
irony is that it is more basic than the game-girl we built earlier but anyway , enjoy

## Code walkthrough: lighting the matrix, line by line

This is the 8051 program that lit the whole screen blue. Every line is explained. Read it once top to bottom, then use it as a template.

### The chain of command (big picture)

```
 your game code
    |  decides which pixels are lit, counts them (N)
    v
 pkt_start(N) --> pkt_pixel(x, y, colour) x N --> pkt_end()
    |  each call sends bytes out of the 8051 through tx()
    v
 UART wire (P3.1 TXD, 57600 baud)
    v
 ESP8266 feed() state machine: wait AA -> read N -> read pairs -> checksum -> end 55
    v
 applyPacket() -> frameBuf[] (one colour per pixel) -> render() -> matrix.show()
    v
 128 WS2812 LEDs light up
```

Example: pixel (0, 0) green and pixel (7, 15) red. That is N = 2 and these bytes on the wire:

| Byte | Meaning |
|---|---|
| `AA` | Start marker |
| `02` | N = 2 pixels |
| `00 02` | pos 0 (y*8+x = 0), colour 2 (green) |
| `7F 01` | pos 127 (15*8+7), colour 1 (red) |
| `7E` | Checksum: 02 xor 00 xor 02 xor 7F xor 01 |
| `55` | End marker |

That is 8 bytes, about 1.4 ms at 57600 baud.

### The code, every line explained

```c
#include <8051.h>      // names for the 8051's special registers: SBUF, TI, TMOD, TH1, ...
#include <stdint.h>    // fixed-size types: uint8_t = 1 byte (0..255)

static uint8_t cs;     // running checksum. "static" = private to this file; lives in RAM all the time

void uart_init(void)               // sets up the serial port. Call once at start
{
    TMOD &= 0x0F;                  // clear the Timer 1 half of TMOD, keep the Timer 0 half untouched
    TMOD |= 0x20;                  // Timer 1 in mode 2: 8-bit counter that reloads itself
    TH1 = 0xFF;                    // reload value 255. Counting only 1 step gives the fastest baud
    TL1 = 0xFF;                    // starting value, so the first tick is immediate
    PCON |= 0x80;                  // SMOD = 1: doubles the baud rate. Now 11.0592 MHz gives exactly 57600
    TR1 = 1;                       // start Timer 1 (it is the baud-rate clock)
    SCON = 0x50;                   // serial mode 1 (8-bit UART, variable baud) with receive enabled
}

void tx(uint8_t b)                 // send ONE byte out of the TX pin
{
    SBUF = b;                      // writing to SBUF starts the hardware sending
    while (!TI);                   // wait until TI (transmit-done flag) becomes 1. About 174 us per byte
    TI = 0;                        // clear the flag, ready for the next byte
}

void txc(uint8_t b)                // send one byte AND include it in the checksum
{
    cs ^= b;                       // xor the byte into the running checksum
    tx(b);                         // then send it
}

void pkt_start(uint8_t n)          // begin a packet that will hold n pixels
{
    tx(0xAA);                      // start marker. Not part of the checksum
    cs = 0;                        // reset the checksum for this packet
    txc(n);                        // send the pixel count (and add it to the checksum)
}

void pkt_pixel(uint8_t x, uint8_t y, uint8_t colour)   // add one pixel to the packet
{
    txc((uint8_t)((y << 3) | x));  // position = y*8 + x. "y << 3" is y*8; x is 0..7 so "|" just adds it
    txc(colour);                   // colour index 0..9 (see the palette table)
}

void pkt_end(void)                 // finish the packet
{
    tx(cs);                        // send the checksum so the ESP8266 can detect damaged packets
    tx(0x55);                      // end marker
}

void delay_ms(unsigned int ms)     // rough delay. Not exact: measure it if timing matters
{
    unsigned int i, j;
    for (i = 0; i < ms; i++)       // repeat once per millisecond
        for (j = 0; j < 120; j++); // empty loop that wastes about 1 ms at 11.0592 MHz
}

void main(void)
{
    uint8_t x, y;                  // loop counters for column and row
    uart_init();                   // 1) set up the serial port
    while (1)                      // 2) repeat forever: one pass = one frame
    {
        pkt_start(8 * 16);         //    we will send 128 pixels, so N = 128
        for (x = 0; x < 8; x++)            // every column...
            for (y = 0; y < 16; y++)       // ...and every row
                pkt_pixel(x, y, 3);        // pixel (x, y) in colour 3 = blue
        pkt_end();                 //    checksum + end marker
        delay_ms(100);             //    wait before the next frame
    }
}
```

### Rules the protocol depends on

1. **N must equal the number of `pkt_pixel()` calls** between `pkt_start()` and `pkt_end()`. A wrong N means the ESP8266 never accepts the packet.
2. x must be 0..7, y must be 0..15, colour must be 0..9. Anything else is rejected.
3. Do not write `;` at the end of a `#define` colour (`#define RED 1;` breaks the code).
4. Send exactly one packet per frame. The ESP8266 redraws the whole screen once per packet.
5. Keep the lit pixels few. At 57600 baud a packet takes about 174 us per byte, so 100 lit pixels is about 35 ms.

### How to build your game on top of it

Every game uses the same loop. You only write `read_input()`, `game_update()` and `game_draw()`:

```c
void main(void)
{
    uart_init();
    TMOD |= 0x01;          // Timer 0 free-running: TL0 changes constantly, use it as a random number
    TR0 = 1;
    game_init();           // set the starting positions and score
    while (1)
    {
        read_input();      // joystick (fake it with a counter until the ADC driver exists)
        game_update();     // move things, check collisions, change the score
        game_draw();       // send the packet (below)
        delay_ms(100);     // frame pacing: bigger = slower game
    }
}
```

**Smallest possible example: a dot that sweeps across the screen.**

```c
static uint8_t px = 0, py = 0;

void game_update(void)
{
    px++;                              // move right
    if (px > 7) { px = 0; py = (py + 1) & 15; }   // wrap to the next row, and back to the top after row 15
}

void game_draw(void)
{
    pkt_start(1);                      // one pixel, so N = 1
    pkt_pixel(px, py, 3);              // draw it in blue
    pkt_end();
}
```

**A Snake-style draw function.** Count first, then send:

```c
void game_draw(void)
{
    uint8_t i;
    pkt_start(snake_len + 1);                          // N = snake body cells + 1 food
    for (i = 0; i < snake_len; i++)
        pkt_pixel(snake_x[i], snake_y[i], i == 0 ? 6 : 5);   // head light blue, body dark blue
    pkt_pixel(food_x, food_y, 1);                      // food red
    pkt_end();
}
```

Only the **lit** pixels go into the packet. With sparse-frame mode on (see above), everything you do not send is off, so you never have to "erase" a pixel: if a pixel is no longer in the list, it goes dark on the next frame.

### If it does not work

| What you see | Likely cause |
|---|---|
| Nothing lights, ESP8266 onboard LED stays on solid | N does not match the number of pixels, or the baud rates differ |
| Nothing lights, onboard LED never turns on | Wire from 8051 TX not reaching D7, ground not shared, or the program is not running |
| Random wrong colours | Check the `#define` colour numbers and that you reflashed both chips |
| Rows look swapped or mirrored | The `SERPENTINE` setting in the ESP8266 code |
| ESP8266 restarts, a white dot sweeps again | Power or wiring problem (see Known issues) |
