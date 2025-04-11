#pragma once
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <objbase.h>

// GUID для CLSID вашего контекстного меню
DEFINE_GUID(CLSID_ContextMenuHandler,
    0x8f27e153, 0x676a, 0x4ff5, 0x9e, 0x7e, 0x58, 0x99, 0xc1, 0x62, 0x4f, 0x1b);

// GUID для интерфейсов (IShellExtInit и IContextMenu) - уже определены в Windows API


// Класс для обработки контекстного меню
class ContextMenuHandler : public IContextMenu, public IShellExtInit
{
public:
    ContextMenuHandler();
    virtual ~ContextMenuHandler();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO *pici) override;
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, char *pszName, UINT cchMax) override;

private:
    ULONG m_refCount;
    std::vector<std::wstring> m_filePaths;
};

// Прототипы для функций регистрации и отмены регистрации
extern "C" __declspec(dllexport) HRESULT __stdcall DllRegisterServer();
extern "C" __declspec(dllexport) HRESULT __stdcall DllUnregisterServer();

