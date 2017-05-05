#ifndef SCREEN_H
#define SCREEN_H

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/GraphicsOutput.h>

typedef EFI_GRAPHICS_OUTPUT_BLT_PIXEL BltPixel;

EFI_STATUS InitScreen();

VOID DrawRect(IN UINT32 x, IN UINT32 y,
	      IN UINT32 width, IN UINT32 height,
	      IN BltPixel color);


VOID SetBackgroundColor(IN UINT8 r, IN UINT8 g, IN UINT8 b);

UINT32 ScreenGetWidth(VOID);

#endif
