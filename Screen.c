#include "Screen.h"

static EFI_GUID graphicProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GRAPHICS_OUTPUT_PROTOCOL* protocol = NULL;
static UINT32 pplin;
static UINT32 width;
static UINT32 height;

EFI_STATUS
InitScreen(){
    
  EFI_STATUS s = gBS->LocateProtocol(&graphicProtocolGuid,
				     NULL, (VOID**) &protocol);
  
  if( EFI_ERROR(s) || protocol == NULL ){
    return s;
  }

  width  = protocol->Mode->Info->HorizontalResolution;
  height = protocol->Mode->Info->VerticalResolution;
  pplin = protocol->Mode->Info->PixelsPerScanLine;

  return s;
}

VOID
DrawRect(IN UINT32 x, IN UINT32 y,
	 IN UINT32 width, IN UINT32 height,
	 IN BltPixel color)
{
  BltPixel buf = color;
  EFI_STATUS st = protocol->Blt(protocol, &buf, EfiBltVideoFill,
				0, 0,
				x, y,
				width, height, 0);
  if(EFI_ERROR(st)){
    Print(L"Error %d drawing rectangle at (%u,%u,%u,%u)\n", st, x, y, width, height);
  }
}

VOID
SetBackgroundColor(IN UINT8 r, IN UINT8 g, IN UINT8 b)
{
  BltPixel background = {b, g, r, 0};
  DrawRect(0, 0, width, height, background);
}

UINT32
ScreenGetWidth(VOID)
{
  return width;
}

