#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

using namespace std;

const int BUFFER_SIZE = 2048;
HANDLE clientPipe;
mutex cout_mutex;

void ReceiveMessages()
{
    char buffer[BUFFER_SIZE] = {};

    while (true)
    {
        DWORD bytesRead;
        if (ReadFile(clientPipe, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0)
        {
            lock_guard<mutex> guard(cout_mutex);
            cout << "[Server]: " << buffer << endl;
        }
        else
        {
            cerr << "Error reading from server or connection closed." << endl;
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
        if (!WriteFile(clientPipe, input.c_str(), input.size() + 1, &bytesWritten, NULL))
        {
            cerr << "Error writing to server." << endl;
            break;
        }
    }
}

int main()
{
    clientPipe = CreateFile(
        L"\\\\.\\pipe\\PipeServer",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (clientPipe == INVALID_HANDLE_VALUE)
    {
        cerr << "Failed to connect to server. Error: " << GetLastError() << endl;
        return 1;
    }

    cout << "Connected to server." << endl;

    thread receiver(ReceiveMessages);
    thread sender(SendMessages);

    receiver.join();
    sender.join();

    CloseHandle(clientPipe);
    return 0;
}
