#include "ContextMenuHandler.h"
#include <iostream>
#include <windows.h>
#include <shlobj.h>

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

class GlobalRefTracker {
    static inline LONG s_activeObjects = 0;

public:
    static void Inc() { InterlockedIncrement(&s_activeObjects); }
    static void Dec() { InterlockedDecrement(&s_activeObjects); }
    static bool CanUnload() { return s_activeObjects == 0; }
};



class ClassFactory : public IClassFactory {
    LONG m_refCount = 1;
    static inline LONG s_serverLocks = 0;  // Глобальный счётчик ссылок на сервер

public:
    ClassFactory() { InterlockedIncrement(&s_serverLocks); }
    virtual ~ClassFactory() { InterlockedDecrement(&s_serverLocks); }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        GlobalRefTracker::Inc();
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override {
        GlobalRefTracker::Dec();
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) delete this;
        return count;
    }


    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override {
        if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;
        auto* handler = new(std::nothrow) ContextMenuHandler();
        if (!handler) return E_OUTOFMEMORY;
        return handler->QueryInterface(riid, ppvObject);
    }

    STDMETHODIMP LockServer(BOOL fLock) override {
        if (fLock) InterlockedIncrement(&s_serverLocks);
        else       InterlockedDecrement(&s_serverLocks);
        return S_OK;
    }

    static bool CanUnload() {
        return s_serverLocks == 0;
    }
};

STDAPI DllCanUnloadNow() {
    return GlobalRefTracker::CanUnload() ? S_OK : S_FALSE;
}


// Конструктор
ContextMenuHandler::ContextMenuHandler() : m_refCount(1) {}

// Деструктор
ContextMenuHandler::~ContextMenuHandler() = default;

STDMETHODIMP ContextMenuHandler::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown) {
        *ppv = static_cast<IUnknown*>(static_cast<IShellExtInit*>(this));
    } else if (riid == IID_IShellExtInit) {
        *ppv = static_cast<IShellExtInit*>(this);
    } else if (riid == IID_IContextMenu) {
        *ppv = static_cast<IContextMenu*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) ContextMenuHandler::AddRef()
{
    GlobalRefTracker::Inc();
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) ContextMenuHandler::Release()
{
    GlobalRefTracker::Dec();
    ULONG refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}


STDMETHODIMP ContextMenuHandler::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    // Получаем файлы, с которыми будем работать
    FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmed;
    if (FAILED(pdtobj->GetData(&format, &stgmed))) {
        return E_INVALIDARG;
    }

    // Загружаем список файлов
    if (char *files = static_cast<char *>(GlobalLock(stgmed.hGlobal))) {
        while (*files) {
            // Преобразуем каждый файл из char* в wchar_t*
            int len = static_cast<int>(strlen(files));
            std::wstring wideStr(len, L'\0');
            MultiByteToWideChar(CP_ACP, 0, files, len, &wideStr[0], len);
            m_filePaths.push_back(wideStr);
            files += len + 1; // Переходим к следующему файлу
        }
        GlobalUnlock(stgmed.hGlobal);
    }
    ReleaseStgMedium(&stgmed);
    return S_OK;
}

STDMETHODIMP ContextMenuHandler::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    MessageBoxW(nullptr, L"Adding custom menu items", L"Debug", MB_OK);

    if (uFlags & CMF_DEFAULTONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

    // Настройка пунктов меню
    MENUITEMINFOW mii = {0};  // Используем MENUITEMINFOW для широких символов
    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_STRING;

    // Первый пункт меню
    mii.wID = idCmdFirst;
    mii.dwTypeData = const_cast<LPWSTR>(L"Custom Action 1");  // Указываем строку для широких символов
    InsertMenuItemW(hmenu, indexMenu, TRUE, &mii);

    // Второй пункт меню
    mii.wID = idCmdFirst+1;
    mii.dwTypeData = const_cast<LPWSTR>(L"Custom Action 2");  // Указываем строку для широких символов
    InsertMenuItemW(hmenu, indexMenu + 1, TRUE, &mii);

    // Возвращаем количество добавленных пунктов
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 2);
}



// Обработчик вызова команды
STDMETHODIMP ContextMenuHandler::InvokeCommand(CMINVOKECOMMANDINFO *pici)
{
    if (HIWORD(pici->lpVerb) == 0) {
        switch (LOWORD(pici->lpVerb)) {
        case 0:
            MessageBoxW(nullptr, L"Custom Action 1 invoked", L"Action", MB_OK);
            break;
        case 1:
            MessageBoxW(nullptr, L"Custom Action 2 invoked", L"Action", MB_OK);
            break;
        default: ;
        }
    }
    return S_OK;
}

// Получение строки команды
STDMETHODIMP ContextMenuHandler::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, char *pszName, UINT cchMax)
{
    if (uFlags & GCS_HELPTEXT) {
        // Преобразуем строку с описанием в wchar_t, так как pszName - это char*
        if (idCmd == 0) {
            // Преобразуем строку из wchar_t в char*
            std::wstring description = L"Custom Action 1 Description";
            if (cchMax > description.size()) {
                wcstombs(pszName, description.c_str(), cchMax);  // Преобразуем в char* с использованием wcstombs
            }
        } else if (idCmd == 1) {
            std::wstring description = L"Custom Action 2 Description";
            if (cchMax > description.size()) {
                wcstombs(pszName, description.c_str(), cchMax);
            }
        }
    }
    return S_OK;
}
HRESULT DllRegisterServer() {
    const wchar_t* clsidStr = L"{8f27e153-676a-4ff5-9e7e-5899c1624f1b}";
    const wchar_t* progID = L"MyShellExtension.ProgID";
    const wchar_t* dllPath = L"C:\\Users\\anton\\Desktop\\CRYPT\\ShellExtensionHandler.dll";

    HKEY hKey;
    LONG res;

    // 1. CLSID
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, (std::wstring(L"CLSID\\") + clsidStr).c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (res != ERROR_SUCCESS) return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)L"My Context Menu Handler", sizeof(L"My Context Menu Handler"));
    RegCloseKey(hKey);

    // 2. InprocServer32
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, (std::wstring(L"CLSID\\") + clsidStr + L"\\InprocServer32").c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (res != ERROR_SUCCESS) return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)dllPath, (DWORD)((wcslen(dllPath) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (BYTE*)L"Apartment", sizeof(L"Apartment"));
    RegCloseKey(hKey);

    // 3. ProgID
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, (std::wstring(L"CLSID\\") + clsidStr + L"\\ProgID").c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (res != ERROR_SUCCESS) return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)progID, (DWORD)((wcslen(progID) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // 4. Привязка к * (всем файлам)
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, L"*\\shellex\\ContextMenuHandlers\\MyShellExtension", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (res != ERROR_SUCCESS) return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)clsidStr, (DWORD)((wcslen(clsidStr) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    return S_OK;
}



HRESULT DllUnregisterServer() {
    SHDeleteKeyW(HKEY_CLASSES_ROOT, L"*\\shellex\\ContextMenuHandlers\\MyShellExtension");
    SHDeleteKeyW(HKEY_CLASSES_ROOT, L"CLSID\\{8f27e153-676a-4ff5-9e7e-5899c1624f1b}");
    return S_OK;
}




