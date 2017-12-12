
Keyboard 
============
PS/2 keyboard with own firmware mostly based on ps2avr.
Diode per key charliplexed keyboard matrix have no ghosting and support NKRO.

Microcontroller
=================
Used ATmega328p from Arduino, but TQFP version, with ability
to connect A6 and A7. Antighosting reference diode with level shift
resisor(2.7k) connected to AIN0(+)=D6:
```
(D6)---\/\/\--->|--GND
```
Bus pinout (Arduino):

```
  BUS0 PC0 (A0)
  BUS1 PC1 (A1)
  BUS2 PC2 (A2)
  BUS3 PC3 (A3)
  BUS4 PC4 (A4)
  BUS5 PC5 (A5)
  BUS6 PD3+A6 (D3)
  BUS7 PD2+A7 (D2)
```

optional for 9x8 matrix:
```
  BUS8 PD7 (D7)
```
BUS8 does not implemented in firmware, but it is easy to do.

PS/2 interface connection:
 DATA  PB2 (D10)
 CLOCK PB0 (D8)


Wiring
============

Matrix of 7x8 has 56 commutation points was connected with 58 key switches.
Control and Shift key switches from left and right are connected together.

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
```
                    |
   -----o---------------
        |           |          --+-----|-      ------|
         `-|<---/ --o       =    `-KEY-+    =    `KEY|
                    |                  |                 
```
  Buses with same number are connected:
```
      0   1   2   3   4   5   6   7      0   1   2   3   4   5   6   7
0-----o---|---|---|---|---|---|---|  7---|---|---|---|---|---|---|---o  Es=Esc
      |`Es| `1| `2| `3| `4| `5| `6|    `7| `8| `9| `0| `-| `=| `\|   |  En=Enter
1-----|---o---|---|---|---|---|---|  6---|---|---|---|---|---|---o---|  Gr=Grave
  `Tab|   | `Q| `W| `E| `R| `T| `Y|    `U| `I| `O| `P| `[| `]|   |`En|  Bs=BkSp
2-----|---|---o---|---|---|---|---|  5---|---|---|---|---|---o---|---|  Sp=Space
  `Ctl| `A|   | `S| `D| `F| `G| `H|    `J| `K| `L| `;| `'|   |`Bs|`Gr|  Al=Alt
3-----|---|---|---o---|---|---|---|  4---|---|---|---|---o---|---|---| Shf=Shift
  `Shf| `Z| `X|   | `C| `V| `B| `N|    `M| `,| `.| `/|   |`Sp|`Fn|`Al|
```

  full wiring map:
```
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

```

Arduino pro mini 3.3v 8MHz:
* pro.bootloader.low_fuses=0xc6      # ckdiv=1(no) ckout=1(no) sut=00 (ceramic,fastRizing) cksel=0110(fullswing crystal)  (def=62)
* pro.bootloader.high_fuses=0xdd     # rstdis=1(no) dwen=1(no) spien=0(en) wdon=1(off) eesav=1(no) bodlvl=101(2.7v) (def=DF)
* pro.bootloader.extended_fuses=0x00 # bootsz=00(1k) bootres=0(bootloader)
* pro.bootloader.file=ATmegaBOOT_168_pro_8MHz.hex
* pro.build.mcu=atmega168
* pro.build.f_cpu=8000000L

Internal OSC 8Mhz mega168:
* pro.bootloader.low_fuses=0xe2      # ckdiv=1(no) ckout=1(no) sut=10 (slow rising) cksel=0010(internal)
* pro.bootloader.high_fuses=0xdf     # default
* pro.bootloader.extended_fuses=0x00 # bootsz=00(1k) bootres=0(bootloader)
* pro.bootloader.file=ATmegaBOOT_168_pro_8MHz.hex
* pro.build.mcu=atmega168
* pro.build.f_cpu=8000000L

Arduino Pro or Pro Mini (3.3V, 8 MHz) w/ ATmega328:
pro328.bootloader.low_fuses=0xFF      # ckdiv8=1(no) ckout=1(no) sut=11(xtal,slowRising) cksel=1111(lopwr,8-16mhz) (def=62)
pro328.bootloader.high_fuses=0xDA     # rstdis=1(no) dwen=1(no) spien=0(en) wdon=1(no) eesav=1(no) bootsz=01(512b) bootrs=0(boot) (def=D9) 
pro328.bootloader.extended_fuses=0x05 # bodlvl=101 (2.7v)
pro328.bootloader.file=ATmegaBOOT_168_atmega328_pro_8MHz.hex
pro328.build.mcu=atmega328p
pro328.build.f_cpu=8000000L

Internal OSC 8Mhz mega328p:
pro328.bootloader.low_fuses=0xE2      # ckdiv8=1(no) ckout=1(no) sut=10(slowRising) cksel=0010(internal)
pro328.bootloader.high_fuses=0xDA     # rstdis=1(no) dwen=1(no) spien=0(en) wdon=1(no) eesav=1(no) bootsz=01(512b) bootrs=0(boot)
pro328.bootloader.extended_fuses=0x07 # nobodlvl
pro328.bootloader.file=ATmegaBOOT_168_atmega328_pro_8MHz.hex
pro328.build.mcu=atmega328p
pro328.build.f_cpu=8000000L
