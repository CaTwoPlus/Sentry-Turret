/*
Copyright 2020 - github.com/CaTwoPlus

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

constexpr int iArduinoWaitTime{ 2000 };
constexpr int iMaxDataLength{ 196000000 }; // You may want to customize this value, so data output stays up to date on the console for longer
constexpr int iWriteToArduinoLength{ 32 };
constexpr const char* pPortName{"COM3"};

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cstdlib> // for std::rand() and std::srand()
#include <ctime> // for std::time() 
#include <array>
#include <thread> // for multithreading support

class cSerialPort
{
private:
    HANDLE Handler;
    COMSTAT Status;
    DWORD Errors;
public:
    const char* pWriteToArduino;
    const char* pStopArduinoOutput;
    const char* pIncomingData;

    bool bIsDataSent;
    bool bConnected;

    std::string sFilePath;

    std::array<std::string, 12> TurretDetectionWavFiles;
    std::array<std::string, 6> TurretActivationWavFiles;
    std::array<std::string, 11> TurretSearchingWavFiles;
    std::array<std::string, 16> TurretPowerDownWavFiles;
    std::array<std::string, 3> TurretAttackWavFiles;
    std::array<std::string, 11> TurretMovedWavFiles;
    std::array<std::string, 12> TurretDie;

public:
    explicit cSerialPort(const char* pPortName);
    ~cSerialPort();

    int ReadSerialPort(const char *pBuffer, unsigned int iNbOfChars);
    bool WriteSerialPort(const char *pBuffer, unsigned int iNbOfChars);
    bool IsConnected();
    void AutoConnect(void);
    void ExampleReceiveData(void);
    void PlayAudio(const char* pIncomingData, const int IncomingDataMaxSize, std::array<std::string, 12>& TurretDetectionWavFiles,
                   std::array<std::string, 6>& TurretActivationWavFiles, std::array<std::string, 11>& TurretSearchingWavFiles,
                   std::array<std::string, 16>& TurretPowerDownWavFiles, std::array<std::string, 3>& TurretAttackWavFiles,
                   std::array<std::string, 11>& TurretMovedWavFiles, std::array<std::string, 12>& TurretDie);
};