
#include <iostream>
#include <cassert>
#include <algorithm>
#include "gamepad_raw_input_win.h"

#undef min
#undef max

namespace gamepad {

const uint32_t kGameControlsUsagePage = 0x05;
const uint32_t kButtonUsagePage = 0x9;
const uint32_t kAxisMinimumUsageNumber = 0x30;
const uint32_t kIdLengthCap = 128;

const uint32_t kVendorSony = 0x54c;
const uint32_t kProductDualshock4 = 0x05c4;
const uint32_t kProductDualshock4Slim = 0x9cc;

const uint32_t kGenericDesktopJoystick = 0x04U;
const uint32_t kGenericDesktopGamePad = 0x05U;
const uint32_t kGenericDesktopMultiAxisController = 0x08U;


enum ControllerType {
  UNKNOWN_CONTROLLER,
  DUALSHOCK4_CONTROLLER,
  DUALSHOCK4_SLIM_CONTROLLER
};

unsigned long GetBitmask(unsigned short bits) {
  return (1 << bits) - 1;
}

GamepadRawInputWin::GamepadRawInputWin(HANDLE device_handle)
 : handle_(device_handle),
   hid_functions_(std::make_shared<HidDllFunctionsWin>())
{
  ::ZeroMemory(buttons_, sizeof(buttons_));
  ::ZeroMemory(axes_, sizeof(axes_));
 
  if (hid_functions_->IsValid()) {
	is_valid_ = QueryDeviceInfo();
  }
}

bool GamepadRawInputWin::IsGamepadUsageId() {
	return usage_ == kGenericDesktopJoystick || usage_ == kGenericDesktopGamePad ||
		   usage_ == kGenericDesktopMultiAxisController;
}

bool GamepadRawInputWin::QueryDeviceInfo() {

  if (!QueryHidInfo()) {
    return false;
  }

  if (!IsGamepadUsageId()) {
	return false;
  }

  // Fail to get the correct device and product name.
  if (!QueryDeviceName()) {
	return false;
  }
  
  if (!QueryProductString()) {
    return false;
  }

  ControllerType type = UNKNOWN_CONTROLLER;
  if (vendor_id_ == kVendorSony) {
	switch (product_id_) {
	 case kProductDualshock4:
	   type = DUALSHOCK4_CONTROLLER;
	   break;
	 case kProductDualshock4Slim:
	   type = DUALSHOCK4_SLIM_CONTROLLER;
	   break;
	 default:
	   break;
	}
  }

  if (type == DUALSHOCK4_CONTROLLER ||
	  type == DUALSHOCK4_SLIM_CONTROLLER) {
    is_ds4_ = true;
  }

  return true;
}

bool GamepadRawInputWin::QueryHidInfo() {
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceInfo() failed";
    return false;
  }
  assert(0u == result);

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, buffer.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceInfo() failed";
    return false;
  }
  assert(size == result);
  RID_DEVICE_INFO* device_info =
      reinterpret_cast<RID_DEVICE_INFO*>(buffer.get());

  assert(device_info->dwType == static_cast<DWORD>(RIM_TYPEHID));
  vendor_id_ = device_info->hid.dwVendorId;
  product_id_ = device_info->hid.dwProductId;
  version_number_ = device_info->hid.dwVersionNumber;
  usage_ = device_info->hid.usUsage;

  return true;
}

bool GamepadRawInputWin::QueryDeviceName() {
  UINT size = 0;
  UINT result = ::GetRawInputDeviceInfo(handle_, RIDI_DEVICENAME,
                                        nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
	  std::cerr << "GetRawInputDeviceInfo() failed";
	  return false;
  }
  assert(0u == result);

  std::shared_ptr<char[]> buffer(new char[size]);
  result = ::GetRawInputDeviceInfo(handle_, RIDI_DEVICENAME,
	  buffer.get(), &size);
  if (result == static_cast<UINT>(-1)) {
	  std::cerr << "GetRawInputDeviceInfo() failed";
	  return false;
  }

  /*assert(size == result);*/

  name_ = buffer.get();

  return true;
}

bool GamepadRawInputWin::QueryProductString() {
  product_string_.resize(kIdLengthCap);
  HANDLE hid_handle = ::CreateFile(name_.c_str(), GENERIC_READ | GENERIC_WRITE,
	  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

  if (!hid_handle) {
	::CloseHandle(hid_handle);
    return false;
  }

  bool result = hid_functions_->HidDGetProductString()(hid_handle, &product_string_.front(),
                                                       kIdLengthCap);
  ::CloseHandle(hid_handle);

  return result;
}

bool GamepadRawInputWin::QueryDeviceCapabilities() {
  assert(hid_functions_);
  assert(hid_functions_->IsValid());
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceInfo() failed";
    return false;
  }
  assert(0u == result);

  ppd_buffer_.reset(new uint8_t[size]);
  preparsed_data_ = reinterpret_cast<PHIDP_PREPARSED_DATA>(ppd_buffer_.get());
  result = ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA,
                                   ppd_buffer_.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    std::cerr << "GetRawInputDeviceInfo() failed";
    return false;
  }
  assert(size == result);

  HIDP_CAPS caps;
  NTSTATUS status = hid_functions_->HidPGetCaps()(preparsed_data_, &caps);
  assert(HIDP_STATUS_SUCCESS == status);

  QueryButtonCapabilities(caps.NumberInputButtonCaps);
  QueryAxisCapabilities(caps.NumberInputValueCaps);

  return true;
}

bool GamepadRawInputWin::QueryButtonCapabilities(uint16_t button_count) {
  assert(hid_functions_);
  assert(hid_functions_->IsValid());
  if (button_count > 0) {
    std::unique_ptr<HIDP_BUTTON_CAPS[]> button_caps(
        new HIDP_BUTTON_CAPS[button_count]);
    NTSTATUS status = hid_functions_->HidPGetButtonCaps()(
        HidP_Input, button_caps.get(), &button_count, preparsed_data_);
    assert(HIDP_STATUS_SUCCESS == status);

    for (size_t i = 0; i < button_count; ++i) {
      if (button_caps[i].Range.UsageMin <= kButtonsLengthCap &&
          button_caps[i].UsagePage == kButtonUsagePage) {
        size_t max_index =
            std::min(kButtonsLengthCap,
                     static_cast<size_t>(button_caps[i].Range.UsageMax));
        buttons_length_ = std::max(buttons_length_, max_index);
      }
    }
  }

  return true;
}

bool GamepadRawInputWin::QueryAxisCapabilities(uint16_t axis_count) {
  assert(hid_functions_);
  assert(hid_functions_->IsValid());
  std::unique_ptr<HIDP_VALUE_CAPS[]> axes_caps(new HIDP_VALUE_CAPS[axis_count]);
  hid_functions_->HidPGetValueCaps()(HidP_Input, axes_caps.get(), &axis_count,
                                     preparsed_data_);

  bool mapped_all_axes = true;

  for (size_t i = 0; i < axis_count; i++) {
    size_t axis_index = axes_caps[i].Range.UsageMin - kAxisMinimumUsageNumber;
    if (axis_index < kAxesLengthCap && !axes_[axis_index].active) {
      axes_[axis_index].caps = axes_caps[i];
      axes_[axis_index].value = 0;
      axes_[axis_index].active = true;
      axes_[axis_index].bitmask = GetBitmask(axes_caps[i].BitSize);
      axes_length_ = std::max(axes_length_, axis_index + 1);
    } else {
      mapped_all_axes = false;
    }
  }

  if (!mapped_all_axes) {
    // For axes whose usage puts them outside the standard axesLengthCap range.
    size_t next_index = 0;
    for (size_t i = 0; i < axis_count; i++) {
      size_t usage = axes_caps[i].Range.UsageMin - kAxisMinimumUsageNumber;
      if (usage >= kAxesLengthCap &&
          axes_caps[i].UsagePage <= kGameControlsUsagePage) {
        for (; next_index < kAxesLengthCap; ++next_index) {
          if (!axes_[next_index].active)
            break;
        }
        if (next_index < kAxesLengthCap) {
          axes_[next_index].caps = axes_caps[i];
          axes_[next_index].value = 0;
          axes_[next_index].active = true;
          axes_[next_index].bitmask = GetBitmask(axes_caps[i].BitSize);
          axes_length_ = std::max(axes_length_, next_index + 1);
        }
      }

      if (next_index >= kAxesLengthCap)
        break;
    }
  }

  return true;
}

} // namespace gamepad