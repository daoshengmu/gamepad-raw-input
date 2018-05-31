
#ifndef GAMEPAD_RAW_INPUT_WIN_H
#define GAMEPAD_RAW_INPUT_WIN_H

#include "hid_dll_functions_win.h"

#include <memory>

namespace gamepad {

class GamepadRawInputWin {

public:
  GamepadRawInputWin() = default;

  bool QueryHidInfo();
  bool QueryDeviceCapabilities();
  bool QueryButtonCapabilities();
  bool QueryAxisCapabilities();

private:
  std::shared_ptr<HidDllFunctionsWin> hid_functions_;

};

} // namespace gamepad

#endif // GAMEPAD_RAW_INPUT_WIN_H