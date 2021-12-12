#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include < stdlib.h >

#define BUF_SIZE 256

// Receive memory page
TCHAR recvName[] = TEXT("Global\FileMappingObjectClientRecv"); // Use global for acsess the memory page from several terminals
// Send memory page
TCHAR sendName[] = TEXT("Global\FileMappingObjectClientSend");

bool console_output = true;

int main() {

    // Variables used for send data to the client
    HANDLE hMapFileSend;
    LPCTSTR pBufSend;

    // Variables used for receive data from the client
    HANDLE hMapFileRecv;
    LPCTSTR pBufRecv;

    // More details can be found about shared memory on https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory

    // Memory file for send data
    hMapFileSend = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        sendName);               // name of mapping object

    if (hMapFileSend == NULL)
    {
        _tprintf(TEXT("Could not open file mapping object of send (%d).\n"),
            GetLastError());
        return 1;
    }

    pBufSend = (LPTSTR)MapViewOfFile(hMapFileSend, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBufSend == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFileSend);

        return 1;
    }

    // Memory file for receive data
    hMapFileRecv = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        recvName);               // name of mapping object

    if (hMapFileRecv == NULL)
    {
        _tprintf(TEXT("Could not open file mapping object of receive (%d).\n"),
            GetLastError());
        return 1;
    }

    pBufRecv = (LPTSTR)MapViewOfFile(hMapFileRecv, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        BUF_SIZE);

    if (pBufRecv == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
            GetLastError());

        CloseHandle(hMapFileRecv);

        return 1;
    }

    // Main loop of the program
    while (true)
    {
    std::cout << "Enter the command: ";
        // User input
        std::wstring message;
        std::getline(std::wcin, message);

        // String to const char conversion
        const wchar_t* wCharCommand = message.c_str();

        // Send command to shared memory
        CopyMemory((PVOID)pBufSend, wCharCommand, sizeof(wchar_t) * message.length());

        if (console_output) {
            std::wcout << "Sending message: " << message << std::endl;
            std::wcout << "Sending message char: " << wCharCommand << std::endl;
            std::cout << *pBufSend << std::endl;
        }

        // Waiting for reply
        while (pBufRecv[0] == 0)
        {
            // Waits for the reply
            Sleep(10); // this interval can be changed according to the user (units : miliseconds)
        }

        // char to string conversion
        std::wstring w = pBufRecv;
        std::string buffer(w.begin(), w.end());

        // clear the receive memory
        FillMemory((PVOID)pBufRecv, BUF_SIZE, 0);

        std::cout << buffer << std::endl;
    }

    // Clear the shared memory 
    UnmapViewOfFile(pBufSend);
    UnmapViewOfFile(pBufRecv);

    CloseHandle(hMapFileSend);
    CloseHandle(hMapFileRecv);

    return 0;
}
