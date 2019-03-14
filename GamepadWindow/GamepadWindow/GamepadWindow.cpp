// GamepadWindow.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <cassert>
#include <memory>
#include <iostream>
#include "gamepad_raw_input_win.h"
#include "GamepadWindow.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
CHAR szTitle[MAX_LOADSTRING];                  // The title bar text
CHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

using namespace gamepad;

std::unordered_map<HANDLE,
  std::shared_ptr<GamepadRawInputWin>> controllers_;

HWND ghWnd = 0;

bool Init() {
	// Getting input device list
	UINT count = 0;
	UINT result = ::GetRawInputDeviceList(nullptr, &count,
		sizeof(RAWINPUTDEVICELIST));
	if (result == static_cast<UINT>(-1)) {
		std::cerr << "GetRawInputDeviceList() failed";
		return EXIT_FAILURE;
	}
	assert(0u == result);

  // Register to receive raw HID input.
  const uint16_t DeviceUsages[] = {
    GamepadRawInputWin::kGenericDesktopJoystick,
    GamepadRawInputWin::kGenericDesktopGamePad,
    GamepadRawInputWin::kGenericDesktopMultiAxisController,
  };

 /* std::unique_ptr<RAWINPUTDEVICE[]> devices(
    GetRawInputDevices(RIDEV_INPUTSINK));*/
  assert(ghWnd);
  size_t usage_count = sizeof(DeviceUsages);
  std::unique_ptr<RAWINPUTDEVICE[]> devices(new RAWINPUTDEVICE[usage_count]);
  for (size_t i = 0; i < usage_count; ++i) {
    devices[i].dwFlags = RIDEV_INPUTSINK;
    devices[i].usUsagePage = 1;
    devices[i].usUsage = DeviceUsages[i];
    devices[i].hwndTarget = (RIDEV_INPUTSINK & RIDEV_REMOVE) ? 0 : ghWnd;
  }
  if (!::RegisterRawInputDevices(devices.get(), sizeof(DeviceUsages),
    sizeof(RAWINPUTDEVICE))) {
    assert(false && "RegisterRawInputDevices() failed for RIDEV_INPUTSINK");
  //  window_.reset();
    return false;
  }

	std::shared_ptr<RAWINPUTDEVICELIST[]> device_list(
		new RAWINPUTDEVICELIST[count]);
	result = ::GetRawInputDeviceList(device_list.get(), &count,
		sizeof(RAWINPUTDEVICELIST));
	if (result == static_cast<UINT>(-1)) {
		std::cerr << "GetRawInputDeviceList() failed";
	}
	assert(count == result);

	for (UINT i = 0; i < count; ++i) {
		if (device_list[i].dwType == RIM_TYPEHID) {
			HANDLE device_handle = device_list[i].hDevice;
			auto controller_it = controllers_.find(device_handle);

			if (controller_it == controllers_.end()) {
				auto gamepad = std::make_shared<GamepadRawInputWin>(device_list[i].hDevice);

				if (gamepad->IsValid() && gamepad->IsDS4()) {
					//
					gamepad->QueryDeviceCapabilities();
				}

				// We should skip xinput?
				controllers_.emplace(device_handle, gamepad);
			}
			else {
				std::cout << "there is a duplicated gamepad.";
			}
		}
	}

	return true;
}

LRESULT OnInput(HRAWINPUT input_handle) {
  UINT size = 0;
  UINT result = GetRawInputData(input_handle, RID_INPUT, nullptr, &size,
    sizeof(RAWINPUTHEADER));
  if (result == static_cast<UINT>(-1)) {
    assert(false && "GetRawInputData() failed.");
    return false;
  }
  assert(result == 0);

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  RAWINPUT* input = reinterpret_cast<RAWINPUT*>(buffer.get());
  result = GetRawInputData(input_handle, RID_INPUT, buffer.get(), &size,
    sizeof(RAWINPUTHEADER));
  if (result == static_cast<UINT>(-1)) {
    assert(false && "GetRawInputData failed.");
    return false;
  }
  assert(size == result);

  if (input->header.dwType == RIM_TYPEHID && input->header.hDevice != NULL) {
    auto it = controllers_.find(input->header.hDevice);
    if (it != controllers_.end()) {
      it->second->UpdateGamepad(input);
      it->second->ReadPadState();
    }
  }

  return DefRawInputProc(&input, 1, sizeof(RAWINPUTHEADER));
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // TODO: Place code here.

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_GAMEPADWINDOW, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow))
  {
    return FALSE;
  }

  HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMEPADWINDOW));

  Init();

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMEPADWINDOW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_GAMEPADWINDOW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   ghWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!ghWnd)
   {
      return FALSE;
   }

   ShowWindow(ghWnd, nCmdShow);
   UpdateWindow(ghWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_COMMAND:
    {
      int wmId = LOWORD(wParam);
      // Parse the menu selections:
      switch (wmId)
      {
      case IDM_ABOUT:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        break;
      case IDM_EXIT:
        DestroyWindow(hWnd);
        break;
      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
      }
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      // TODO: Add any drawing code that uses hdc here...
      EndPaint(hWnd, &ps);
    }
    break;
    case WM_KEYDOWN:
      std::cout << "test..." << std::endl;
      break;
    case WM_INPUT:
	  {
		  LRESULT result = OnInput(reinterpret_cast<HRAWINPUT>(lParam));
      // GetGamepadState

	  }
	  break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  switch (message)
  {
  case WM_INITDIALOG:
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}
