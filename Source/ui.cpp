#include <Windows.h>
#include <shellapi.h>

#include <cstdio>

#include "ui.h"
#include "resource.h"
#include "settings.h"
#include "RFU.h"

#define BFU_TRAYICON			WM_APP + 1
#define BFU_TRAYMENU_APC		WM_APP + 2
#define BFU_TRAYMENU_CONSOLE	WM_APP + 3
#define BFU_TRAYMENU_EXIT		WM_APP + 4
#define BFU_TRAYMENU_VSYNC		WM_APP + 5
#define BFU_TRAYMENU_LOADSET	WM_APP + 6
#define BFU_TRAYMENU_GITHUB		WM_APP + 7
#define BFU_TRAYMENU_STUDIO		WM_APP + 8
#define BFU_TRAYMENU_CFU		WM_APP + 9
#define BFU_TRAYMENU_ADV_NBE	WM_APP + 10
#define BFU_TRAYMENU_ADV_SE		WM_APP + 11
#define BFU_TRAYMENU_ADV_QS		WM_APP + 12
#define BFU_TRAYMENU_CLIENT		WM_APP + 13

#define BFU_FCS_FIRST			(WM_APP + 20)
#define BFU_FCS_NONE			BFU_FCS_FIRST + 0
#define BFU_FCS_30				BFU_FCS_FIRST + 1
#define BFU_FCS_60				BFU_FCS_FIRST + 2
#define BFU_FCS_75				BFU_FCS_FIRST + 3
#define BFU_FCS_120				BFU_FCS_FIRST + 4
#define BFU_FCS_144				BFU_FCS_FIRST + 5
#define BFU_FCS_240				BFU_FCS_FIRST + 6
#define BFU_FCS_360				BFU_FCS_FIRST + 7
#define BFU_FCS_480				BFU_FCS_FIRST + 8
#define BFU_FCS_580				BFU_FCS_FIRST + 9
#define BFU_FCS_LAST			(BFU_FCS_580)

HWND UI::Window = NULL;
int UI::AttachedProcessesCount = 0;
bool UI::IsConsoleOnly = false;

HANDLE WatchThread;
NOTIFYICONDATA NotifyIconData;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case BFU_TRAYICON:
	{
		if (lParam == WM_RBUTTONDOWN || lParam == WM_LBUTTONDOWN)
		{
			POINT position;
			GetCursorPos(&position);

			HMENU popup = CreatePopupMenu();

			char buffer[64];
			sprintf_s(buffer, "Attached Processes: %d", UI::AttachedProcessesCount);

			AppendMenu(popup, MF_STRING | MF_GRAYED, BFU_TRAYMENU_APC, buffer);
			AppendMenu(popup, MF_SEPARATOR, 0, NULL);

			AppendMenu(popup, MF_STRING | (Settings::UnlockClient ? MF_CHECKED : 0), BFU_TRAYMENU_CLIENT, "Unlock Bubbaverse Player");
			AppendMenu(popup, MF_STRING | (Settings::UnlockStudio ? MF_CHECKED : 0), BFU_TRAYMENU_STUDIO, "unlock studio (dont exist)");
			AppendMenu(popup, MF_STRING | (Settings::CheckForUpdates ? MF_CHECKED : 0), BFU_TRAYMENU_CFU, "Check for Updates");

			HMENU submenu = CreatePopupMenu();
			AppendMenu(submenu, MF_STRING, BFU_FCS_NONE, "None");
			AppendMenu(submenu, MF_STRING, BFU_FCS_30, "30");
			AppendMenu(submenu, MF_STRING, BFU_FCS_60, "60");
			AppendMenu(submenu, MF_STRING, BFU_FCS_75, "75");
			AppendMenu(submenu, MF_STRING, BFU_FCS_120, "120");
			AppendMenu(submenu, MF_STRING, BFU_FCS_144, "144");
			AppendMenu(submenu, MF_STRING, BFU_FCS_240, "240");
			AppendMenu(submenu, MF_STRING, BFU_FCS_360, "360");
			AppendMenu(submenu, MF_STRING, BFU_FCS_480, "480");
			AppendMenu(submenu, MF_STRING, BFU_FCS_580, "580");
			CheckMenuRadioItem(submenu, BFU_FCS_FIRST, BFU_FCS_LAST, BFU_FCS_FIRST + Settings::FPSCapSelection, MF_BYCOMMAND);
			AppendMenu(popup, MF_POPUP, (UINT_PTR)submenu, "FPS Cap");

			HMENU advanced = CreatePopupMenu();
			AppendMenu(advanced, MF_STRING | (Settings::SilentErrors ? MF_CHECKED : 0), BFU_TRAYMENU_ADV_SE, "Silent Errors");
			AppendMenu(advanced, MF_STRING | (Settings::SilentErrors ? MF_GRAYED : 0) | (Settings::NonBlockingErrors ? MF_CHECKED : 0), BFU_TRAYMENU_ADV_NBE, "Use Console Errors");
			AppendMenu(advanced, MF_STRING | (Settings::QuickStart ? MF_CHECKED : 0), BFU_TRAYMENU_ADV_QS, "Quick Start");
			AppendMenu(popup, MF_POPUP, (UINT_PTR)advanced, "Advanced");

			AppendMenu(popup, MF_SEPARATOR, 0, NULL);
			AppendMenu(popup, MF_STRING, BFU_TRAYMENU_LOADSET, "Load Settings");
			AppendMenu(popup, MF_STRING, BFU_TRAYMENU_CONSOLE, "Toggle Console");
			AppendMenu(popup, MF_STRING, BFU_TRAYMENU_GITHUB, "Visit GitHub");
			AppendMenu(popup, MF_STRING, BFU_TRAYMENU_EXIT, "Exit");

			SetForegroundWindow(hwnd); // to allow "clicking away"
			BOOL result = TrackPopupMenu(popup, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, position.x, position.y, 0, hwnd, NULL);

			if (result != 0)
			{
				switch (result)
				{
				case BFU_TRAYMENU_EXIT:
					SetFPSCapExternal(60);
					Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
					TerminateThread(WatchThread, 0);
					FreeConsole();
					PostQuitMessage(0);
					break;

				case BFU_TRAYMENU_CONSOLE:
					UI::ToggleConsole();
					break;

				case BFU_TRAYMENU_GITHUB:
					ShellExecuteA(NULL, "open", "https://github.com/" BFU_GITHUB_REPO, NULL, NULL, SW_SHOWNORMAL);
					break;

				case BFU_TRAYMENU_LOADSET:
					Settings::Load();
					Settings::Update();
					break;

				case BFU_TRAYMENU_CLIENT:
					Settings::UnlockClient = !Settings::UnlockClient;
					CheckMenuItem(popup, BFU_TRAYMENU_CLIENT, Settings::UnlockClient ? MF_CHECKED : MF_UNCHECKED);
					break;

				case BFU_TRAYMENU_STUDIO:
					Settings::UnlockStudio = !Settings::UnlockStudio;
					CheckMenuItem(popup, BFU_TRAYMENU_STUDIO, Settings::UnlockStudio ? MF_CHECKED : MF_UNCHECKED);
					break;

				case BFU_TRAYMENU_CFU:
					Settings::CheckForUpdates = !Settings::CheckForUpdates;
					CheckMenuItem(popup, BFU_TRAYMENU_CFU, Settings::CheckForUpdates ? MF_CHECKED : MF_UNCHECKED);
					break;

				case BFU_TRAYMENU_ADV_NBE:
					Settings::NonBlockingErrors = !Settings::NonBlockingErrors;
					CheckMenuItem(popup, BFU_TRAYMENU_ADV_NBE, Settings::NonBlockingErrors ? MF_CHECKED : MF_UNCHECKED);
					break;

				case BFU_TRAYMENU_ADV_SE:
					Settings::SilentErrors = !Settings::SilentErrors;
					CheckMenuItem(popup, BFU_TRAYMENU_ADV_SE, Settings::SilentErrors ? MF_CHECKED : MF_UNCHECKED);
					if (Settings::SilentErrors) CheckMenuItem(popup, BFU_TRAYMENU_ADV_NBE, MF_GRAYED);
					break;

				case BFU_TRAYMENU_ADV_QS:
					Settings::QuickStart = !Settings::QuickStart;
					CheckMenuItem(popup, BFU_TRAYMENU_ADV_QS, Settings::QuickStart ? MF_CHECKED : MF_UNCHECKED);
					break;

				default:
					if (result >= BFU_FCS_FIRST
						&& result <= BFU_FCS_LAST)
					{
						static double fcs_map[] = { 0.0, 30.0, 60.0, 75.0, 120.0, 144.0, 240.0, 360.0, 480.0, 580.0 };
						Settings::FPSCapSelection = result - BFU_FCS_FIRST;
						Settings::FPSCap = fcs_map[Settings::FPSCapSelection];
					}
				}

				if (result != BFU_TRAYMENU_CONSOLE
					&& result != BFU_TRAYMENU_LOADSET
					&& result != BFU_TRAYMENU_GITHUB
					&& result != BFU_TRAYMENU_EXIT)
				{
					Settings::Update();
					Settings::Save();
				}
			}

			return 1;
		}

		break;
	}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

bool IsConsoleVisible = false;

void UI::SetConsoleVisible(bool visible)
{
	IsConsoleVisible = visible;
	ShowWindow(GetConsoleWindow(), visible ? SW_SHOWNORMAL : SW_HIDE);
}

void UI::CreateHiddenConsole()
{
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);

	if (!UI::IsConsoleOnly)
	{
		HMENU menu = GetSystemMenu(GetConsoleWindow(), FALSE);
		EnableMenuItem(menu, SC_CLOSE, MF_GRAYED);
	}

#ifdef _WIN64
	SetConsoleTitleA("Bubbaverse FPS Unlocker " BFU_VERSION " (64-bit) Console");
#else
	SetConsoleTitleA("Bubbaverse FPS Unlocker " BFU_VERSION " (32-bit) Console");
#endif

	SetConsoleVisible(false);
}

bool UI::ToggleConsole()
{
	if (!GetConsoleWindow())
		UI::CreateHiddenConsole();

	SetConsoleVisible(!IsConsoleVisible);

	return IsConsoleVisible;
}

int UI::Start(HINSTANCE instance, LPTHREAD_START_ROUTINE watchthread)
{	
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(wcex);
	wcex.style = 0;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "BFUClass";
	wcex.hIconSm = NULL;

	RegisterClassEx(&wcex);

	UI::Window = CreateWindow("BFUClass", "Bubbaverse FPS Unlocker", 0, 0, 0, 0, 0, NULL, NULL, instance, NULL);
	if (!UI::Window)
		return 0;

	NotifyIconData.cbSize = sizeof(NotifyIconData);
	NotifyIconData.hWnd = UI::Window;
	NotifyIconData.uID = IDI_ICON1;
	NotifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	NotifyIconData.uCallbackMessage = BFU_TRAYICON;
	NotifyIconData.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
	strcpy_s(NotifyIconData.szTip, "Bubbaverse FPS Unlocker");

	Shell_NotifyIcon(NIM_ADD, &NotifyIconData);

	WatchThread = CreateThread(NULL, 0, watchthread, NULL, NULL, NULL);

	BOOL ret;
	MSG msg;

	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
