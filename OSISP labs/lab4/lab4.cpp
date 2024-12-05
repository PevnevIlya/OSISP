#include <iostream>
#include <thread>
#include <Windows.h>
#include <mutex>
#include <vector>

using namespace std;

HANDLE semaphoreHandle;
mutex consoleMutex;

int sharedResource = 0;

void DisplayCurrentTime()
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    printf("[%02d:%02d:%02d] ", systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
}

void ReaderTask(int readerId)
{
    while (true)
    {
        DWORD waitResult = WaitForSingleObject(semaphoreHandle, 0);

        if (waitResult == WAIT_OBJECT_0)
        {
            {
                lock_guard<mutex> lock(consoleMutex);
                DisplayCurrentTime();
                cout << "Reader " << readerId << " accessed the resource." << endl;
            }
            ReleaseSemaphore(semaphoreHandle, 1, NULL);
        }
        else if (waitResult == WAIT_TIMEOUT)
        {
            {
                lock_guard<mutex> lock(consoleMutex);
                DisplayCurrentTime();
                cout << "Reader " << readerId << " could not access the resource." << endl;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

void WriterTask(int writerId)
{
    while (true)
    {
        WaitForSingleObject(semaphoreHandle, INFINITE);

        sharedResource = writerId;

        {
            lock_guard<mutex> lock(consoleMutex);
            DisplayCurrentTime();
            cout << "Writer " << writerId << " updated the resource." << endl;
        }

        ReleaseSemaphore(semaphoreHandle, 1, NULL);

        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

int main()
{
    semaphoreHandle = CreateSemaphore(NULL, 1, 1, NULL);

    if (semaphoreHandle == NULL)
    {
        cerr << "Failed to create semaphore. Error: " << GetLastError() << endl;
        return 1;
    }

    int numberOfReaders, numberOfWriters;

    cout << "Enter the number of readers: ";
    cin >> numberOfReaders;
    cout << "Enter the number of writers: ";
    cin >> numberOfWriters;

    vector<thread> readerThreads;
    vector<thread> writerThreads;

    for (int i = 0; i < numberOfReaders; ++i)
        readerThreads.emplace_back(ReaderTask, i + 1);

    for (int i = 0; i < numberOfWriters; ++i)
        writerThreads.emplace_back(WriterTask, i + 1);

    for (auto& reader : readerThreads)
        reader.join();

    for (auto& writer : writerThreads)
        writer.join();

    CloseHandle(semaphoreHandle);

    return 0;
}
