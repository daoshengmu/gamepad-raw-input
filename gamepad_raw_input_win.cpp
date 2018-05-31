
#include "gamepad_raw_input_win.h"

namespace gamepad {

GamepadRawInputWin::GamepadRawInputWin() {
  hid_functions_ = std::make_shared<HidDllFunctionsWin>();
}

bool GamepadRawInputWin::QueryHidInfo() {
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(0u, result);

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, buffer.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(size, result);
  RID_DEVICE_INFO* device_info =
      reinterpret_cast<RID_DEVICE_INFO*>(buffer.get());

  DCHECK_EQ(device_info->dwType, static_cast<DWORD>(RIM_TYPEHID));
  vendor_id_ = device_info->hid.dwVendorId;
  product_id_ = device_info->hid.dwProductId;
  version_number_ = device_info->hid.dwVersionNumber;
  usage_ = device_info->hid.usUsage;

  return true;
}

bool GamepadRawInputWin::QueryDeviceCapabilities() {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(0u, result);

  ppd_buffer_.reset(new uint8_t[size]);
  preparsed_data_ = reinterpret_cast<PHIDP_PREPARSED_DATA>(ppd_buffer_.get());
  result = ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA,
                                   ppd_buffer_.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(size, result);

  HIDP_CAPS caps;
  NTSTATUS status = hid_functions_->HidPGetCaps()(preparsed_data_, &caps);
  DCHECK_EQ(HIDP_STATUS_SUCCESS, status);

  QueryButtonCapabilities(caps.NumberInputButtonCaps);
  QueryAxisCapabilities(caps.NumberInputValueCaps);

  return true;
}

bool GamepadRawInputWin::QueryButtonCapabilities() {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  if (button_count > 0) {
    std::unique_ptr<HIDP_BUTTON_CAPS[]> button_caps(
        new HIDP_BUTTON_CAPS[button_count]);
    NTSTATUS status = hid_functions_->HidPGetButtonCaps()(
        HidP_Input, button_caps.get(), &button_count, preparsed_data_);
    DCHECK_EQ(HIDP_STATUS_SUCCESS, status);

    for (size_t i = 0; i < button_count; ++i) {
      if (button_caps[i].Range.UsageMin <= Gamepad::kButtonsLengthCap &&
          button_caps[i].UsagePage == kButtonUsagePage) {
        size_t max_index =
            std::min(Gamepad::kButtonsLengthCap,
                     static_cast<size_t>(button_caps[i].Range.UsageMax));
        buttons_length_ = std::max(buttons_length_, max_index);
      }
    }
  }

  return true;
}

bool GamepadRawInputWin::QueryAxisCapabilities() {
   DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  std::unique_ptr<HIDP_VALUE_CAPS[]> axes_caps(new HIDP_VALUE_CAPS[axis_count]);
  hid_functions_->HidPGetValueCaps()(HidP_Input, axes_caps.get(), &axis_count,
                                     preparsed_data_);

  bool mapped_all_axes = true;

  for (size_t i = 0; i < axis_count; i++) {
    size_t axis_index = axes_caps[i].Range.UsageMin - kAxisMinimumUsageNumber;
    if (axis_index < Gamepad::kAxesLengthCap && !axes_[axis_index].active) {
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
      if (usage >= Gamepad::kAxesLengthCap &&
          axes_caps[i].UsagePage <= kGameControlsUsagePage) {
        for (; next_index < Gamepad::kAxesLengthCap; ++next_index) {
          if (!axes_[next_index].active)
            break;
        }
        if (next_index < Gamepad::kAxesLengthCap) {
          axes_[next_index].caps = axes_caps[i];
          axes_[next_index].value = 0;
          axes_[next_index].active = true;
          axes_[next_index].bitmask = GetBitmask(axes_caps[i].BitSize);
          axes_length_ = std::max(axes_length_, next_index + 1);
        }
      }

      if (next_index >= Gamepad::kAxesLengthCap)
        break;
    }
  }

  return true;
}

} // namespace gamepad