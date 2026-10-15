// 8051 test for the ESP8266 receiver. Build: sdcc --model-small test_fixed.c
// Sends ONE packet per frame:  0xAA, N, N x (pos, colour), XOR checksum, 0x55
//   pos = y * 8 + x, colour = palette index 0..9
// The packet is streamed byte by byte, so no RAM buffer is needed.
/*  PAL[0] = matrix.Color(0, 0, 0);        // off
  PAL[1] = matrix.Color(255, 0, 0);      // red
  PAL[2] = matrix.Color(0, 255, 0);      // green
  PAL[3] = matrix.Color(0, 0, 255);      // blue
  PAL[4] = matrix.Color(0, 255, 255);    // cyan
  PAL[5] = matrix.Color(0, 10, 128);     // dark blue
  PAL[6] = matrix.Color(89, 89, 255);    // light blue
  PAL[7] = matrix.Color(255, 255, 0);    // yellow
  PAL[8] = matrix.Color(50, 50, 50);     // dim white
  PAL[9] = matrix.Color(255, 0, 255);    // magenta*/

#include <8051.h>
#include <stdint.h>
#define BLACK 0;
#define RED 1;
#define GREEN 2;
#define BLUE 3;
#define CYAN 4;
#define DARKBLUE 5;
#define LIGHTBLUE 6;
#define YELLOW 7;
#define GREY 8;
#define MAGENTA 9;


static uint8_t cs;   // running XOR checksum

void uart_init(void)
{
    TMOD &= 0x0F;
    TMOD |= 0x20;    // Timer 1, mode 2 (8-bit auto-reload)
    TH1 = 0xFF;      // 57600 baud with SMOD = 1 at 11.0592 MHz
    TL1 = 0xFF;
    PCON |= 0x80;    // SMOD = 1
    TR1 = 1;
    SCON = 0x50;     // mode 1, 8-bit UART
}

void tx(uint8_t b) // transmitter function
{
    SBUF = b;    // we store our data here in sbuffer
    while (!TI);  //until its's not over
    TI = 0;      //now reset
}


//txc is checksum to detect errors , now xor will produce 0 if same bits , therefore it will show 1 for different bits
void txc(uint8_t b)
{   // send a byte that counts towards the checksum
    cs ^= b; // ^= is XOR
    tx(b);  // send the byte
}

void pkt_start(uint8_t n)
{
    tx(0xAA); //header , like this will indicate the start of our packet
    cs = 0; // sequential checksum = 0 at beginning
    txc(n);  // send txc
}

void pkt_pixel(uint8_t x, uint8_t y, uint8_t colour)
{
    txc((uint8_t)((y << 3) | x));  //pos = y*8 + x because x max value is 8 ,so it will do moduo operation in 8266
    txc(colour);  //send colour
}

void pkt_end(void) 
{
    tx(cs);
    tx(0x55); //this is the end.
}

void delay_ms(unsigned int ms)  //actually a very interesting solution for delay
{
    //since loop decrement , conditional check and jump take 10 machine cycles
    unsigned int i, j;
    for (i = 0; i < ms; i++) //no of ms
        for (j = 0; j < 120; j++);  // since using 12 MhZ
            //120 iterations * 10.85 us = 1302 us = 1.3ms , decent enough approximation
}

/*
//write game functions here and just use them in the main while (1)

typedef struct
{
    unsigned char id;
    unsigned char x:3;
    unsigned char y:4;
    unsigned char active:1;
    //const unsigned char colour 1; //red    
}enemy_t;

typedef struct
{
    unsigned char x:3;
    unsigned char y:4;
    unsigned char active:1;
    //const unsigned char colour 7; //yellow
}bullet_t;

typedef struct
{
    unsigned char x:3;
    unsigned char y:4;
    //const unsigned char colour 9; //magenta
}ship_t;

unsigned char game_status; //1 = running 0 = over
unsigned char score = 0;


void check_kill( enemy_t *e , bullet_t *b )
{
    if(e->active&&b->active)
    {
        if((e->x == b->x)&&(e->y == b->y))
        {
            b->active = 0;
            e->active = 0;
            score++;
        }
    }
}

void check_game_over(enemy_t *e)
{
    if(e->y > 13)
    {
        game_status = 0; // game over
    }
    //game continue
}

//enemy spawning put in main only dont need a function
//if (no_of_enemy < 5) : spawn enemy with y = 0 and x= rand()%8

enemy_t enemies[5];

unsigned char no_duplicated(unsigned char x)
{
    for(unsigned char i = 0; i< 5; i++)
    {
        if(enemies[i].active && (enemies[i].x == x))
        {
            return 1;
        }
    }
    return 0;
}

void spawn_enemy(void)
{
    unsigned char i , new_x , attempts = 0;

    for(i = 0; i < 5 ; i++)
    {
        if(!enemies[i].active)
        {
            do
            {
                new_x = (unsigned char)(TL0%8);
                attempts++;
            }
            while (no_duplicated(new_x)  && attempts<10);

            enemies[i].id = i;
            enemies[i].x = new_x;
            enemies[i].y = 0;
            enemies[i].active = 1;
            break;
        }
    }
}
*/
void main(void)
{
    uint8_t x, y;  //initalise x and y
    uart_init();   //init timer and uart
    while (1)
    {
        pkt_start(8 * 16);                 // 8 columns x 16 rows = 128 pixels
        //unsigned char count_enemies = 0;
        for (x = 0; x < 8; x++)
            for (y = 0; y < 1; y++)
                pkt_pixel(x, y, 3);        // colour 3 = blue
        pkt_end();
        delay_ms(100);                     // pace the frames
    }
}
