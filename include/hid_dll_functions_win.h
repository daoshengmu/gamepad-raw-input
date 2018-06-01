#ifndef HID_DLL_FUNCTIONS_WIN_H
#define HID_DLL_FUNCTIONS_WIN_H

#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <hidsdi.h>
#include <Hidpi.h>


namespace gamepad {

// Function types we use from hid.dll.
typedef NTSTATUS(__stdcall* HidPGetCapsFunc)(PHIDP_PREPARSED_DATA PreparsedData,
                                             PHIDP_CAPS Capabilities);
typedef NTSTATUS(__stdcall* HidPGetButtonCapsFunc)(
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData);
typedef NTSTATUS(__stdcall* HidPGetValueCapsFunc)(
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PUSHORT ValueCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData);
typedef NTSTATUS(__stdcall* HidPGetUsagesExFunc)(
    HIDP_REPORT_TYPE ReportType,
    USHORT LinkCollection,
    PUSAGE_AND_PAGE ButtonList,
    ULONG* UsageLength,
    PHIDP_PREPARSED_DATA PreparsedData,
    PCHAR Report,
    ULONG ReportLength);
typedef NTSTATUS(__stdcall* HidPGetUsageValueFunc)(
    HIDP_REPORT_TYPE ReportType,
    USAGE UsagePage,
    USHORT LinkCollection,
    USAGE Usage,
    PULONG UsageValue,
    PHIDP_PREPARSED_DATA PreparsedData,
    PCHAR Report,
    ULONG ReportLength);
typedef NTSTATUS(__stdcall* HidPGetScaledUsageValueFunc)(
    HIDP_REPORT_TYPE ReportType,
    USAGE UsagePage,
    USHORT LinkCollection,
    USAGE Usage,
    PLONG UsageValue,
    PHIDP_PREPARSED_DATA PreparsedData,
    PCHAR Report,
    ULONG ReportLength);
typedef BOOLEAN(__stdcall* HidDGetStringFunc)(HANDLE HidDeviceObject,
                                              PVOID Buffer,
                                              ULONG BufferLength);

// Loads hid.dll and provides access to HID methods needed for enumeration and
// polling of RawInput devices on Windows.
class HidDllFunctionsWin {
 public:
  HidDllFunctionsWin();
  ~HidDllFunctionsWin() = default;

  // Return true if the hid.dll functions were successfully loaded.
  bool IsValid() const { return is_valid_; }

  // Getters for each hid.dll function.
  HidPGetCapsFunc HidPGetCaps() const { return hidp_get_caps_; }
  HidPGetButtonCapsFunc HidPGetButtonCaps() const {
    return hidp_get_button_caps_;
  }
  HidPGetValueCapsFunc HidPGetValueCaps() const { return hidp_get_value_caps_; }
  HidPGetUsagesExFunc HidPGetUsagesEx() const { return hidp_get_usages_ex_; }
  HidPGetUsageValueFunc HidPGetUsageValue() const {
    return hidp_get_usage_value_;
  }
  HidPGetScaledUsageValueFunc HidPGetScaledUsageValue() const {
    return hidp_get_scaled_usage_value_;
  }
  HidDGetStringFunc HidDGetProductString() const {
    return hidd_get_product_string_;
  }

 private:
  bool is_valid_;
  HMODULE hid_dll_;
  HidPGetCapsFunc hidp_get_caps_ = nullptr;
  HidPGetButtonCapsFunc hidp_get_button_caps_ = nullptr;
  HidPGetValueCapsFunc hidp_get_value_caps_ = nullptr;
  HidPGetUsagesExFunc hidp_get_usages_ex_ = nullptr;
  HidPGetUsageValueFunc hidp_get_usage_value_ = nullptr;
  HidPGetScaledUsageValueFunc hidp_get_scaled_usage_value_ = nullptr;
  HidDGetStringFunc hidd_get_product_string_ = nullptr;
};

} // namespace gamepad

#endif // HID_DLL_FUNCTIONS_WIN_H