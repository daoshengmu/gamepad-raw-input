
#include <iostream>
#include <cassert>
#include <algorithm>
#include <memory>
#include "gamepad_raw_input_win.h"

#undef min
#undef max

namespace gamepad {

const uint32_t kGenericDesktopUsagePage = 0x01;
const uint32_t kGameControlsUsagePage = 0x05;
const uint32_t kButtonUsagePage = 0x09;
const uint32_t kConsumerUsagePage = 0x0c;

const uint32_t kAxisMinimumUsageNumber = 0x30;
const uint32_t kSystemMainMenuUsageNumber = 0x85;
const uint32_t kPowerUsageNumber = 0x30;
const uint32_t kSearchUsageNumber = 0x0221;
const uint32_t kHomeUsageNumber = 0x0223;
const uint32_t kBackUsageNumber = 0x0224;

const uint32_t kIdLengthCap = 128;

const uint32_t kVendorSony = 0x54c;
const uint32_t kProductDualshock4 = 0x05c4;
const uint32_t kProductDualshock4Slim = 0x9cc;


// The fetcher will collect all HID usages from the Button usage page and any
// additional usages listed below.
struct SpecialUsages {
  const uint16_t usage_page;
  const uint16_t usage;
} kSpecialUsages[] = {
  // Xbox One S pre-FW update reports Xbox button as SystemMainMenu over BT.
  {kGenericDesktopUsagePage, kSystemMainMenuUsageNumber},
  // Power is used for the Guide button on the Nvidia Shield 2015 gamepad.
  {kConsumerUsagePage, kPowerUsageNumber},
  // Search is used for the Guide button on the Nvidia Shield 2017 gamepad.
  {kConsumerUsagePage, kSearchUsageNumber},
  // Start, Back, and Guide buttons are often reported as Consumer Home or
  // Back.
  {kConsumerUsagePage, kHomeUsageNumber},
  {kConsumerUsagePage, kBackUsageNumber},
};
const size_t kSpecialUsagesLen = sizeof(kSpecialUsages);

enum ControllerType {
  UNKNOWN_CONTROLLER,
  DUALSHOCK4_CONTROLLER,
  DUALSHOCK4_SLIM_CONTROLLER
};

float NormalizeAxis(long value, long min, long max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

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

  // result is the num of characters, size is the num. of bytes.
  assert(size == result * 2);

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
    assert(false && "GetRawInputDeviceInfo() failed");
    return false;
  }
  assert(0u == result);

  ppd_buffer_.reset(new uint8_t[size]);
  preparsed_data_ = reinterpret_cast<PHIDP_PREPARSED_DATA>(ppd_buffer_.get());
  result = ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA,
                                   ppd_buffer_.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    assert(false && "GetRawInputDeviceInfo() failed");
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
     /* USAGE uu = axes_caps[i].Range.UsageMin;
      USAGE page = axes_caps[i].UsagePage;*/
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

void GamepadRawInputWin::UpdateGamepad(RAWINPUT* input) {
  assert(hid_functions_->IsValid());
  NTSTATUS status;

  // Query button state.
  if (buttons_length_ > 0) {
    // Clear the button state
    ZeroMemory(buttons_, sizeof(buttons_));
    ULONG buttons_length = 0;

    hid_functions_->HidPGetUsagesEx() (
      HidP_Input, 0, nullptr, &buttons_length, preparsed_data_,
      reinterpret_cast<PCHAR>(input->data.hid.bRawData),
        input->data.hid.dwSizeHid);
    
    /*std:unique_ptr<USAGE_AND_PAGE[]> usages(new USAGE_AND_PAGE[button_length]);*/

    std::unique_ptr<USAGE_AND_PAGE[]> usages(new USAGE_AND_PAGE[buttons_length]);
    status = hid_functions_->HidPGetUsagesEx() (HidP_Input, 0, usages.get(), &buttons_length,
      preparsed_data_, reinterpret_cast<PCHAR>(input->data.hid.bRawData), input->data.hid.dwSizeHid);

    if (status == HIDP_STATUS_SUCCESS) {
      // Set each reprted button to true.
      for (size_t j = 0; j < buttons_length; j++) {
        uint16_t usage_page = usages[j].UsagePage;
        uint16_t usage = usages[j].Usage;
        if (usage_page == kButtonUsagePage && usage > 0) {
          size_t button_index = size_t(usage - 1);
          if (button_index < kButtonsLengthCap) {
            buttons_[button_index] = true;
          } else if (usage_page != kButtonUsagePage &&
            !special_button_map_.empty()) {
            for (size_t special_index = 0; special_index < kSpecialUsagesLen;
              ++special_index) {
              int button_index = special_button_map_[special_index];
              if (button_index < 0)
                continue;
              const auto& special = kSpecialUsages[special_index];
              if (usage_page == special.usage_page && usage == special.usage)
                buttons_[button_index] = true;
            }
          }
        }
      }
    }

    // Query axis state
    ULONG axis_value = 0;

    LONG scaled_axis_value = 0;
    for (uint32_t i = 0; i < axes_length_; i++) {
      RawGamepadAxis* axis = &axes_[i];

      // If the min is < 0 we have to query the scaled value, otherwise we need
      // the normal unscaled value.
      if (axis->caps.LogicalMin < 0) {
        status = hid_functions_->HidPGetScaledUsageValue()(
          HidP_Input, axis->caps.UsagePage, 0, axis->caps.Range.UsageMin,
          &scaled_axis_value, preparsed_data_,
          reinterpret_cast<PCHAR>(input->data.hid.bRawData),
          input->data.hid.dwSizeHid);
        if (status == HIDP_STATUS_SUCCESS) {
          axis->value = NormalizeAxis(scaled_axis_value, axis->caps.PhysicalMin,
            axis->caps.PhysicalMax);
        }
      }
      else {
        status = hid_functions_->HidPGetUsageValue()(
          HidP_Input, axis->caps.UsagePage, 0, axis->caps.Range.UsageMin,
          &axis_value, preparsed_data_,
          reinterpret_cast<PCHAR>(input->data.hid.bRawData),
          input->data.hid.dwSizeHid);
        if (status == HIDP_STATUS_SUCCESS) {
          axis->value = NormalizeAxis(axis_value & axis->bitmask,
            axis->caps.LogicalMin & axis->bitmask,
            axis->caps.LogicalMax & axis->bitmask);
        }
      }
    }

  //  last_update_timestamp_ = GamepadDataFetcher::CurrentTimeInMicroseconds();

  }
}


// These values are logged to UMA. Entries should not be renumbered and
// numeric values should never be reused. Please keep in sync with
// "GamepadSource" in src/tools/metrics/histograms/enums.xml.
enum GamepadSource {
  GAMEPAD_SOURCE_NONE = 0,
  GAMEPAD_SOURCE_ANDROID,
  GAMEPAD_SOURCE_GVR,
  GAMEPAD_SOURCE_CARDBOARD,
  GAMEPAD_SOURCE_LINUX_UDEV,
  GAMEPAD_SOURCE_MAC_GC,
  GAMEPAD_SOURCE_MAC_HID,
  GAMEPAD_SOURCE_MAC_XBOX,
  GAMEPAD_SOURCE_NINTENDO,
  GAMEPAD_SOURCE_OCULUS,
  GAMEPAD_SOURCE_OPENVR,
  GAMEPAD_SOURCE_TEST,
  GAMEPAD_SOURCE_WIN_XINPUT,
  GAMEPAD_SOURCE_WIN_RAW,
  kMaxValue = GAMEPAD_SOURCE_WIN_RAW,
};

class GamepadButton {
public:
  // Matches XInput's trigger deadzone.
  static constexpr float kDefaultButtonPressedThreshold = 30.f / 255.f;

  GamepadButton() : pressed(false), touched(false), value(0.) {}
  GamepadButton(bool pressed, bool touched, double value)
    : pressed(pressed), touched(touched), value(value) {}
  bool pressed;
  bool touched;
  double value;
};

class Gamepad {
public:
  //static constexpr size_t kIdLengthCap = 128;
  //static constexpr size_t kMappingLengthCap = 16;
  //static constexpr size_t kAxesLengthCap = 16;
  //static constexpr size_t kButtonsLengthCap = 32;

  Gamepad();
  Gamepad(const Gamepad& other);

  // Is there a gamepad connected at this index?
  bool connected;

  // Device identifier (based on manufacturer, model, etc.).
  unsigned short id[kIdLengthCap];

  // Time value representing the last time the data for this gamepad was
  // updated. Measured as TimeTicks::Now().since_origin().InMicroseconds().
  int64_t timestamp;

  // Number of valid entries in the axes array.
  unsigned axes_length;

  // Normalized values representing axes, in the range [-1..1].
  double axes[GamepadRawInputWin::kAxesLengthCap];

  // Number of valid entries in the buttons array.
  unsigned buttons_length;

  // Button states
  GamepadButton buttons[GamepadRawInputWin::kButtonsLengthCap];

 // GamepadHapticActuator vibration_actuator;

  // Mapping type (for example "standard")
  unsigned short mapping[GamepadRawInputWin::kMappingLengthCap];

  //GamepadPose pose;

 // GamepadHand hand;

  unsigned display_id;

  bool is_xr = false;
};

Gamepad::Gamepad()
  : connected(false),
  timestamp(0),
  axes_length(0),
  buttons_length(0),
  display_id(0) {
  id[0] = 0;
  mapping[0] = 0;
}

struct PadState {
  // Which data fetcher provided this gamepad's data.
  GamepadSource source;
  // Data fetcher-specific identifier for this gamepad.
  int source_id;

  // Indicates whether this gamepad is actively receiving input. |is_active| is
  // initialized to false on each polling cycle and must is set to true when new
  // data is received.
  bool is_active;

  // True if the gamepad is newly connected but notifications have not yet been
  // sent.
  bool is_newly_active;

  // Set by the data fetcher to indicate that one-time initialization for this
  // gamepad has been completed.
  bool is_initialized;

  // Gamepad data, unmapped.
  Gamepad data;

  // Functions to map from device data to standard layout, if available. May
  // be null if no mapping is available or needed.
//  GamepadStandardMappingFunction mapper;

  // Sanitization masks
  // axis_mask and button_mask are bitfields that represent the reset state of
  // each input. If a button or axis has ever reported 0 in the past the
  // corresponding bit will be set to 1.

  // If we ever increase the max axis count this will need to be updated.
  static_assert(GamepadRawInputWin::kAxesLengthCap <=
    std::numeric_limits<uint32_t>::digits,
    "axis_mask is not large enough");
  uint32_t axis_mask;

  // If we ever increase the max button count this will need to be updated.
  static_assert(GamepadRawInputWin::kButtonsLengthCap <=
    std::numeric_limits<uint32_t>::digits,
    "button_mask is not large enough");
  uint32_t button_mask;
};

void GamepadRawInputWin::ReadPadState() {
  //assert(pad);
  std::shared_ptr<Gamepad> pad(new Gamepad());

  // pad->timestamp = last_update_timestamp_;
  pad->buttons_length = buttons_length_;
  pad->axes_length = axes_length_;

  for (unsigned int i = 0; i < buttons_length_; i++) {
    pad->buttons[i].pressed = buttons_[i];
    pad->buttons[i].value = buttons_[i] ? 1.0 : 0.0;
  }

  for (unsigned int i = 0; i < axes_length_; i++) {
    if (axes_[i].active) {
      pad->axes[i] = axes_[i].value;
    }
  }
}

} // namespace gamepad