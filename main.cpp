//
// Created by wention on 2023/2/17.
//

#include <windows.h>

#include <strsafe.h>
#include <iostream>

#define VARIABLE_ATTRIBUTE_NON_VOLATILE 0x00000001
#define VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS 0x00000002
#define VARIABLE_ATTRIBUTE_RUNTIME_ACCESS 0x00000004

#define EFI_GLOBAL_VAR_GUID		L"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}"

void efi_print_error(const wchar_t* message) {
    DWORD err = GetLastError();

    wchar_t* errorString = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPWSTR) &errorString, 0, nullptr);
    wprintf(L"%s %d: %s\n", message, err, errorString);
    LocalFree(errorString);
}

int efi_has_privilege() {
    /* get the privileges necessary */
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return -1;
    }

    TOKEN_PRIVILEGES tkp;
    LookupPrivilegeValue(nullptr, SE_SYSTEM_ENVIRONMENT_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, 0);
    if (GetLastError() != ERROR_SUCCESS) {
        return -2;
    }

    return 0;
}

#define EFI_ERR_INVALID -1
#define EFI_ERR_PRIVILEGE -2
#define EFI_ERR_NOT_SUPPORT -3
#define EFI_ERR_NOT_FOUND -4

#define EFIVAR_BUFFER_SIZE 4096

static uint8_t efivar_buf[EFIVAR_BUFFER_SIZE] = {0};

int efi_read_var(const wchar_t* name, const wchar_t* uuid, void* buf, int bufsize) {
    size_t len = GetFirmwareEnvironmentVariableW(name, uuid, buf, bufsize);

    if (len == 0 && GetLastError() == ERROR_NOACCESS) {
        return EFI_ERR_NOT_FOUND;
    }

    return len;
}

int efi_write_var(const wchar_t* name, const wchar_t* uuid, void* buf, int bufsize, DWORD attrs) {
    return SetFirmwareEnvironmentVariableExW(name, uuid, buf, bufsize, attrs);
}

bool efi_variables_supported() {
    FIRMWARE_TYPE fType;
    BOOL status = GetFirmwareType(&fType);
    return status && fType == FirmwareTypeUefi;
}

int read_u16(const wchar_t* name) {

    size_t length = efi_read_var(name, EFI_GLOBAL_VAR_GUID, efivar_buf, sizeof(efivar_buf));

    if (length < 2) {
        return -1;
    }

    return *((uint16_t *)efivar_buf);
}

int set_u16(const wchar_t* name, uint16_t num) {
    int ret = efi_write_var(name, EFI_GLOBAL_VAR_GUID, (uint8_t *)&num, sizeof(num),
                            VARIABLE_ATTRIBUTE_NON_VOLATILE | VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS | VARIABLE_ATTRIBUTE_RUNTIME_ACCESS
                            );
    return ret;
}

typedef struct efi_load_option {
    uint32_t attributes;
    uint16_t file_path_list_length;
    uint16_t description[];
    // uint8_t file_path_list[];
    // uint8_t optional_data[];
} efi_load_option;


int print_efi_bootnext() {
    const wchar_t* name = L"BootNext";
    int num = read_u16(name);
    if (num >= 0)
        wprintf(L"%s: %04X\n", name, num);
    return 0;
}

int print_efi_bootcurrent() {
    const wchar_t* name = L"BootCurrent";
    int num = read_u16(name);
    if (num >= 0)
        wprintf(L"%s: %04X\n", name, num);

    return 0;
}

int print_efi_bootorder() {
    int length;
    const wchar_t* name = L"BootOrder";
    uint16_t* bootnum = (uint16_t*)efivar_buf;
    if ((length = efi_read_var(name, EFI_GLOBAL_VAR_GUID, efivar_buf, sizeof(efivar_buf))) > 0) {
        wprintf(L"%s: ", name);
        for (int i=0;i<length/sizeof(uint16_t);i++) {
            wprintf(i == 0 ? L"%04X" : L", %04X", bootnum[i]);
        }
        wprintf(L"\n");
    }

    return length;
}

int print_efi_loadoption(uint16_t index) {
    int length;
    wchar_t name[9] = {0};
    struct efi_load_option* load_option = (struct efi_load_option*)efivar_buf;

    swprintf(name, sizeof(name), L"Boot%04X", index);
    if ((length = efi_read_var(name, EFI_GLOBAL_VAR_GUID, efivar_buf, sizeof(efivar_buf))) > 0) {
        wprintf(L"%s: %s\n", name, load_option->description);
    }

    return length;
}

void show_efiboot_vars() {
    print_efi_bootnext();
    print_efi_bootcurrent();
    print_efi_bootorder();
    for (int i=0;i<10;i++) {
        print_efi_loadoption(i);
    }
}

void print_usage() {

}

int wmain(int argc, wchar_t* argv[])
{
    int ret = 0;

    if (efi_has_privilege() != 0) {
        efi_print_error(L"efi privilege required");
        return EFI_ERR_PRIVILEGE;
    }

    if (!efi_variables_supported()) {
        printf("efivar not supported");
        return EFI_ERR_NOT_SUPPORT;
    }

    //wchar_t* var_name = argv[1];
    //wchar_t* var_guid = argv[2];

    if (argc < 2) {
        show_efiboot_vars();
    } else {
        if (wcscmp(argv[1], L"-n") == 0) {
            int bootnext = wcstoul(argv[2], nullptr, 16);
            ret = set_u16(L"BootNext", bootnext & 0xFFFF);
            if (ret == ERROR_SUCCESS) {
                efi_print_error(L"set BootNext failed\n");
                return -1;
            }
        }
    }

    return 0;
}
