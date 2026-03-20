#include "pch.h"
#include <windows.h>
#include <xinput.h>
#include <cmath>
#include <process.h>
#include <string>

#pragma comment(lib, "xinput.lib")

// 変数（iniで上書きするため大文字にしない慣習に従いつつ、g_を付与）
long g_thresholdOn = 21844;
long g_thresholdOff = 14000;

bool g_isShiftPressed = false;
bool g_isRunning = true;

void LoadSettings(HMODULE hModule) {
    char path[MAX_PATH];
    if (GetModuleFileNameA(hModule, path, MAX_PATH)) {
        std::string iniPath = path;
        size_t lastDot = iniPath.find_last_of(".");
        if (lastDot != std::string::npos) {
            iniPath = iniPath.substr(0, lastDot) + ".ini";
        }

        // 浮動小数点での計算を確実にするため 100.0 で割る
        int pctOn = GetPrivateProfileIntA("Settings", "ThresholdOnPct", 50, iniPath.c_str());
        int pctOff = GetPrivateProfileIntA("Settings", "ThresholdOffPct", 40, iniPath.c_str());

        // 計算の順番を変えて精度を保つ (32767 * pct / 100)
        g_thresholdOn = (long)(32767.0 * ((double)pctOn / 100.0));
        g_thresholdOff = (long)(32767.0 * ((double)pctOff / 100.0));
    }
}

void SendShiftKey(bool down) {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_LSHIFT;
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

unsigned __stdcall MonitorThread(void* pArguments) {
    // ループ内で毎回計算せず、あらかじめ二乗しておく（long longでオーバーフロー防止）
    long long onSq = (long long)g_thresholdOn * g_thresholdOn;
    long long offSq = (long long)g_thresholdOff * g_thresholdOff;

    while (g_isRunning) {
        XINPUT_STATE state;
        if (XInputGetState(0, &state) == ERROR_SUCCESS) {
            long lx = state.Gamepad.sThumbLX;
            long ly = state.Gamepad.sThumbLY;
            long long magSq = (long long)lx * lx + (long long)ly * ly;

            if (!g_isShiftPressed && magSq > onSq) {
                SendShiftKey(true);
                g_isShiftPressed = true;
            }
            else if (g_isShiftPressed && magSq < offSq) {
                SendShiftKey(false);
                g_isShiftPressed = false;
            }
        }
        Sleep(10); // 反応速度を上げるため少し短縮
    }
    if (g_isShiftPressed) SendShiftKey(false);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        LoadSettings(hModule);
        _beginthreadex(NULL, 0, MonitorThread, NULL, 0, NULL);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        g_isRunning = false;
    }
    return TRUE;
}