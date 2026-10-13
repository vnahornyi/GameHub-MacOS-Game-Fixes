#include <windows.h>
__declspec(dllexport) const UINT D3D12SDKVersion = 611;
__declspec(dllexport) const char* D3D12SDKPath = ".\\D3D12-REDIST\\";
BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID x){ return TRUE; }
