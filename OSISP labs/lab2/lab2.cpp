#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

// Constants
const size_t BUFFER_SIZE = 4096; // Initial buffer size

struct AsyncContext {
    HANDLE fileHandle;
    char* buffer;
    DWORD bytesRead;
    OVERLAPPED overlapped;
    double mean, stddev;
    char* outputFile;
};

// Function to calculate statistics
void calculateStatistics(const char* buffer, size_t size, double& mean, double& stddev, char& minVal, char& maxVal) {
    long long sum = 0;
    long long sumSq = 0;
    minVal = CHAR_MAX;
    maxVal = CHAR_MIN;

    for (size_t i = 0; i < size; ++i) {
        char value = buffer[i];
        sum += value;
        sumSq += value * value;
        if (value < minVal) minVal = value;
        if (value > maxVal) maxVal = value;
    }

    mean = static_cast<double>(sum) / size;
    double variance = static_cast<double>(sumSq) / size - mean * mean;
    stddev = std::sqrt(variance);
}

// Asynchronous read completion callback
VOID CALLBACK onAsyncReadComplete(DWORD errorCode, DWORD bytesRead, LPOVERLAPPED overlapped) {
    AsyncContext* context = reinterpret_cast<AsyncContext*>(overlapped->hEvent);
    if (errorCode != 0 || bytesRead == 0) {
        CloseHandle(context->fileHandle);
        delete[] context->buffer;
        delete context;
        return;
    }

    char minVal, maxVal;
    calculateStatistics(context->buffer, bytesRead, context->mean, context->stddev, minVal, maxVal);

    // Save statistics to output file
    std::ofstream outFile(context->outputFile, std::ios::app);
    outFile << "Mean: " << context->mean << ", StdDev: " << context->stddev
        << ", Min: " << static_cast<int>(minVal) << ", Max: " << static_cast<int>(maxVal) << "\n";
    outFile.close();

    delete[] context->buffer;
    delete context;
}

// Asynchronous file processing
void processFileAsync(const char* inputFile, const char* outputFile) {
    HANDLE fileHandle = CreateFile(inputFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file for async read." << std::endl;
        return;
    }

    AsyncContext* context = new AsyncContext;
    context->fileHandle = fileHandle;
    context->buffer = new char[BUFFER_SIZE];
    context->overlapped = { 0 };
    context->overlapped.hEvent = reinterpret_cast<HANDLE>(context);
    context->outputFile = const_cast<char*>(outputFile);

    if (!ReadFileEx(fileHandle, context->buffer, BUFFER_SIZE, &context->overlapped, onAsyncReadComplete)) {
        std::cerr << "ReadFileEx failed." << std::endl;
        CloseHandle(fileHandle);
        delete[] context->buffer;
        delete context;
        return;
    }

    SleepEx(INFINITE, TRUE); // Wait for the async operation to complete
}

// Synchronous file processing
void processFileSync(const char* inputFile, const char* outputFile) {
    HANDLE fileHandle = CreateFile(inputFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file for sync read." << std::endl;
        return;
    }

    char* buffer = new char[BUFFER_SIZE];
    DWORD bytesRead;
    double mean, stddev;
    char minVal, maxVal;

    while (ReadFile(fileHandle, buffer, BUFFER_SIZE, &bytesRead, nullptr) && bytesRead > 0) {
        calculateStatistics(buffer, bytesRead, mean, stddev, minVal, maxVal);

        std::ofstream outFile(outputFile, std::ios::app);
        outFile << "Mean: " << mean << ", StdDev: " << stddev
            << ", Min: " << static_cast<int>(minVal) << ", Max: " << static_cast<int>(maxVal) << "\n";
        outFile.close();
    }

    delete[] buffer;
    CloseHandle(fileHandle);
}

int main() {
    const char* inputFile = "input.txt";
    const char* outputFileAsync = "stats_async.txt";
    const char* outputFileSync = "stats_sync.txt";

    // Clear output files
    std::ofstream(outputFileAsync, std::ios::trunc).close();
    std::ofstream(outputFileSync, std::ios::trunc).close();

    auto start = std::chrono::high_resolution_clock::now();
    processFileAsync(inputFile, outputFileAsync);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Async processing completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    processFileSync(inputFile, outputFileSync);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Sync processing completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms." << std::endl;

    return 0;
}