# game-girl-advanced
irony is that it is more basic than the game-girl we built earlier but anyway , enjoy



GAME GIRL ADVANCED! GAME GIRL ADVANCED! ゲームガール・アドバンスド ! ゲームガール・アドバンスド ! <mark>J</mark> ~~<u><mark>sfe —</mark></u>~~ Group 9 Microprocessors and Microcontrollers M2026 <mark>-</mark> Group 9 Microprocessors and Microcontrollers M2026 

## TABLE OF CONTENTS TABLE OF CONTENTS 

01 Intro Information about the project and team member contributions 

03 Software Code , Debugging and error explanation . Boot up Process 

02 Hardware All of the components used and their description 

## 04 

Hurdles ADC Interfacing , Display timing , Yaw and Drift. Timing errors 

<mark>0</mark> ~~<mark>1</mark>~~ <mark>0</mark> ~~<mark>1</mark>~~ 

Int <mark>roductio</mark> n Int <mark>roductio</mark> n 

Project Details and Team Members Project Details and Team Members 



## Introduction Introduction 



# The Game Girl Advanced is a Game Arcade based on the Classic 8051 Microcontroller (AT89S52 ) . It can run Three Games - Snake Xenia ( Rishitha) - Space Commander (Sripathy) - Tetris (Venkat) We made this so that we can play games on the easiest hardware available to use. Shown are the previews of game rendered in Figma built to scale. Snake Xenia Space Commander TETRIS | ~~<u><mark>Re</mark></u>~~ 

## TEAM Members TEAM Members 

Rishitha Group Leader and Principal Engineer Programmed Snake Xenia S20240020325 

Sripathy Hardware and Software Engineer Programmed Space Commander S20240020332 

Venkat Structural Integrity Engineer Programmed Tetris S20240020314 

<mark>0</mark> ~~<mark>2</mark>~~ <mark>0</mark> ~~<mark>2</mark>~~ 

H <mark>ardware</mark> H <mark>ardware</mark> 

Component Selection and Working Component Selection and Working 

Microcontrollers Microcontrollers ~~<u><mark>FY</mark></u>~~ <mark>| Ss SE</mark> ~~<mark>Ee</mark>~~ ~~<u><mark>s</mark></u>~~ <u><mark>e</mark></u> 1 ae <mark>"|</mark> Beet fae ee!74 | <mark>ete:I S</mark> S Deeos “s Buy eee,URS Oe45°. SS ED2 So Ca? S one Cass NE come @, AT89S52 ESP8266 

ESP8266 The Display Driver which has a 26MHz clock , which is necessary for the WS2812B Matrix display. It also houses the EEPROM , which stores the high scores. 

The main controller,which interfaces the ADC,Joystick and computes all the required data to be sent to the Display Driver. 



<!-- Start of picture text -->
ia] ooo"<br>ow S|<br><!-- End of picture text -->

Peripherals PIG ~~<u><mark>FY</mark></u>~~ Peripherals | <mark>Dl ———</mark> |! <mark>| i ee</mark> | <mark>ee La i= |</mark> 1 aeae <mark>eeeee</mark> . « Joystick WS2812B aeae <mark>eeeee</mark> Ney fig eaeeeeee SANS, aeeeeeee oY Human Interface Device 128 pixel Full Colour Pee for playing the game Gamma Corrected display peaeeaass BESEEEES oe Baeeeees PS‘ CORN * ADC0809 Been."aN <mark>‘Ss.</mark> a ASP8051 Poaeceagryre<sup>$3344.</sup> pero as r ae °° <mark>aS.</mark> SEEREEEe Y * Analog to Digital Converter he > o sd Programmer and Power cry 2 + Ses ai bball «3 for both X and Y Channel <6) = Delivery for the 8051 dS oh Rad 

ASP8051 Programmer and Power Delivery for the 8051 





Block Diagram Block Diagram ~~<u><mark>Lo</mark></u>~~ i | <mark>Da</mark> ee <mark>ee a iEE</mark> .7 a — <mark>—</mark> — <mark>— o</mark> se <i es ¢ > ISP Joystick ADC0809 Gives power from the ADC takes two channels User Analog Input to wall and programs the input and gives out the 8 control the Game 8051 bit Digital Equivalent 

Joystick ADC0809 ADC takes two channels User Analog Input to input and gives out the 8 control the Game bit Digital Equivalent 



<!-- Start of picture text -->
Burret<br><!-- End of picture text -->



<!-- Start of picture text -->
eoeecess)<br><!-- End of picture text -->

WS2 <mark>812</mark> B 2X64bit Cascaded LED Matrix display that supports full colour 

ESP8266 Display Retimer. WS2812B needs minimum of 24Mhz. EEPROM 

8051 Boss of the Operation. Does all the compute in house and gives it to ESP for display retiming 

<mark>0</mark> ~~<mark>3</mark>~~ <mark>0</mark> ~~<mark>3</mark>~~ 

S <mark>oftware</mark> S <mark>oftware</mark> 

Embedded C and C++ Embedded C and C++ 

Timeline Timeline Se <mark>pt</mark> Oct Space Commander Tetris Snake Xenia 

Nov 



### SPACE COMMANDER - BLOCK DIAGRAM SPACE COMMANDER - BLOCK DIAGRAM 

#### 1. INIT 

#### 2. STEERING 

3. COOLDOWN Decrement fire timer each tick; spawn bullet & reload cooldown only when fire=1 and timer==0. 

4. BULLET PHYS Step active bullets up <mark>ward</mark> (y--) per frame; deactivate bullets exiting top edge (y < 0). 

Reset ship X/Y, clear LED buffer, zero score & cooldown, seed PRNG, clear enemy & bullet arrays. 

Poll 8-bit ADC0809 X-axis via AT89S52; map analog threshold to move ship left/right in bounds. 

#### 8. HUD & RENDER 

7. BULLET HIT Test active bullets vs enemy hitboxes; on overlap deactivate both, score++ & trigger flash. 

#### 6. ENEMY HITBOX 

#### 5. ENEMY SPAWN 

Decrement spawn counter; at 0 spawn enemy at random X (y=0), set active=1 & reset interval. 

Advance enemies down (y+ +); check enemy hitbox vs ship hitbox or bottom breach -> lose life. 

Draw ship, bullets, enemies & score/lives HUD to buffer; stream frame to ESP8266 WS2812B. 

### SNAKE XENIA - BLOCK DIAGRAM SNAKE XENIA - BLOCK DIAGRAM 

1. MOVEMENT DIR Sample joystick X/Y ADC channels; update heading (U/D/L/R) while blocking 180-deg self-reversal. 

6. LED RENDERING Write snake head, body, and A1/A2 pixels into buffer; stream frame to ESP8266 WS2812B driver. 

2. COLLISION Compute next head (nx, ny); test wall bounds and self-body segments 1..len-1 -> Game Over on hit. 

5. SPEED ADJUST Decrease loop tick delay as score and snake length increase, clamping at minimum frame interval. 

3. EATING & GROW If head matches A1 or A2: score+ <mark>+,</mark> keep tail to grow length; else shift body array & clear tail. 

4. FOOD A1 / A2 

Spawn primary food A1 & bonus food A2 at random unoccupied cells (!= snake body) after eat/timer. 

### TETRIS - BLOCK DIAGRAM TETRIS - BLOCK DIAGRAM 

1. GAME LOOP 2. ROTATE & MOVE Superloop: poll ADC/buttons, tick Compute candidate (x+dx, y) or rotated gravity, process move/rotate, stamp & 4x4 matrix; test canPlace() vs walls & stream frame to ESP8266. locked cells first. 

#### 6. SCORE & LEVEL 

#### 5. LINE COLLAPSE 

Scan board rows bottom-to-top; clear each full row, shift all upper rows down by 1 & count lines. 

Award points by lines cleared (1-4 bonus), increment level/lines & tighten gravity_delay. 

3. GRAVITY TIMING Increment tick counter each loop; <mark>when</mark> tick >= gravity_delay, attempt downward step (y+1) & reset. 

#### 4. STAMPING 

When !canPlace(x, y+1), stamp 4x4 piece into board grid, spawn next tetromino, check top-out. 





2 <mark>mont</mark> h ~~s~~ 15days 2m <mark>onths 15d</mark> ays Time Spent <mark>Tim</mark> e Spent <mark>591</mark> 8 Bytes <mark>5918</mark> Bytes Of code in the Flash Of code in the Flash <mark>200</mark> / <mark>256 By</mark> tes <mark>200</mark> /2 <mark>56 B</mark> ytes RAM used RAM used 



<mark>0</mark> ~~<mark>4</mark>~~ <mark>0</mark> ~~<mark>4</mark>~~ 

Encount <mark>ered Pro</mark> blems Encount <mark>ered Pro</mark> blems Difficulties faced and how we overcame them Difficulties faced and how we overcame them 





||Problems Faced<br>Problems Faced||
|---|---|---|
|Problem|Description|Time Taken|
|ADC|28 Pin Multi Channel ADC was difficult to interface.<br>We had no cock source as NE555 was too noisy<br>for an ADC and the 1MHz clock from 8051 was too<br>high.|2 months|
|UART|Finding a common BAUD rate which can satisfy<br>sampling and polling rates and at the same time<br>avoiding Bit Banging was also essential|3 weeks|
|CLOCK|WS2812B news a minimum of 24 MHz Clock  but<br>the 8051 has a Clock speed of 11.082 MHz.<br>Frequency multipliers were out of question and we<br>had to resort to using a slave retimer|1 month|





Project Images Project Images 4 ~~<u><mark>LO</mark></u>~~ | <mark>———</mark> |! <mark>DO e</mark> e <mark>ee</mark> <u><mark>QI, EEE</mark></u> hy 1 



<!-- Start of picture text -->
i<br>7 1? ieee |<br>Ss S a e<br><!-- End of picture text -->

| ~~<u><mark>eel</mark></u>~~ <u><mark>ey</mark></u> <mark>Thank</mark> ~~<mark>You</mark> !~~ <mark>Thank You! 有り難う御座い</mark> ま <mark>有り難う御座い</mark> ま <mark>す ! す !</mark> ~~<mark>a</mark>~~ e <mark>y</mark> 





<!-- Start of picture text -->
on<br><!-- End of picture text -->

