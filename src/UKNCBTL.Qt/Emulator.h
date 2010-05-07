// Emulator.h

#pragma once

#include "emubase/Board.h"

//////////////////////////////////////////////////////////////////////


extern CMotherboard* g_pBoard;

extern BOOL g_okEmulatorRunning;

extern BYTE* g_pEmulatorRam[3];  // RAM values - for change tracking
extern BYTE* g_pEmulatorChangedRam[3];  // RAM change flags
extern WORD g_wEmulatorCpuPC;      // Current PC value
extern WORD g_wEmulatorPrevCpuPC;  // Previous PC value
extern WORD g_wEmulatorPpuPC;      // Current PC value
extern WORD g_wEmulatorPrevPpuPC;  // Previous PC value


//////////////////////////////////////////////////////////////////////


BOOL InitEmulator();
void DoneEmulator();
//void Emulator_SetCPUBreakpoint(WORD address);
//void Emulator_SetPPUBreakpoint(WORD address);
//BOOL Emulator_IsBreakpoint();
//void Emulator_SetSound(BOOL soundOnOff);
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();
int Emulator_SystemFrame();

void Emulator_PrepareScreenRGB32(void* pBits);

void Emulator_KeyEvent(BYTE keyPressed, BOOL pressed);
WORD Emulator_GetKeyEventFromQueue();
void Emulator_ProcessKeyEvent();

void Emulator_LoadROMCartridge(int slot, LPCTSTR sFilePath);

// Update cached values after Run or Step
void Emulator_OnUpdate();
WORD Emulator_GetChangeRamStatus(int addrtype, WORD address);

//void Emulator_SaveImage(LPCTSTR sFilePath);
//void Emulator_LoadImage(LPCTSTR sFilePath);


//////////////////////////////////////////////////////////////////////
