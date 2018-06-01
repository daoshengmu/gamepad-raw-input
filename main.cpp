
#include <memory>
#include <iostream>
#include <cassert>
#include "gamepad_raw_input_win.h"

using namespace gamepad;


int main()
{
  // Getting input device list
  UINT count = 0;
  UINT result = ::GetRawInputDeviceList(nullptr, &count,
    sizeof(RAWINPUTDEVICELIST));
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceList() failed";
    return EXIT_FAILURE;
  }
  assert(0u == result);

  std::shared_ptr<RAWINPUTDEVICELIST[]> device_list(
    new RAWINPUTDEVICELIST[count]);
  result = ::GetRawInputDeviceList(device_list.get(), &count,
    sizeof(RAWINPUTDEVICELIST));
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceList() failed";
  }
  assert(count == result);

  std::unordered_map<HANDLE,
	  std::shared_ptr<GamepadRawInputWin>> controllers_;

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
	  } else {
	    std::cout << "there is a duplicated gamepad.";
	  }
	}
  }

  return EXIT_SUCCESS;
}
