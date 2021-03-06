
Keyboard 
============
PS/2 keyboard with own firmware based on tmk_keyboard and ps2avr.
Diode per key charliplexed keyboard matrix have no ghosting and support NKRO.

Microcontroller
=================
Used ATmega328p from Arduino, but TQFP version, with ability
to connect A6 and A7. Antighosting reference diode with level shift
resisor(2.7k) connected to AIN0(+)=D6:

(D6)---\/\/\--->|--GND

Bus pinout (Arduino):

  BUS0 PC0 (A0)
  BUS1 PC1 (A1)
  BUS2 PC2 (A2)
  BUS3 PC3 (A3)
  BUS4 PC4 (A4)
  BUS5 PC5 (A5)
  BUS6 PD3+A6 (D3)
  BUS7 PD2+A7 (D2)

optional for 9x8 matrix:

  BUS8 PD7 (D7)

BUS8 does not implemented in firmware, but it is easy to do.


Wiring
============

Matrix of 7x8 has 56 commutation points was connected with 58 key switches.
Control and Shift keyswitches from left and right connected together.

    /* Keymap 0: Default Layer
     * ,-----------------------------------------------------------.
     * |Esc|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|  \|  `|
     * |-----------------------------------------------------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|BckSp|
     * |-----------------------------------------------------------|
     * |Ctrl  |  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|Return  |
     * |-----------------------------------------------------------|
     * |Shift   |  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /|Shift     |
     * `-----------------------------------------------------------'
     *           | Alt |         Space             |Ctrl |FN|
     *           `------------------------------------------'
     */

  A key switch connected this manner:
                    |
   -----o---------------       --+-----|-        -------|
        |           |       =    `-KEY-+    =       `KEY|
         `-|<---/ --o                  |
                    |

  Buses with same number are connected:
      0   1   2   3   4   5   6   7     0   1   2   3   4   5   6   7
 -----o---|---|---|---|---|---|---|  ---|---|---|---|---|---|---|---o Es=Esc
      |`Es| `1| `2| `3| `4| `5| `6|   `7| `8| `9| `0| `-| `=| `\|   | En=Enter
 -----|---o---|---|---|---|---|---|  ---|---|---|---|---|---|---o---| Gr=Grave
  `Tab|   | `Q| `W| `E| `R| `T| `Y|   `U| `I| `O| `P| `[| `]|   |`En| Bs=BkSp
 -----|---|---o---|---|---|---|---|  ---|---|---|---|---|---o---|---| Sp=Space
  `Ctl| `A|   | `S| `D| `F| `G| `H|   `J| `K| `L| `;| `'|   |`Bs|`Gr| Al=Alt
 -----|---|---|---o---|---|---|---|  ---|---|---|---|---o---|---|---| Shf=Shift
  `Shf| `Z| `X|   | `C| `V| `B| `N|   `M| `,| `.| `/|   |`Sp|`Fn|`Al|


  full wiring map:

       BUS0    BUS1    BUS2    BUS3    BUS4    BUS5    BUS6    BUS7
        |       |       |       |       |       |       |       |          0       1       2       3       4       5       6       7 
0-------o-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-  7-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-------o- 
        | `-ESC-+ `--1--+ `--2--+ `--3--+ `--4--+ `--5--+ `--6--+     `--7--+ `--8--+ `--9--+ `--0--+ `-{-}-+ `-{=}-+ `-{\}-+       | 
1-+-----|-------o-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-  6-+-----|-+-----|-+-----|-+-----|-+-----|-+-----|-------o-+-----|- 
  `-TAB-+       | `--Q--+ `--W--+ `--E--+ `--R--+ `--T--+ `--Y--+     `--U--+ `--I--+ `--O--+ `--P--+ `-{[}-+ `-{]}-+       | `-ENT-+ 
2-+-----|-+-----|-------o-+-----|-+-----|-+-----|-+-----|-+-----|-  5-+-----|-+-----|-+-----|-+-----|-+-----|-------o-+-----|-+-----|- 
  `-CTL-+ `--A--+       | `--S--+ `--D--+ `--F--+ `--G--+ `--H--+     `--J--+ `--K--+ `--L--+ `-{;}-+ `-{'}-+       | `-BSP-+ `-{`}-+ 
3-+-----|-+-----|-+-----|-------o-+-----|-+-----|-+-----|-+-----|-  4-+-----|-+-----|-+-----|-+-----|-------o-+-----|-+-----|-+-----|-
  `-SHFT+ `--Z--+ `--X--+       | `--C--+ `--V--+ `--B--+ `--N--+     `--M--+ `-{,}-+ `-{.}-+ `-{/}-+       | `-SPC-+ `-FN0-+ `-ALT-+ 
        |       |       |       |       |       |       |       |           |       |       |       |       |       |       |       | 
        |       |       |       |       |       |       |       `-----------|-------|-------|-------|-------|-------|-------|-------'
        |       |       |       |       |       |       `-------------------|-------|-------|-------|-------|-------|-------'
        |       |       |       |       |       `---------------------------|-------|-------|-------|-------|-------'
        |       |       |       |       `-----------------------------------|-------|-------|-------|-------'
        |       |       |       `-------------------------------------------|-------|-------|-------'
        |       |       `---------------------------------------------------|-------|-------'
        |       `-----------------------------------------------------------|-------'
        `-------------------------------------------------------------------'

