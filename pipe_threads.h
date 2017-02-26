#ifndef PIPE_THREADS_H_INCLUDED
#define PIPE_THREADS_H_INCLUDED

#define BUFFER_SIZE_IN 256
#define BUFFER_SIZE_OUT 1024

void stdoutThread(LPVOID shell);
void stdinThread(LPVOID shell);

#endif // PIPE_THREADS_H_INCLUDED
