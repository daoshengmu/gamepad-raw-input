
#include "gamepad_raw_input_win.h"

#include <memory>

using namespace gamepad;


int main()
{
  // Getting the device handle
  UINT count = 0;
  UINT result =
      ::GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST));
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceList() failed";
    return;
  }
  std::shared_ptr<RAWINPUTDEVICELIST[]> device_list(
                                        new RAWINPUTDEVICELIST[count]);
  result = ::GetRawInputDeviceList(device_list.get(), &count,
                                   sizeof(RAWINPUTDEVICELIST));

  auto gamepad = std::make_shared<GamepadRawInputWin>();
  // QueryHidInfo

  //
  gamepad.QueryDeviceCapabilities();

  return 0;
}
