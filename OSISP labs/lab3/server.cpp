#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

using namespace std;

const int BUFFER_SIZE = 2048;
HANDLE serverPipe;
mutex cout_mutex;

void ReceiveMessages()
{
    char buffer[BUFFER_SIZE] = {};

    while (true)
    {
        DWORD bytesRead;
        if (ReadFile(serverPipe, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0)
        {
            lock_guard<mutex> guard(cout_mutex);
            cout << "[Client]: " << buffer << endl;
        }
        else
        {
            cerr << "Error reading from client or connection closed." << endl;
            break;
        }
    }
}

void SendMessages()
{
    string input;
    while (true)
    {
        getline(cin, input);
        DWORD bytesWritten;
        if (!WriteFile(serverPipe, input.c_str(), input.size() + 1, &bytesWritten, NULL))
        {
            cerr << "Error writing to client." << endl;
            break;
        }
    }
}

int main()
{
    serverPipe = CreateNamedPipe(
        L"\\\\.\\pipe\\PipeServer",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL);

    if (serverPipe == INVALID_HANDLE_VALUE)
    {
        cerr << "Failed to create named pipe. Error: " << GetLastError() << endl;
        return 1;
    }

    cout << "Waiting for client connection..." << endl;

    if (ConnectNamedPipe(serverPipe, NULL))
    {
        cout << "Client connected." << endl;

        thread receiver(ReceiveMessages);
        thread sender(SendMessages);

        receiver.join();
        sender.join();
    }
    else
    {
        cerr << "Client connection failed." << endl;
    }

    CloseHandle(serverPipe);
    return 0;
}
