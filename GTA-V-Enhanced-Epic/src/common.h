#include <windows.h>
#include <stdio.h>
static FILE* lf; static CRITICAL_SECTION cs; static int csinit;
static void plog(const char* fmt, ...){ (void)fmt; }
static const char* g(const GUID* x){ static char b[8][64]; static int i; char* s=b[(i++)&7]; if(!x) return "null";
  sprintf(s,"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",x->Data1,x->Data2,x->Data3,x->Data4[0],x->Data4[1],x->Data4[2],x->Data4[3],x->Data4[4],x->Data4[5],x->Data4[6],x->Data4[7]); return s; }
/* Finds GameHub's GPTK d3d12.dll: WINEDLLPATH (GameHub sets it to .../gptk-4.0-2/wine),
   then WINE_GPTK_LIBD3DSHARED_PATH (.../gptk-4.0-2/external/libd3dshared.dylib). */
static HMODULE tryunix(const wchar_t* dir, const wchar_t* n){ wchar_t p[2048]; swprintf(p,2048,L"Z:%ls/x86_64-windows/%ls",dir,n); for(wchar_t* q=p;*q;q++) if(*q==L'/') *q=L'\\'; return LoadLibraryW(p); }
static HMODULE loadreal(const wchar_t* n){ wchar_t v[4096]; HMODULE h=NULL;
  if(GetEnvironmentVariableW(L"WINEDLLPATH",v,4096)){ wchar_t* ctx=NULL; for(wchar_t* t=wcstok(v,L":",&ctx); t && !h; t=wcstok(NULL,L":",&ctx)) if(wcsstr(t,L"gptk")) h=tryunix(t,n); }
  if(!h && GetEnvironmentVariableW(L"WINE_GPTK_LIBD3DSHARED_PATH",v,4096)){ wchar_t* s=wcsstr(v,L"/external/"); if(s){ wcscpy(s,L"/wine"); h=tryunix(v,n);} }
  return h; }
typedef void* (*GEN)(void*,void*,void*,void*,void*,void*);
