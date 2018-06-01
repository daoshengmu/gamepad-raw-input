
#ifndef GAMEPAD_RAW_INPUT_WIN_H
#define GAMEPAD_RAW_INPUT_WIN_H

#include "hid_dll_functions_win.h"

#include <memory>
#include <unordered_map>

namespace gamepad {

class GamepadRawInputWin {

public:
  explicit GamepadRawInputWin(HANDLE device_handle);

  bool IsValid() const { return is_valid_; }
  bool IsDS4() const { return is_ds4_; }
  bool IsGamepadUsageId();
  bool QueryDeviceInfo();
  bool QueryHidInfo();
  bool QueryDeviceName();
  bool QueryProductString();
  bool QueryDeviceCapabilities();
  bool QueryButtonCapabilities(uint16_t button_count);
  bool QueryAxisCapabilities(uint16_t axis_count);

private:
  struct RawGamepadAxis {
    HIDP_VALUE_CAPS caps;
	float value;
	bool active;
	unsigned long bitmask;
  };

  static const uint32_t kButtonsLengthCap = 32;
  static const uint32_t kAxesLengthCap = 16;

  HANDLE handle_ = nullptr;
  std::shared_ptr<HidDllFunctionsWin> hid_functions_;
  PHIDP_PREPARSED_DATA preparsed_data_ = nullptr;
  std::shared_ptr<uint8_t[]> ppd_buffer_;

  bool is_valid_ = false;
  bool is_ds4_ = false;

  uint32_t vendor_id_ = 0;
  uint32_t product_id_ = 0;
  uint32_t version_number_ = 0;
  uint16_t usage_ = 0;
  std::string name_;
  std::string product_string_;

  size_t buttons_length_ = 0;
  bool buttons_[kButtonsLengthCap];

  size_t axes_length_ = 0;
  RawGamepadAxis axes_[kAxesLengthCap];
};

} // namespace gamepad

#endif // GAMEPAD_RAW_INPUT_WIN_H