    ORG    $10000            ; Program start address

START:
    ; Disable interrupts to take over the hardware
    MOVE.W #$7FFF, $DFF09A   ; Clear all bits in INTENA (disable interrupts)

    ; Disable all DMA channels initially
    MOVE.W #$7FFF, $DFF096   ; Clear all bits in DMACON (disable DMA)

    ; Set up the copper list pointer
    LEA    COPPERLIST, A0    ; Load address of copper list into A0
    MOVE.L A0, $DFF080       ; Set COP1LC to point to copper list

    ; Enable necessary DMA channels
    ; Bit 15=SET, 9=DMAEN, 8=BPLEN, 7=COPEN, 5=SPREN
    MOVE.W #$8380, $DFF096   ; Enable DMA for copper, bitplanes, and sprites

    ; Main loop to wait for 'Q' key press
MAINLOOP:
WAITKEY:
    BTST   #3, $BFED01       ; Test CIA-A ICR bit 3 (serial data ready)
    BEQ    WAITKEY           ; Loop until keyboard data is available
    MOVE.B $BFEC01, D0       ; Read scancode from CIA-A SP (clears interrupt)
    CMP.B  #$10, D0          ; Compare with 'Q' scancode ($10 for key down)
    BNE    MAINLOOP          ; If not 'Q', continue looping

    ; Quit (infinite loop for simplicity)
QUIT:
    BRA    QUIT              ; Loop forever (no system restore in this example)

; Copper list definition
COPPERLIST:
    DC.W   $0100, $9200      ; BPLCON0: Low-res, 1 bitplane, color on
    DC.W   $008E, $2C81      ; DIWSTRT: Display window start (PAL 320x256)
    DC.W   $0090, $2CC1      ; DIWSTOP: Display window stop
    DC.W   $0092, $0038      ; DDFSTRT: Data fetch start for low-res
    DC.W   $0094, $00D0      ; DDFSTOP: Data fetch stop
    DC.W   $0108, $0000      ; BPL1MOD: Bitplane modulo (0 for simple screen)
    DC.W   $010A, $0000      ; BPL2MOD: Not used here

    ; Bitplane pointer (points to blank memory at $20000)
    DC.W   $00E0, $0002      ; BPL1PTH: High word of $20000
    DC.W   $00E2, $0000      ; BPL1PTL: Low word of $20000

    ; Sprite pointers (point to sprite data blocks at $21000 + n*$48)
    DC.W   $0120, $0021      ; SPR0PTH: Sprite 0 high ($21000)
    DC.W   $0122, $0000      ; SPR0PTL: Sprite 0 low
    DC.W   $0124, $0021      ; SPR1PTH: Sprite 1 high ($21048)
    DC.W   $0126, $0048      ; SPR1PTL: Sprite 1 low
    DC.W   $0128, $0021      ; SPR2PTH: Sprite 2 high ($21090)
    DC.W   $012A, $0090      ; SPR2PTL: Sprite 2 low
    DC.W   $012C, $0021      ; SPR3PTH: Sprite 3 high ($210D8)
    DC.W   $012E, $00D8      ; SPR3PTL: Sprite 3 low
    DC.W   $0130, $0021      ; SPR4PTH: Sprite 4 high ($21120)
    DC.W   $0132, $0120      ; SPR4PTL: Sprite 4 low
    DC.W   $0134, $0021      ; SPR5PTH: Sprite 5 high ($21168)
    DC.W   $0136, $0168      ; SPR5PTL: Sprite 5 low
    DC.W   $0138, $0021      ; SPR6PTH: Sprite 6 high ($211B0)
    DC.W   $013A, $01B0      ; SPR6PTL: Sprite 6 low

    ; Color palette
    DC.W   $0180, $0000      ; COLOR00: Black background
    DC.W   $01A2, $0FFF      ; COLOR17: White for sprite 0 color 1
    DC.W   $01AA, $0FFF      ; COLOR21: White for sprite 1 color 1
    DC.W   $01B2, $0FFF      ; COLOR25: White for sprite 2 color 1
    DC.W   $01BA, $0FFF      ; COLOR29: White for sprite 3 color 1
    DC.W   $01C2, $0FFF      ; COLOR33: White for sprite 4 color 1
    DC.W   $01CA, $0FFF      ; COLOR37: White for sprite 5 color 1
    DC.W   $01D2, $0FFF      ; COLOR41: White for sprite 6 color 1

    ; End of copper list
    DC.W   $FFFF, $FFFE      ; Copper waits forever (static display)

; Sprite data definitions
; Each sprite is 16 pixels wide, 16 lines high, positioned at Y=100, X=20+40*n
; Sprite data format: [VSTART,HSTART high], [VSTOP,HSTART low], [16 pairs of pixel data], [0,0]
; VSTART=100 ($64), VSTOP=116 ($74), X varies, pixel data = solid white (color 1)

SPRITE0:
    DC.W   $640A, $7400      ; X=20: ($64<<8)|($14>>1)=25610=$640A, ($74<<8)|0=$7400
    DC.W   $FFFF, $0000      ; Line 0: Bitplane 0 all 1s, Bitplane 1 all 0s
    DC.W   $FFFF, $0000      ; Line 1
    DC.W   $FFFF, $0000      ; Line 2
    DC.W   $FFFF, $0000      ; Line 3
    DC.W   $FFFF, $0000      ; Line 4
    DC.W   $FFFF, $0000      ; Line 5
    DC.W   $FFFF, $0000      ; Line 6
    DC.W   $FFFF, $0000      ; Line 7
    DC.W   $FFFF, $0000      ; Line 8
    DC.W   $FFFF, $0000      ; Line 9
    DC.W   $FFFF, $0000      ; Line 10
    DC.W   $FFFF, $0000      ; Line 11
    DC.W   $FFFF, $0000      ; Line 12
    DC.W   $FFFF, $0000      ; Line 13
    DC.W   $FFFF, $0000      ; Line 14
    DC.W   $FFFF, $0000      ; Line 15
    DC.W   $0000, $0000      ; End of sprite

SPRITE1:
    DC.W   $641E, $7400      ; X=60: ($64<<8)|($3C>>1)=25630=$641E
    DC.W   $FFFF, $0000      ; 16 lines of pixel data (same as above)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

SPRITE2:
    DC.W   $6432, $7400      ; X=100: ($64<<8)|($64>>1)=25650=$6432
    DC.W   $FFFF, $0000      ; 16 lines (repeated)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

SPRITE3:
    DC.W   $6446, $7400      ; X=140: ($64<<8)|($8C>>1)=25670=$6446
    DC.W   $FFFF, $0000      ; 16 lines (repeated)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

SPRITE4:
    DC.W   $645A, $7400      ; X=180: ($64<<8)|($B4>>1)=25690=$645A
    DC.W   $FFFF, $0000      ; 16 lines (repeated)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

SPRITE5:
    DC.W   $646E, $7400      ; X=220: ($64<<8)|($DC>>1)=25710=$646E
    DC.W   $FFFF, $0000      ; 16 lines (repeated)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

SPRITE6:
    DC.W   $6482, $7400      ; X=260: ($64<<8)|($104>>1)=25730=$6482
    DC.W   $FFFF, $0000      ; 16 lines (repeated)
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $FFFF, $0000
    DC.W   $0000, $0000

    ; End of program
