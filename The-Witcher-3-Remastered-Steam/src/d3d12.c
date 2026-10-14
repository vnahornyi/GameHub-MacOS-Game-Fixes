/* d3d12.dll wrapper for witcher3.exe (DX12) under GameHub + GPTK.
   Forwards every export to the GPTK d3d12.dll and makes the device reject graphics pipelines with
   stream output, which crash D3DMetal's shader converter (libmetalirconverter). */
#define COBJMACROS
#include <windows.h>
#include <d3d12.h>
#include <wchar.h>

/* Finds GameHub's GPTK d3d12.dll: WINEDLLPATH (GameHub sets it to .../gptk-4.0-2/wine),
   then WINE_GPTK_LIBD3DSHARED_PATH (.../gptk-4.0-2/external/libd3dshared.dylib). */
static HMODULE tryunix(const wchar_t *dir, const wchar_t *n)
{
  wchar_t p[2048];
  swprintf(p, 2048, L"Z:%ls/x86_64-windows/%ls", dir, n);
  for (wchar_t *q = p; *q; q++)
    if (*q == L'/') *q = L'\\';
  return LoadLibraryW(p);
}

static HMODULE loadreal(const wchar_t *n)
{
  wchar_t v[4096];
  HMODULE h = NULL;
  if (GetEnvironmentVariableW(L"WINEDLLPATH", v, 4096)) {
    wchar_t *ctx = NULL;
    for (wchar_t *t = wcstok(v, L":", &ctx); t && !h; t = wcstok(NULL, L":", &ctx))
      if (wcsstr(t, L"gptk")) h = tryunix(t, n);
  }
  if (!h && GetEnvironmentVariableW(L"WINE_GPTK_LIBD3DSHARED_PATH", v, 4096)) {
    wchar_t *s = wcsstr(v, L"/external/");
    if (s) { wcscpy(s, L"/wine"); h = tryunix(v, n); }
  }
  return h;
}

static HMODULE R;
static FARPROC P(const char *n)
{
  if (!R) R = loadreal(L"d3d12.dll");
  return R ? GetProcAddress(R, n) : NULL;
}

/* Exported under their real names through d3d12.def; the fwd_ prefix avoids clashing with d3d12.h. */
typedef void *(*GEN)(void *, void *, void *, void *, void *, void *);
#define FWD(name) void *fwd_##name(void *a, void *b, void *c, void *d, void *e, void *f) \
  { GEN p = (GEN)P(#name); return p ? p(a, b, c, d, e, f) : (void *)E_NOTIMPL; }
FWD(GetBehaviorValue)
FWD(D3D12GetDebugInterface)
FWD(D3D12CoreCreateLayeredDevice)
FWD(D3D12CoreGetLayeredDeviceSize)
FWD(D3D12CoreRegisterLayers)
FWD(D3D12CreateRootSignatureDeserializer)
FWD(D3D12CreateVersionedRootSignatureDeserializer)
FWD(D3D12EnableExperimentalFeatures)
FWD(D3D12SerializeRootSignature)
FWD(D3D12SerializeVersionedRootSignature)

typedef HRESULT (STDMETHODCALLTYPE *PGFX)(ID3D12Device *, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *, REFIID, void **);
static PGFX ogfx;
static CRITICAL_SECTION lk;

static HRESULT STDMETHODCALLTYPE hgfx(ID3D12Device *d, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *s, REFIID r, void **o)
{
  if (s && s->StreamOutput.NumEntries) {
    if (o) *o = NULL;
    return E_INVALIDARG;
  }
  return ogfx(d, s, r, o);
}

/* ID3D12Device::CreateGraphicsPipelineState is vtable slot 10. The game creates devices more than
   once; patch under a lock and only once, so the hook never saves itself as the original. */
static void hookdev(ID3D12Device *dev)
{
  void **slot = &(*(void ***)dev)[10];
  EnterCriticalSection(&lk);
  if (*slot != (void *)hgfx && !ogfx) {
    DWORD old;
    ogfx = (PGFX)*slot;
    VirtualProtect(slot, sizeof *slot, PAGE_EXECUTE_READWRITE, &old);
    *slot = (void *)hgfx;
    VirtualProtect(slot, sizeof *slot, old, &old);
  }
  LeaveCriticalSection(&lk);
}

typedef HRESULT (WINAPI *PCD)(IUnknown *, D3D_FEATURE_LEVEL, REFIID, void **);
HRESULT WINAPI fwd_D3D12CreateDevice(IUnknown *a, D3D_FEATURE_LEVEL fl, REFIID r, void **o)
{
  PCD p = (PCD)P("D3D12CreateDevice");
  if (!p) return E_NOTIMPL;
  HRESULT hr = p(a, fl, r, o);
  if (hr == S_OK && o && *o) {
    ID3D12Device *dev = NULL;
    if (SUCCEEDED(IUnknown_QueryInterface((IUnknown *)*o, &IID_ID3D12Device, (void **)&dev))) {
      hookdev(dev);
      ID3D12Device_Release(dev);
    }
  }
  return hr;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD why, LPVOID r)
{
  if (why == DLL_PROCESS_ATTACH) InitializeCriticalSection(&lk);
  return TRUE;
}
