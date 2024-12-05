#include <windows.h>
#include <iostream>
#include <chrono>

const int M = 512;  
int N;              

int matrixA[M][M], matrixB[M][M], result[M][M];

LPVOID* fibers;
LPVOID mainFiber;
bool* fiberCompleted;

double GetCPULoad() {
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;

    ULONGLONG totalSystemTime = (kernel.QuadPart + user.QuadPart);
    ULONGLONG totalIdleTime = idle.QuadPart;

    return 100.0 * (1.0 - (double)totalIdleTime / (double)totalSystemTime);
}

void LinearMultiply() {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            result[i][j] = 0;
            for (int k = 0; k < M; k++) {
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }
}

DWORD WINAPI ThreadMultiply(void* param) {
    int threadId = *(int*)param;
    int startRow = (M / N) * threadId;
    int endRow = (M / N) * (threadId + 1);

    for (int i = startRow; i < endRow; i++) {
        for (int j = 0; j < M; j++) {
            result[i][j] = 0;
            for (int k = 0; k < M; k++) {
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
            Sleep(0); 
        }
    }

    return 0;
}

void WINAPI FiberMultiply(void* param) {
    int fiberId = *(int*)param;
    int startRow = (M / N) * fiberId;
    int endRow = (M / N) * (fiberId + 1);

    for (int i = startRow; i < endRow; i++) {
        for (int j = 0; j < M; j++) {
            result[i][j] = 0;
            for (int k = 0; k < M; k++) {
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
            SwitchToFiber(mainFiber);
        }
    }

    fiberCompleted[fiberId] = true;

    while (true) {
        SwitchToFiber(mainFiber);
    }
}

int main() {
    std::cout << "Enter number of threads (fibers): ";
    std::cin >> N;

    if (N <= 0 || N > M) {
        std::cerr << "Invalid number of threads. Exiting." << std::endl;
        return -1;
    }

    fibers = new LPVOID[N];
    fiberCompleted = new bool[N] { false };  

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            matrixA[i][j] = rand() % 10;
            matrixB[i][j] = rand() % 10;
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    double cpuLoadBefore = GetCPULoad();
    LinearMultiply();
    double cpuLoadAfter = GetCPULoad();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> linearTime = end - start;
    std::cout << "Linear multiplication: " << linearTime.count() << " seconds" << std::endl;
    std::cout << "CPU load during linear multiplication: " << cpuLoadBefore << "% before, " << cpuLoadAfter << "% after" << std::endl;

    HANDLE* threads = new HANDLE[N];
    int* threadIds = new int[N];
    start = std::chrono::high_resolution_clock::now();
    cpuLoadBefore = GetCPULoad();

    for (int i = 0; i < N; i++) {
        threadIds[i] = i;
        threads[i] = CreateThread(NULL, 0, ThreadMultiply, &threadIds[i], 0, NULL);
    }

    WaitForMultipleObjects(N, threads, TRUE, INFINITE);
    end = std::chrono::high_resolution_clock::now();
    cpuLoadAfter = GetCPULoad();
    std::chrono::duration<double> threadTime = end - start;
    std::cout << "Multithreaded multiplication: " << threadTime.count() << " seconds" << std::endl;
    std::cout << "CPU load during multithreaded multiplication: " << cpuLoadBefore << "% before, " << cpuLoadAfter << "% after" << std::endl;

    for (int i = 0; i < N; i++) {
        CloseHandle(threads[i]);
    }

    start = std::chrono::high_resolution_clock::now();
    cpuLoadBefore = GetCPULoad();

    mainFiber = ConvertThreadToFiber(NULL); 

    int* fiberIds = new int[N];
    for (int i = 0; i < N; i++) {
        fiberIds[i] = i;
        fibers[i] = CreateFiber(0, FiberMultiply, &fiberIds[i]);
    }

    int currentFiber = 0;
    while (true) {
        if (!fiberCompleted[currentFiber]) {
            SwitchToFiber(fibers[currentFiber]);
        }

        bool allCompleted = true;
        for (int i = 0; i < N; i++) {
            if (!fiberCompleted[i]) {
                allCompleted = false;
                break;
            }
        }
        if (allCompleted) {
            break;
        }

        currentFiber = (currentFiber + 1) % N;
    }

    end = std::chrono::high_resolution_clock::now();
    cpuLoadAfter = GetCPULoad();
    std::chrono::duration<double> fiberTime = end - start;
    std::cout << "Fiber-based multiplication: " << fiberTime.count() << " seconds" << std::endl;
    std::cout << "CPU load during fiber-based multiplication: " << cpuLoadBefore << "% before, " << cpuLoadAfter << "% after" << std::endl;

    for (int i = 0; i < N; i++) {
        DeleteFiber(fibers[i]);
    }

    delete[] fibers;
    delete[] threads;
    delete[] threadIds;
    delete[] fiberIds;
    delete[] fiberCompleted;

    return 0;
}
