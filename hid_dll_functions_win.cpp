
#include "hid_dll_functions_win.h"

namespace gamepad {

HidDllFunctionsWin::HidDllFunctionsWin()
  : hid_dll_(
          base::LoadNativeLibrary(base::FilePath(FILE_PATH_LITERAL("hid.dll")),
                                  nullptr)) {
  if (hid_dll_.is_valid()) {
    hidp_get_caps_ = reinterpret_cast<HidPGetCapsFunc>(
        hid_dll_.GetFunctionPointer("HidP_GetCaps"));
    hidp_get_button_caps_ = reinterpret_cast<HidPGetButtonCapsFunc>(
        hid_dll_.GetFunctionPointer("HidP_GetButtonCaps"));
    hidp_get_value_caps_ = reinterpret_cast<HidPGetValueCapsFunc>(
        hid_dll_.GetFunctionPointer("HidP_GetValueCaps"));
    hidp_get_usages_ex_ = reinterpret_cast<HidPGetUsagesExFunc>(
        hid_dll_.GetFunctionPointer("HidP_GetUsagesEx"));
    hidp_get_usage_value_ = reinterpret_cast<HidPGetUsageValueFunc>(
        hid_dll_.GetFunctionPointer("HidP_GetUsageValue"));
    hidp_get_scaled_usage_value_ =
        reinterpret_cast<HidPGetScaledUsageValueFunc>(
            hid_dll_.GetFunctionPointer("HidP_GetScaledUsageValue"));
    hidd_get_product_string_ = reinterpret_cast<HidDGetStringFunc>(
        hid_dll_.GetFunctionPointer("HidD_GetProductString"));
  }

  is_valid_ = hidp_get_caps_ != nullptr && hidp_get_button_caps_ != nullptr &&
              hidp_get_value_caps_ != nullptr &&
              hidp_get_usages_ex_ != nullptr &&
              hidp_get_usage_value_ != nullptr &&
              hidp_get_scaled_usage_value_ != nullptr &&
              hidd_get_product_string_ != nullptr;
}

} // namespace gamepad