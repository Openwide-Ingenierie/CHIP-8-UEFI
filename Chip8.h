#ifndef CHIP_H
#define CHIP_H

#define RAMSIZE 0xFFF
#define WIDTH 64
#define HEIGHT 32

UINT8 Ram[RAMSIZE];
UINT8 Reg[0xF];
UINT16 Stack[0xF];
BOOLEAN ScreenBefore[HEIGHT][WIDTH];
BOOLEAN Screen[HEIGHT][WIDTH];
BltPixel white = {0xFF, 0xFF, 0xFF, 0};
BltPixel black = {0, 0, 0, 0};
UINT32 PIXELSIZE;

VOID Initialization(VOID);
VOID PressKey(BOOLEAN DisplayText);
VOID LoadRom(UINT8* Data, UINT32 Data_len);
VOID Emulate(VOID);
VOID drawScreen(VOID);
VOID instruction8(UINT32 instr);
EFI_INPUT_KEY keyToValue(UINT8 value);
UINT8 valueToKey(EFI_INPUT_KEY key);
VOID instructionE(UINT32 instr, UINT32* pc);
VOID instructionF(UINT32 instr, UINT16* DT, UINT16* ST, UINT16* I);

#endif
