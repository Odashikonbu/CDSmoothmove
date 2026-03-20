#include "pch.h"
#include <windows.h>
#include <xinput.h>
#include <cmath>
#include <process.h>

#pragma comment(lib, "xinput.lib")

const SHORT STICK_THRESHOLD_ON = 21844; // 32767 / 2 (半分以上でON)
const SHORT STICK_THRESHOLD_OFF = 14000; // チャタリング防止用の遊び

bool g_isShiftPressed = false;
bool g_isRunning = true;

void SendShiftKey(bool down) {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_LSHIFT;
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

unsigned __stdcall MonitorThread(void* pArguments) {
    while (g_isRunning) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        // XInputデバイス（Xbox系/エミュレートされたDualSense）を取得
        if (XInputGetState(0, &state) == ERROR_SUCCESS) {
            SHORT lx = state.Gamepad.sThumbLX;
            SHORT ly = state.Gamepad.sThumbLY;

            double squaredMagnitude = (double)lx * lx + (double)ly * ly;
            double thresholdOnSq = (double)STICK_THRESHOLD_ON * STICK_THRESHOLD_ON;
            double thresholdOffSq = (double)STICK_THRESHOLD_OFF * STICK_THRESHOLD_OFF;

            if (!g_isShiftPressed && squaredMagnitude > thresholdOnSq) {
                SendShiftKey(true);
                g_isShiftPressed = true;
            }
            else if (g_isShiftPressed && squaredMagnitude < thresholdOffSq) {
                SendShiftKey(false);
                g_isShiftPressed = false;
            }
        }
        Sleep(16);
    }
    if (g_isShiftPressed) SendShiftKey(false);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        _beginthreadex(NULL, 0, MonitorThread, NULL, 0, NULL);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        g_isRunning = false;
    }
    return TRUE;
}