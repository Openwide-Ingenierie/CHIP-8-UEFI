#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/Rng.h>

#include "Screen.h"
#include "Chip8.h"
#include "Roms.h"

static EFI_RNG_PROTOCOL* pRng = NULL;
static EFI_GUID gRngGuid = EFI_RNG_PROTOCOL_GUID;

EFI_STATUS EFIAPI
UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  Initialization();
  // Print Games
  Print(L"1 - PONG\n2 - PONG2\n3 - TETRIS\n4 - BRIX\n5 - TANK");
  EFI_INPUT_KEY Key;
  UINTN EventIndex;
  BOOLEAN KeyOk = FALSE;
  while(!KeyOk){
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    KeyOk = TRUE;
    switch(Key.UnicodeChar){
    case '1':
      LoadRom(PONG, PONG_len);
      break;
    case '2':
      LoadRom(PONG2, PONG2_len);
      break;
    case '3':
      LoadRom(TETRIS, TETRIS_len);
      break;
    case '4':
      LoadRom(BRIX, BRIX_len);
      break;
    case '5':
      LoadRom(TANK, TANK_len);
      break;
    default:
      KeyOk = FALSE;
      break;
    }
  }
  Emulate();
  return EFI_SUCCESS;
}

VOID
Emulate(VOID)
{
  UINT32 pc = 0x200;
  UINT8 sp = 0xE;
  UINT16 I = 0;
  UINT16 DT = 0;
  UINT16 ST = 0;
  SetBackgroundColor(0, 0, 0);
  
  while(TRUE){
    if(pc == RAMSIZE - 1) break;

    UINT16 instr = Ram[pc] << 8 | Ram[pc+1];
    // 00EE RET
    if(instr == 0x00EE){
      if(sp == 0xE){
        Print(L"Stack pointer already on top!\n");
	break;
      }
      pc = Stack[++sp];
    }
    // 00E0 CLS
    else if(instr == 0x00E0){
      gBS->SetMem(Screen, WIDTH*HEIGHT, 0);
    }
    // 0nnn - SYS addr
    else if(instr >> 12 == 0){
      //Nothing
    }
    // 1nnn - JP addr
    else if(instr >> 12 == 1){
      pc = (instr & 0x0FFF);
      pc -= 2;
    }
    // 2nnn - CALL addr
    else if(instr >> 12 == 2){
      UINT16 addr = instr & 0x0FFF;
      Stack[sp--] = pc;
      pc = addr - 2;
    }
    // 3xkk - SE Vx, byte
    else if(instr >> 12 == 3){
      UINT8 x = instr >> 8 & 0xF;
      UINT8 value = instr & 0xFF;
      if(Reg[x] == value) pc += 2;
    }
    // 4xkk - SNE
    else if(instr >> 12 == 4){
      UINT8 x = instr >> 8 & 0xF;
      UINT8 value = instr & 0xFF;
      if(Reg[x] != value) pc += 2;
    }
    // 5xy0 - SE Vx, Vy
    else if(instr >> 12 == 5 && (instr & 0xF) == 0){
      UINT32 x = (instr >> 8) & 0xF;
      UINT32 y = (instr >> 4) & 0xF;
      if(Reg[x] == Reg[y]) pc+=2;
    }
    // 6xkk - LD Vx, byte
    else if(instr >> 12 == 6){
      UINT32 x = (instr >> 8) & 0xF;
      UINT8 value = instr & 0xFF;
      Reg[x] = value;
    }
    // 7xkk - ADD Vx, byte
    else if(instr >> 12 == 7){
      UINT32 x = (instr >> 8) & 0xF;
      UINT8 value = instr & 0xFF;
      Reg[x] += value;
    }
    // 8xy0 - LD Vx, Vy
    else if(instr >> 12 == 8)
      instruction8(instr);
    // 9xy0 - SNE Vx, Vy
    else if(instr >> 12 == 9 && (instr & 0xF) == 0){
      UINT32 x = (instr >> 8) & 0xF;
      UINT32 y = (instr >> 4) & 0xF;
      if(Reg[x] != Reg[y]) pc += 2;
    }
    // Annn - LD I, addr
    else if(instr >> 12 == 0xA){
      I = instr & 0x0FFF;
    }
    // Bnnn - JP V0, addr
    else if(instr >> 12 == 0xB){
      UINT32 addr = instr & 0xFFF;
      pc = addr + Reg[0] - 2;
    }
    // Cxkk - RND Vx, byte
    else if(instr >> 12 == 0xC){
      UINT32 x = (instr >> 8) & 0xF;
      UINT32 value = instr & 0xFF;
      UINT8 rand = 0;
      if(pRng != NULL){
	pRng->GetRNG(pRng, NULL, 1, &rand);
      } else {
	UINT64 Count;
	gBS->GetNextMonotonicCount(&Count);
	rand = Count & 0xFF;
      }
      Reg[x] = (rand++) & value;
    }
    // Dxyn - DRW Vx, Vy, nibble
    else if(instr >> 12 == 0xD){
      Reg[0xF] = 0;

      UINT32 vx = (instr >> 8) & 0xF;
      UINT32 vy = (instr >> 4) & 0xF;
      UINT32 x = Reg[vx];
      UINT32 y = Reg[vy];
      UINT32 n = instr & 0xF;
      for(UINT32 i = 0; i < n; i++){
        UINT8 b = Ram[I+i];
        for(UINT32 j = 0 ; j < 8; j++){
          UINT32 bit = (b >> (7 - j)) & 0x1;
          if(bit && Screen[y+i][x+j])
	    Reg[0xF] = 1;
          Screen[y+i][x+j] ^= bit;
        }
      }
    }
    else if(instr >> 12 == 0xE)
      instructionE(instr, &pc);
    else if(instr >> 12 == 0xF)
      instructionF(instr, &DT, &ST, &I);
    else
      Print(L"Unknown opcode %x\n", instr);
    
    // Draw the screen
    drawScreen();
    if(DT != 0) DT--;
    if(ST != 0) ST--;

    // Slow down the game
    gBS->Stall(2500);
    
    pc += 2;
  }
}

VOID
drawScreen(VOID)
{
  UINT32 i, j;
  for(i = 0; i < HEIGHT; i++){
    for(j = 0; j < WIDTH; j++){
      if(Screen[i][j] == ScreenBefore[i][j]) continue;
      if(Screen[i][j])
	DrawRect(j*PIXELSIZE, i*PIXELSIZE, PIXELSIZE, PIXELSIZE, white);
      else
	DrawRect(j*PIXELSIZE, i*PIXELSIZE, PIXELSIZE, PIXELSIZE, black);
    }
  }
  gBS->CopyMem(ScreenBefore, Screen, HEIGHT*WIDTH);
}

VOID
instruction8(UINT32 instr)
{
  UINT32 x = (instr >> 8) & 0xF;
  UINT32 y = (instr >> 4) & 0xF;
  UINT32 code = (instr & 0xF);
  switch(code){
  case 0:
    Reg[x] = Reg[y];
    break;
  case 1:
    Reg[x] |= Reg[y];
    break;
  case 2:
    Reg[x] &= Reg[y];
    break;
  case 3:
    Reg[x] ^= Reg[y];
    break;
  case 4:
    Reg[0xF] = (((UINT32) Reg[x]) + ((UINT32) Reg[y]) > 255);
    Reg[x] += Reg[y];
    break;
  case 5:
    Reg[0xF] = (Reg[x] > Reg[y]);
    Reg[x] -= Reg[y];
    break;
  case 6:
    Reg[0xF] = Reg[x] & 0x1;
    Reg[x] >>= 1;
    break;
  case 7:
    Reg[0xF] = (Reg[y] > Reg[x]);
    Reg[x] = Reg[y] - Reg[x];
    break;
  case 0xE:
    Reg[0xF] = (Reg[x] & 0x80) != 0;
    Reg[x] <<= 1;
    break;
  }
}

EFI_INPUT_KEY
keyToValue(UINT8 value)
{
  EFI_INPUT_KEY ret = {0, 0};
  if(value >= 0 && value <= 9)
    ret.UnicodeChar = value + '0';
  else if(value >= 0xA && value <= 0xF)
    ret.UnicodeChar = value - 0xA + 'a';
  return ret;
}

UINT8
valueToKey(EFI_INPUT_KEY key)
{
  CHAR16 code = key.UnicodeChar;
  if(code >= '0' && code <= '9')
    return code - '0';
  else if(code >= 'a' && code <= 'f')
    return code - 'a' + 10;
  return 0xFF;
}

VOID
instructionE(UINT32 instr, UINT32* pc){
  UINT32 x = instr >> 8 & 0xF;
  UINT32 code = instr & 0xFF;
  EFI_STATUS stat;
  if(code == 0x9E){
    EFI_INPUT_KEY Key, KeyD = keyToValue(Reg[x]);
    stat = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    if(stat == EFI_SUCCESS && Key.UnicodeChar == KeyD.UnicodeChar)
      *pc += 2;
  }
  else if(code == 0xA1){
    EFI_INPUT_KEY Key, KeyD = keyToValue(Reg[x]);
    stat = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    if(stat != EFI_SUCCESS || Key.UnicodeChar != KeyD.UnicodeChar)
      *pc += 2;
  }
  else{
    Print(L"Unknwon opcode %x\n", instr);
  }
}

VOID
instructionF(UINT32 instr, UINT16* DT, UINT16* ST, UINT16* I)
{
  UINT32 x = (instr >> 8) & 0xF;
  UINT32 code = instr & 0xFF;
  UINT32 i = 0, ok = 0;
  EFI_INPUT_KEY Key;
  UINTN EventIndex;
  switch(code){
  case 0x07:
    Reg[x] = *DT;
    break;
  case 0x0A:
    while(!ok){
      gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
      gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
      i = valueToKey(Key);
      if(0 <= i && i <= 0xF){
	Reg[x] = i;
	ok = TRUE;
      }
    }
    break;
  case 0x15:
    *DT = Reg[x];
    break;
  case 0x18:
    *ST = Reg[x];
    break;
  case 0x1E:
    *I += Reg[x];
    break;
  case 0x29:
    *I = Reg[x] * 5;
    break;
  case 0x33:
    Ram[*I]   = Reg[x]/100;
    Ram[*I+1] = (Reg[x]/10) % 10;
    Ram[*I+2] = Reg[x] % 10;
    break;
  case 0x55:
    for(i = 0; i <= x; i++)
      Ram[*I+i] = Reg[i];
    break;
  case 0x65:
    for(i = 0; i <= x; i++)
      Reg[i] = Ram[*I+i];
    break;
  }
}


VOID
Initialization(VOID)
{
  EFI_STATUS s = InitScreen();
  if(EFI_ERROR(s)){
    Print(L"Error initializing screen\nPress any key to exit...\n");
    PressKey(FALSE);
    Exit(s);
  }
  PIXELSIZE = ScreenGetWidth()/WIDTH;
  
  gBS->SetMem(Ram, RAMSIZE, 0);
  gBS->SetMem(Screen, HEIGHT*WIDTH, 0);
  gBS->SetMem(ScreenBefore, HEIGHT*WIDTH, 0);
  //Add chars to RAM
  UINT8 Chars[] = {0xF0,0x90,0x90,0x90,0xF0,
		   0x20,0x60,0x20,0x20,0x70,
		   0xF0,0x10,0xF0,0x80,0xF0,
		   0xF0,0x10,0xF0,0x10,0xF0,
		   0x90,0x90,0xF0,0x10,0x10,
		   0xF0,0x80,0xF0,0x10,0xF0,
		   0xF0,0x80,0xF0,0x90,0xF0,
		   0xF0,0x10,0x20,0x40,0x40,
		   0xF0,0x90,0xF0,0x90,0xF0,
		   0xF0,0x90,0xF0,0x10,0xF0,
		   0xF0,0x90,0xF0,0x90,0x90,
		   0xE0,0x90,0xE0,0x90,0xE0,
		   0xF0,0x80,0x80,0x80,0xF0,
		   0xE0,0x90,0x90,0x90,0xE0,
		   0xF0,0x80,0xF0,0x80,0xF0,
		   0xF0,0x80,0xF0,0x80,0x80};
  gBS->CopyMem(Ram, Chars, sizeof(Chars));

  // Init random protocol
  s = gBS->LocateProtocol(&gRngGuid, NULL, (VOID**) &pRng);
  if( EFI_ERROR(s) || pRng == NULL ){
    Print(L"Error initializing random generator protocol (error %d)\n\
Numbers generated won't be random\n\n", s);
  }
}

VOID
LoadRom(UINT8* Data, UINT32 Data_len)
{
  gBS->CopyMem(Ram+0x200, Data, Data_len);
}

VOID
PressKey(BOOLEAN DisplayText)
{
  EFI_INPUT_KEY Key;
  //EFI_STATUS    Status;
  UINTN         EventIndex;
  if (DisplayText) {
    Print(L"\nPress any key to continue ....\n\n");
  }
  gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
}
