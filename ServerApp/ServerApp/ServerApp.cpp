#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <atlstr.h>

#define BUF_SIZE 256

// Receive memory page
TCHAR recvName[] = TEXT("Global\FileMappingObjectClientSend");
// Send memory page
TCHAR sendName[] = TEXT("Global\FileMappingObjectClientRecv");

bool console_output = true;

// Command processor directly imported from previous project
class CommandProcessor {
public:
    std::vector<std::string> process_cmd(std::vector<std::string>cmd) {
        std::vector<std::string> split_command;
        for (auto c : cmd) {
            std::string s = "";
            for (int j = 0; j < c.size(); j++) {
                if ((c[j] == ';' || c[j] == ',') && s != "" && s != ";") {
                    split_command.push_back(s);
                    s = "";
                }
                else {
                    if (c[j] == ' ') continue;
                    s += c[j];
                }
            }
            if (s != "" && s != ";")
                split_command.push_back(strip(s));
        }
        return split_command;
    }

    void process_special_command(std::vector<std::string> cmd, std::vector<std::string> commands[2]) {
        int cmd_id = 0;

        for (auto c : cmd) {
            std::string s = "";
            for (int j = 0; j < c.size(); j++) {
                if (c[j] == ':') {
                    commands[cmd_id].push_back(s);
                    s = "";
                    cmd_id++;
                    cmd_id %= 2; // just to avoid overflowing
                }
                else {
                    s += c[j];
                }
            }
            if (s != "" && s != ";")
                commands[cmd_id].push_back(strip(s));
        }

    }

    std::string strip(std::string str) {
        std::string s = "";
        int start_idx = 0;
        int end_idx = str.size() - 1;
        for (int i = 0; i < str.size(); i++) {
            if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t') {
                start_idx = i;
                break;
            }
        }
        for (int i = str.size() - 1; i >= 0; i--) {
            if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t') {
                end_idx = i;
                break;
            }
        }
        s = str.substr(start_idx, end_idx - start_idx + 1);
        return s;
    }
};


int main(int argc, char* argv[]) {

    // Variables used for send data to the client
    HANDLE hMapFileSend;
    LPCTSTR pBufSend;

    // Variables used for receive data from the client
    HANDLE hMapFileRecv;
    LPCTSTR pBufRecv;

    // Command processor instant
    CommandProcessor cmd_proc;

    int nRetValue = 0;

    // More details can be found about shared memory on https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory

    // Memory file for send data
    hMapFileSend = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        sendName);                 // name of mapping object

    if (hMapFileSend == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return 1;
    }
    pBufSend = (LPTSTR)MapViewOfFile(hMapFileSend,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
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
    hMapFileRecv = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        BUF_SIZE,                // maximum object size (low-order DWORD)
        recvName);                 // name of mapping object

    if (hMapFileRecv == NULL)
    {
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),
            GetLastError());
        return 1;
    }
    pBufRecv = (LPTSTR)MapViewOfFile(hMapFileRecv,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
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

    std::cout << "Server is ready" << std::endl;

    // Main loop of the program
    while (true)
    {
        std::vector<std::string> params;

        // Waiting for command (Implemented as polling)
        while (pBufRecv[0] == 0)
        {
            // Waits
            Sleep(10); // this interval can be changed according to the user (units : miliseconds)
        }

        // Process the command //
        std::wstring w = pBufRecv;
        std::string str_buffer(w.begin(), w.end());

        std::stringstream ss(str_buffer);
        std::string param;
        while (ss >> param) {
            params.push_back(param);
        }

        if (console_output) {
            std::cout << *pBufRecv << std::endl;
            std::cout << str_buffer << std::endl;
        }

        std::vector<std::string> cmds = cmd_proc.process_cmd(params);

        if (console_output) {
            for (auto str : cmds) {
                std::cout << str << " ";
            }
            std::cout << std::endl;
        }

        int command_number = stoi(cmds[0]);

        switch (command_number) {
        case 0:  // Request for initialisation status

            if (cmds[1] == "REPORT") {
                nRetValue = 0;
                std::wstring report(L"00;REPORT;");
                const wchar_t* wCharCommand = report.c_str();

                // Sends the results to the shared memory
                CopyMemory((PVOID)pBufSend, wCharCommand, sizeof(wchar_t) * report.length());
            }
            else {
                std::wstring report(L"00;INVCMD");
                const wchar_t* wCharCommand = report.c_str();

                // Sends the results to the shared memory
                CopyMemory((PVOID)pBufSend, wCharCommand, sizeof(wchar_t) * report.length());
            }

            break;

        case 1:// command specific code here
            nRetValue = 1;
            break;

        case 2: //  command specific code here
            nRetValue = 2;
            break;

        case 3: // command specific code here
            nRetValue = 3;
            break;
        }

        // clear the receive memory
        FillMemory((PVOID)pBufRecv, BUF_SIZE, 0);
    }


    // Clear the shared memory 
    UnmapViewOfFile(pBufSend);
    UnmapViewOfFile(pBufRecv);

    CloseHandle(hMapFileSend);
    CloseHandle(hMapFileRecv);

    return 0;
}

