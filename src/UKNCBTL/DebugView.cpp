/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

// DebugView.cpp

#include "stdafx.h"
#include <commctrl.h>
#include "UKNCBTL.h"
#include "Views.h"
#include "ToolWindow.h"
#include "Emulator.h"
#include "emubase\Emubase.h"


//////////////////////////////////////////////////////////////////////

// Colors
#define COLOR_RED   RGB(255,0,0)
#define COLOR_BLUE  RGB(0,0,255)


HWND g_hwndDebug = (HWND) INVALID_HANDLE_VALUE;  // Debug View window handle
WNDPROC m_wndprocDebugToolWindow = NULL;  // Old window proc address of the ToolWindow

HWND m_hwndDebugViewer = (HWND) INVALID_HANDLE_VALUE;
HWND m_hwndDebugToolbar = (HWND) INVALID_HANDLE_VALUE;

BOOL m_okDebugProcessor = FALSE;  // TRUE - CPU, FALSE - PPU
WORD m_wDebugCpuR[11];  // Old register values - R0..R7, PSW, CPC, CPSW
WORD m_wDebugPpuR[11];  // Old register values - R0..R7, PSW, CPC, CPSW
BOOL m_okDebugCpuRChanged[11];  // Register change flags
BOOL m_okDebugPpuRChanged[11];  // Register change flags

void DebugView_DoDraw(HDC hdc);
BOOL DebugView_OnKeyDown(WPARAM vkey, LPARAM lParam);
void DebugView_DrawProcessor(HDC hdc, const CProcessor* pProc, int x, int y, WORD* arrR, BOOL* arrRChanged);
void DebugView_DrawMemoryForRegister(HDC hdc, int reg, CProcessor* pProc, int x, int y);
void DebugView_DrawPorts(HDC hdc, BOOL okProcessor, const CMemoryController* pMemCtl, CMotherboard* pBoard, int x, int y);
void DebugView_DrawChannels(HDC hdc, int x, int y);
void DebugView_UpdateWindowText();


//////////////////////////////////////////////////////////////////////


void DebugView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= DebugViewViewerWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_DEBUGVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void DebugView_Init()
{
    memset(m_wDebugCpuR, 255, sizeof(m_wDebugCpuR));
    memset(m_okDebugCpuRChanged, 1, sizeof(m_okDebugCpuRChanged));
    memset(m_wDebugPpuR, 255, sizeof(m_wDebugPpuR));
    memset(m_okDebugPpuRChanged, 1, sizeof(m_okDebugPpuRChanged));
}

void CreateDebugView(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndDebug = CreateWindow(
            CLASSNAME_TOOLWINDOW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
    DebugView_UpdateWindowText();

    // ToolWindow subclassing
    m_wndprocDebugToolWindow = (WNDPROC) LongToPtr( SetWindowLongPtr(
            g_hwndDebug, GWLP_WNDPROC, PtrToLong(DebugViewWndProc)) );

    RECT rcClient;  GetClientRect(g_hwndDebug, &rcClient);

    m_hwndDebugViewer = CreateWindowEx(
            WS_EX_STATICEDGE,
            CLASSNAME_DEBUGVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            0, 0, rcClient.right, rcClient.bottom,
            g_hwndDebug, NULL, g_hInst, NULL);

    m_hwndDebugToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_VERT,
            4, 4, 32, rcClient.bottom, m_hwndDebugViewer,
            (HMENU) 102,
            g_hInst, NULL);

    TBADDBITMAP addbitmap;
    addbitmap.hInst = g_hInst;
    addbitmap.nID = IDB_TOOLBAR;
    SendMessage(m_hwndDebugToolbar, TB_ADDBITMAP, 2, (LPARAM) &addbitmap);

    SendMessage(m_hwndDebugToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
    SendMessage(m_hwndDebugToolbar, TB_SETBUTTONSIZE, 0, (LPARAM) MAKELONG (26, 26));

    TBBUTTON buttons[3];
    ZeroMemory(buttons, sizeof(buttons));
    for (int i = 0; i < sizeof(buttons) / sizeof(TBBUTTON); i++)
    {
        buttons[i].fsState = TBSTATE_ENABLED | TBSTATE_WRAP;
        buttons[i].fsStyle = BTNS_BUTTON;
        buttons[i].iString = -1;
    }
    buttons[0].idCommand = ID_DEBUG_CPUPPU;
    buttons[0].iBitmap = 17;
    buttons[1].idCommand = ID_DEBUG_STEPINTO;
    buttons[1].iBitmap = 15;
    buttons[2].idCommand = ID_DEBUG_STEPOVER;
    buttons[2].iBitmap = 16;

    SendMessage(m_hwndDebugToolbar, TB_ADDBUTTONS, (WPARAM) sizeof(buttons) / sizeof(TBBUTTON), (LPARAM) &buttons);
}

// Adjust position of client windows
void DebugView_AdjustWindowLayout()
{
    RECT rc;  GetClientRect(g_hwndDebug, &rc);

    if (m_hwndDebugViewer != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndDebugViewer, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

LRESULT CALLBACK DebugViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LRESULT lResult;
    switch (message)
    {
    case WM_DESTROY:
        g_hwndDebug = (HWND) INVALID_HANDLE_VALUE;  // We are closed! Bye-bye!..
        return CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
    case WM_SIZE:
        lResult = CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
        DebugView_AdjustWindowLayout();
        return lResult;
    default:
        return CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
    }
    //return (LRESULT)FALSE;
}

LRESULT CALLBACK DebugViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_COMMAND:
        ::PostMessage(g_hwnd, WM_COMMAND, wParam, lParam);
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            DebugView_DoDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
    case WM_KEYDOWN:
        return (LRESULT) DebugView_OnKeyDown(wParam, lParam);
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        ::InvalidateRect(hWnd, NULL, TRUE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

BOOL DebugView_OnKeyDown(WPARAM vkey, LPARAM /*lParam*/)
{
    switch (vkey)
    {
    case VK_SPACE:
        DebugView_SwitchCpuPpu();
        break;
    case VK_ESCAPE:
        ConsoleView_Activate();
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

void DebugView_UpdateWindowText()
{
    CProcessor* pDebugPU = (m_okDebugProcessor) ? g_pBoard->GetCPU() : g_pBoard->GetPPU();
    ASSERT(pDebugPU != NULL);
    LPCTSTR sProcName = pDebugPU->GetName();

    TCHAR buffer[64];
    _stprintf_s(buffer, 64, _T("Debug - %s"), sProcName);
    ::SetWindowText(g_hwndDebug, buffer);
}

void DebugView_SwitchCpuPpu()
{
    m_okDebugProcessor = ! m_okDebugProcessor;
    InvalidateRect(m_hwndDebugViewer, NULL, TRUE);
    DebugView_UpdateWindowText();

    DisasmView_SetCurrentProc(m_okDebugProcessor);
    ConsoleView_SetCurrentProc(m_okDebugProcessor);
}


//////////////////////////////////////////////////////////////////////

// Update after Run or Step
void DebugView_OnUpdate()
{
    CProcessor* pCPU = g_pBoard->GetCPU();
    ASSERT(pCPU != NULL);

    // Get new register values and set change flags
    for (int r = 0; r < 8; r++)
    {
        WORD value = pCPU->GetReg(r);
        m_okDebugCpuRChanged[r] = (m_wDebugCpuR[r] != value);
        m_wDebugCpuR[r] = value;
    }
    WORD pswCPU = pCPU->GetPSW();
    m_okDebugCpuRChanged[8] = (m_wDebugCpuR[8] != pswCPU);
    m_wDebugCpuR[8] = pswCPU;
    WORD cpcCPU = pCPU->GetCPC();
    m_okDebugCpuRChanged[9] = (m_wDebugCpuR[9] != cpcCPU);
    m_wDebugCpuR[9] = cpcCPU;
    WORD cpswCPU = pCPU->GetCPSW();
    m_okDebugCpuRChanged[10] = (m_wDebugCpuR[10] != cpswCPU);
    m_wDebugCpuR[10] = cpswCPU;

    CProcessor* pPPU = g_pBoard->GetPPU();
    ASSERT(pPPU != NULL);

    // Get new register values and set change flags
    for (int r = 0; r < 8; r++)
    {
        WORD value = pPPU->GetReg(r);
        m_okDebugPpuRChanged[r] = (m_wDebugPpuR[r] != value);
        m_wDebugPpuR[r] = value;
    }
    WORD pswPPU = pPPU->GetPSW();
    m_okDebugPpuRChanged[8] = (m_wDebugPpuR[8] != pswPPU);
    m_wDebugPpuR[8] = pswPPU;
    WORD cpcPPU = pPPU->GetCPC();
    m_okDebugPpuRChanged[9] = (m_wDebugPpuR[9] != cpcPPU);
    m_wDebugPpuR[9] = cpcPPU;
    WORD cpswPPU = pPPU->GetCPSW();
    m_okDebugPpuRChanged[10] = (m_wDebugPpuR[10] != cpswPPU);
    m_wDebugPpuR[10] = cpswPPU;
}

void DebugView_SetCurrentProc(BOOL okCPU)
{
    m_okDebugProcessor = okCPU;
    InvalidateRect(m_hwndDebugViewer, NULL, TRUE);
    DebugView_UpdateWindowText();
}


//////////////////////////////////////////////////////////////////////
// Draw functions

void DebugView_DoDraw(HDC hdc)
{
    ASSERT(g_pBoard != NULL);

    // Create and select font
    HFONT hFont = CreateMonospacedFont();
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorOld = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    COLORREF colorBkOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    CProcessor* pDebugPU = (m_okDebugProcessor) ? g_pBoard->GetCPU() : g_pBoard->GetPPU();
    ASSERT(pDebugPU != NULL);
    WORD* arrR = (m_okDebugProcessor) ? m_wDebugCpuR : m_wDebugPpuR;
    BOOL* arrRChanged = (m_okDebugProcessor) ? m_okDebugCpuRChanged : m_okDebugPpuRChanged;

    //LPCTSTR sProcName = pDebugPU->GetName();
    //TextOut(hdc, cxChar * 1, 2 + 1 * cyLine, sProcName, 3);

    DebugView_DrawProcessor(hdc, pDebugPU, 30 + cxChar * 2, 2 + 1 * cyLine, arrR, arrRChanged);

    // Draw stack for the current processor
    DebugView_DrawMemoryForRegister(hdc, 6, pDebugPU, 30 + 35 * cxChar, 2 + 0 * cyLine);

    CMemoryController* pDebugMemCtl = pDebugPU->GetMemoryController();
    DebugView_DrawPorts(hdc, m_okDebugProcessor, pDebugMemCtl, g_pBoard, 30 + 57 * cxChar, 2 + 0 * cyLine);

    //DebugView_DrawChannels(hdc, 75 * cxChar, 2 + 0 * cyLine);

    SetTextColor(hdc, colorOld);
    SetBkColor(hdc, colorBkOld);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    if (::GetFocus() == m_hwndDebugViewer)
    {
        RECT rcClient;
        GetClientRect(m_hwndDebugViewer, &rcClient);
        DrawFocusRect(hdc, &rcClient);
    }
}

void DrawRectangle(HDC hdc, int x1, int y1, int x2, int y2)
{
    HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetSysColorBrush(COLOR_BTNSHADOW));
    PatBlt(hdc, x1, y1, x2 - x1, 1, PATCOPY);
    PatBlt(hdc, x1, y1, 1, y2 - y1, PATCOPY);
    PatBlt(hdc, x1, y2, x2 - x1, 1, PATCOPY);
    PatBlt(hdc, x2, y1, 1, y2 - y1 + 1, PATCOPY);
    ::SelectObject(hdc, hOldBrush);
}

void DebugView_DrawProcessor(HDC hdc, const CProcessor* pProc, int x, int y, WORD* arrR, BOOL* arrRChanged)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorText = GetSysColor(COLOR_WINDOWTEXT);

    DrawRectangle(hdc, x - cxChar, y - 8, x + cxChar + 31 * cxChar, y + 8 + cyLine * 14);

    // Registers
    for (int r = 0; r < 8; r++)
    {
        ::SetTextColor(hdc, arrRChanged[r] ? COLOR_RED : colorText);

        LPCTSTR strRegName = REGISTER_NAME[r];
        TextOut(hdc, x, y + r * cyLine, strRegName, (int) _tcslen(strRegName));

        WORD value = arrR[r]; //pProc->GetReg(r);
        DrawOctalValue(hdc, x + cxChar * 3, y + r * cyLine, value);
        DrawHexValue(hdc, x + cxChar * 10, y + r * cyLine, value);
        DrawBinaryValue(hdc, x + cxChar * 15, y + r * cyLine, value);
    }
    ::SetTextColor(hdc, colorText);

    // CPC value
    ::SetTextColor(hdc, arrRChanged[9] ? COLOR_RED : colorText);
    TextOut(hdc, x, y + 8 * cyLine, _T("PC'"), 3);
    WORD cpc = arrR[9];
    DrawOctalValue(hdc, x + cxChar * 3, y + 8 * cyLine, cpc);
    DrawHexValue(hdc, x + cxChar * 10, y + 8 * cyLine, cpc);
    DrawBinaryValue(hdc, x + cxChar * 15, y + 8 * cyLine, cpc);

    // PSW value
    ::SetTextColor(hdc, arrRChanged[8] ? COLOR_RED : colorText);
    TextOut(hdc, x, y + 10 * cyLine, _T("PS"), 2);
    WORD psw = arrR[8]; // pProc->GetPSW();
    DrawOctalValue(hdc, x + cxChar * 3, y + 10 * cyLine, psw);
    DrawHexValue(hdc, x + cxChar * 10, y + 10 * cyLine, psw);
    TextOut(hdc, x + cxChar * 15, y + 9 * cyLine, _T("       HP  TNZVC"), 16);
    DrawBinaryValue(hdc, x + cxChar * 15, y + 10 * cyLine, psw);

    // CPSW value
    ::SetTextColor(hdc, arrRChanged[10] ? COLOR_RED : colorText);
    TextOut(hdc, x, y + 11 * cyLine, _T("PS'"), 3);
    WORD cpsw = arrR[10];
    DrawOctalValue(hdc, x + cxChar * 3, y + 11 * cyLine, cpsw);
    DrawHexValue(hdc, x + cxChar * 10, y + 11 * cyLine, cpsw);
    DrawBinaryValue(hdc, x + cxChar * 15, y + 11 * cyLine, cpsw);

    ::SetTextColor(hdc, colorText);

    // Processor mode - HALT or USER
    BOOL okHaltMode = pProc->IsHaltMode();
    TextOut(hdc, x, y + 13 * cyLine, okHaltMode ? _T("HALT") : _T("USER"), 4);

    // "Stopped" flag
    BOOL okStopped = pProc->IsStopped();
    if (okStopped)
        TextOut(hdc, x + 6 * cxChar, y + 13 * cyLine, _T("STOP"), 4);

}

void DebugView_DrawMemoryForRegister(HDC hdc, int reg, CProcessor* pProc, int x, int y)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);

    WORD current = pProc->GetReg(reg);
    BOOL okExec = (reg == 7);

    // ������ �� ������ ���������� � �����
    WORD memory[16];
    const CMemoryController* pMemCtl = pProc->GetMemoryController();
    for (int idx = 0; idx < 16; idx++)
    {
        BOOL okValidAddress;
        memory[idx] = pMemCtl->GetWordView(
                (WORD)(current + idx * 2 - 16), pProc->IsHaltMode(), okExec, &okValidAddress);
    }

    WORD address = current - 16;
    for (int index = 0; index < 16; index++)    // ������ ������
    {
        // �����
        DrawOctalValue(hdc, x + 4 * cxChar, y, address);

        // �������� �� ������
        WORD value = memory[index];
        DrawOctalValue(hdc, x + 12 * cxChar, y, value);

        // ������� �������
        if (address == current)
        {
            TextOut(hdc, x + 2 * cxChar, y, _T(">>"), 2);
            TextOut(hdc, x, y, REGISTER_NAME[reg], 2);
        }

        address += 2;
        y += cyLine;
    }

}

void DebugView_DrawPorts(HDC hdc, BOOL okProcessor, const CMemoryController* pMemCtl, CMotherboard* pBoard, int x, int y)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);

    TextOut(hdc, x, y, _T("Ports:"), 6);

    if (okProcessor)  // CPU
    {
        WORD value176640 = pMemCtl->GetPortView(0176640);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 1 * cyLine, 0176640);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 1 * cyLine, value176640);
        WORD value176642 = pMemCtl->GetPortView(0176642);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 2 * cyLine, 0176642);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 2 * cyLine, value176642);

        //TODO
    }
    else  // PPU
    {
        WORD value177010 = pMemCtl->GetPortView(0177010);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 1 * cyLine, 0177010);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 1 * cyLine, value177010);
        WORD value177012 = pMemCtl->GetPortView(0177012);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 2 * cyLine, 0177012);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 2 * cyLine, value177012);
        WORD value177014 = pMemCtl->GetPortView(0177014);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 3 * cyLine, 0177014);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 3 * cyLine, value177014);
        WORD value177016 = pMemCtl->GetPortView(0177016);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 4 * cyLine, 0177016);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 4 * cyLine, value177016);
        WORD value177020 = pMemCtl->GetPortView(0177020);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 5 * cyLine, 0177020);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 5 * cyLine, value177020);
        WORD value177022 = pMemCtl->GetPortView(0177022);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 6 * cyLine, 0177022);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 6 * cyLine, value177022);
        WORD value177024 = pMemCtl->GetPortView(0177024);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 7 * cyLine, 0177024);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 7 * cyLine, value177024);
        WORD value177026 = pMemCtl->GetPortView(0177026);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 8 * cyLine, 0177026);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 8 * cyLine, value177026);
        WORD value177054 = pMemCtl->GetPortView(0177054);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 9 * cyLine, 0177054);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 9 * cyLine, value177054);
        WORD value177700 = pMemCtl->GetPortView(0177700);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 10 * cyLine, 0177700);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 10 * cyLine, value177700);
        WORD value177716 = pMemCtl->GetPortView(0177716);
        DrawOctalValue(hdc, x + 0 * cxChar, y + 11 * cyLine, 0177716);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 11 * cyLine, value177716);

        WORD value177710 = pBoard->GetTimerStateView();
        DrawOctalValue(hdc, x + 0 * cxChar, y + 13 * cyLine, 0177710);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 13 * cyLine, value177710);
        WORD value177712 = pBoard->GetTimerReloadView();
        DrawOctalValue(hdc, x + 0 * cxChar, y + 14 * cyLine, 0177712);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 14 * cyLine, value177712);
        WORD value177714 = pBoard->GetTimerValueView();
        DrawOctalValue(hdc, x + 0 * cxChar, y + 15 * cyLine, 0177714);
        DrawOctalValue(hdc, x + 8 * cxChar, y + 15 * cyLine, value177714);
    }
}

void DebugView_DrawChannels(HDC hdc, int x, int y)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);

    //CProcessor* pCPU = g_pBoard->GetCPU();
    //CProcessor* pPPU = g_pBoard->GetPPU();
    //CMemoryController* pCPUMemCtl = pCPU->GetMemoryController();
    //CMemoryController* pPPUMemCtl = pPPU->GetMemoryController();

    TextOut(hdc, x, y, _T("Channels:"), 9);

    TCHAR bufData[7];
    TCHAR buffer[32];
    chan_stc tmpstc;


    tmpstc = g_pBoard->GetChannelStruct(0, 0, 0);

    PrintOctalValue(bufData, tmpstc.data);
    wsprintf(buffer, _T("PPU CH:0 RX D:%s RDY:%d IRQ:%d"), bufData + 3, tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 1 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(0, 1, 0);
    PrintOctalValue(bufData, tmpstc.data);
    wsprintf(buffer, _T("PPU CH:1 RX D:%s RDY:%d IRQ:%d"), bufData + 3, tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 2 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(0, 2, 0);
    PrintOctalValue(bufData, tmpstc.data);
    wsprintf(buffer, _T("PPU CH:2 RX D:%s RDY:%d IRQ:%d"), bufData + 3, tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 3 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(0, 0, 1);
    wsprintf(buffer, _T("PPU CH:0 TX       RDY:%d IRQ:%d"), tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 4 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(0, 1, 1);
    wsprintf(buffer, _T("PPU CH:1 TX       RDY:%d IRQ:%d"), tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 5 * cyLine, buffer, lstrlen(buffer));


    tmpstc = g_pBoard->GetChannelStruct(1, 0, 0);
    PrintOctalValue(bufData, tmpstc.data);
    wsprintf(buffer, _T("CPU CH:0 RX D:%s RDY:%d IRQ:%d"), bufData + 3, tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 6 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(1, 1, 0);
    PrintOctalValue(bufData, tmpstc.data);
    wsprintf(buffer, _T("CPU CH:1 RX D:%s RDY:%d IRQ:%d"), bufData + 3, tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 7 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(1, 0, 1);
    wsprintf(buffer, _T("CPU CH:0 TX       RDY:%d IRQ:%d"), tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 8 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(1, 1, 1);
    wsprintf(buffer, _T("CPU CH:1 TX       RDY:%d IRQ:%d"), tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 9 * cyLine, buffer, lstrlen(buffer));

    tmpstc = g_pBoard->GetChannelStruct(1, 2, 1);
    wsprintf(buffer, _T("CPU CH:2 TX       RDY:%d IRQ:%d"), tmpstc.ready, tmpstc.irq);
    TextOut(hdc, x, y + 10 * cyLine, buffer, lstrlen(buffer));


}


//////////////////////////////////////////////////////////////////////
