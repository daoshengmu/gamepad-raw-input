
#include "hid_dll_functions_win.h"

namespace gamepad {

HidDllFunctionsWin::HidDllFunctionsWin()
{
  // Load module
  HMODULE module = nullptr;
  /*module = ::LoadLibraryExW(L"hid.dll", nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
							LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);*/
  module = ::LoadLibraryW(L"hid.dll");
  if (module) {
	  hid_dll_ = module;
  }
	  
	  //(base::LoadNativeLibrary(base::FilePath(FILE_PATH_LITERAL("hid.dll")),
		//	nullptr));
  if (!!module) {
    hidp_get_caps_ = reinterpret_cast<HidPGetCapsFunc>(
	  GetProcAddress(hid_dll_, "HidP_GetCaps"));
    hidp_get_button_caps_ = reinterpret_cast<HidPGetButtonCapsFunc>(
	  GetProcAddress(hid_dll_, "HidP_GetButtonCaps"));
    hidp_get_value_caps_ = reinterpret_cast<HidPGetValueCapsFunc>(
	  GetProcAddress(hid_dll_, "HidP_GetValueCaps"));
    hidp_get_usages_ex_ = reinterpret_cast<HidPGetUsagesExFunc>(
	  GetProcAddress(hid_dll_,"HidP_GetUsagesEx"));
    hidp_get_usage_value_ = reinterpret_cast<HidPGetUsageValueFunc>(
	  GetProcAddress(hid_dll_, "HidP_GetUsageValue"));
    hidp_get_scaled_usage_value_ = reinterpret_cast<HidPGetScaledUsageValueFunc>(
	  GetProcAddress(hid_dll_, "HidP_GetScaledUsageValue"));
    hidd_get_product_string_ = reinterpret_cast<HidDGetStringFunc>(
	  GetProcAddress(hid_dll_, "HidD_GetProductString"));
  }

  is_valid_ = hidp_get_caps_ != nullptr && hidp_get_button_caps_ != nullptr &&
              hidp_get_value_caps_ != nullptr &&
              hidp_get_usages_ex_ != nullptr &&
              hidp_get_usage_value_ != nullptr &&
              hidp_get_scaled_usage_value_ != nullptr &&
              hidd_get_product_string_ != nullptr;
}

} // namespace gamepad