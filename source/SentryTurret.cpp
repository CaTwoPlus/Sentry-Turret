#include <SentryTurret.hpp>
#pragma comment(lib, "winmm.lib")

// Create class object with global scope
cSerialPort* Arduino;

cSerialPort::cSerialPort(const char* pPortName)
{
    pWriteToArduino = { "Play" };
    pStopArduinoOutput = { "Stop" };
    sFilePath = { "\SentryTurret\sounds" }; // Replace with 'sounds' folder's path 
    pIncomingData = { new char [iMaxDataLength] {} };
    bConnected = false;
    bIsDataSent = false;

    // Add / remove name of the files you would like to use here
    TurretDetectionWavFiles =   { "deploy", "alarm.wav", "alert.wav",  "active.wav", "turret_active_1.wav", "turret_active_2.wav",
                                  "turret_active_3.wav", "turret_active_4.wav", "turret_active_5.wav", "turret_active_6.wav",
                                  "turret_active_7.wav", "turret_active_8.wav" };
    TurretActivationWavFiles =  { "turret_deploy_1.wav", "turret_deploy_2.wav", "turret_deploy_3.wav", "turret_deploy_4.wav",
                                  "turret_deploy_5.wav", "turret_deploy_6.wav" };
    TurretSearchingWavFiles =   { "turret_search_1.wav", "turret_search_2.wav", "turret_search_3.wav", "turret_search_4.wav",
                                  "turret_autosearch_1", "turret_autosearch_2", "turret_autosearch_3", "turret_autosearch_4",
                                  "turret_autosearch_5", "turret_autosearch_6", "ping" };
    TurretPowerDownWavFiles =   { "turret_disabled_1.wav", "turret_disabled_2.wav", "turret_disabled_3.wav", "turret_disabled_4.wav",
                                  "turret_disabled_5.wav", "turret_disabled_6.wav", "turret_disabled_7.wav", "turret_disabled_8.wav",
                                  "turret_retire_1.wav", "turret_retire_2.wav", "turret_retire_3.wav", "turret_retire_4.wav",
                                  "turret_retire_5.wav", "turret_retire_6.wav", "turret_retire_7.wav", "retract.wav" };
    TurretAttackWavFiles =      { "turret_fire_4x_01.wav", "turret_fire_4x_02.wav", "turret_fire_4x_03.wav" };

    TurretMovedWavFiles =       { "turretlaunched10.wav", "turret_pickup_1.wav", "turret_pickup_2.wav", "turret_pickup_3.wav",
                                  "turret_pickup_4.wav", "turret_pickup_5.wav", "turret_pickup_6.wav", "turret_pickup_7.wav",
                                  "turret_pickup_8.wav", "turret_pickup_9.wav", "turret_pickup_10.wav" };
    TurretDie =                 { "sp_sabotage_factory_good_fail01.wav", "sp_sabotage_factory_good_fail04.wav",
                                  "sp_sabotage_factory_good_fail06.wav", "sp_sabotage_factory_good_fail05.wav",
                                  "sp_sabotage_factory_good_fail07.wav", "turret_tipped_1", "turret_tipped_2",
                                  "turret_tipped_3", "turret_tipped_4", "turret_tipped_5", "turret_tipped_6", "die.wav" };

    // We're not yet connected
    this->bConnected = false;
    // Try to connect to the given port through CreateFileA
    this->Handler = CreateFileA(pPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    // Check if the connection was successful
    if (this->Handler == INVALID_HANDLE_VALUE)
    {
        // If it was not, display an error. For tracing other errors, try with $err,hr pseudovariable during debugging.
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            printf("ERROR: Handle was not attached. Reason: %s not available.\n", pPortName);
        else
            printf("ERROR!!!");
    }
    else
    {
        // If we got connected, try to set communication parameters between Arduino and the app
        DCB dcbSerialParams = { 0 };
        // Try retrieve current control settings for Arduino 
        if (!GetCommState(this->Handler, &dcbSerialParams))
            printf("failed to get current serial parameters!");
        else
        {
            // Define serial connection parameters for the Arduino board
            dcbSerialParams.BaudRate = CBR_9600;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;
            // Setting the DTR to Control_Enable ensures that the Arduino is properly reset upon establishing a connection
            dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
            // Set the parameters and check for their proper application
            if (!SetCommState(Handler, &dcbSerialParams))
                printf("ALERT: Could not set Serial Port parameters");
            else
            {
                // If everything went fine we're connected
                this->bConnected = true;
                // Flush any remaining characters in the buffer 
                PurgeComm(this->Handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                // Wait until the Arduino board is reset
                Sleep(iArduinoWaitTime);
            }
        }
    }
}

// rand() should only be called once (in main)
int GetRandomNumber(int iMin, int iMax)
{
    static constexpr double fraction{ 1.0 / (RAND_MAX + 1.0) };  // static used for efficiency, so we only calculate this value once
    // Evenly distribute the random number across our range
    return iMin + static_cast<int>((iMax - iMin + 1) * (std::rand() * fraction));
}

void playSoundAFunc(std::string sFilePath, std::string sWavFile)
{
    // 45 is the index of the first character of sWavFile within appended sFilePath
    int iNrOfFirstWavFileChar = 45;
    sFilePath.append(sWavFile);
    std::cout << sFilePath << "\n";
    PlaySoundA(sFilePath.c_str(), NULL, SND_NOSTOP);
    sFilePath.erase(iNrOfFirstWavFileChar, sWavFile.length()); 
}

void cSerialPort::PlayAudio(const char* pIncomingData, const int iMaxDataLength, std::array<std::string, 12> &TurretDetectionWavFiles, std::array<std::string, 6> &TurretActivationWavFiles,
    std::array<std::string, 11> &TurretSearchingWavFiles, std::array<std::string, 16> &TurretPowerDownWavFiles, std::array<std::string, 3> &TurretAttackWavFiles, std::array<std::string, 11> &TurretMovedWavFiles, 
    std::array<std::string, 12> &TurretDie)
{
    std::string incomingDataStr(pIncomingData, strnlen_s(pIncomingData, iMaxDataLength));
    std::cout << incomingDataStr;
    // std::find searches for first occurrence of a value in a container
    if (incomingDataStr.find("Ping") != std::string::npos)
    {
        std::string sWavFile{ TurretSearchingWavFiles[10] };
        std::thread t1(playSoundAFunc, sFilePath, sWavFile); // Execute this function call on a different thread to speed things up 
        t1.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretDetectionGuns") != std::string::npos)
    {
        std::string sWavFile{ TurretDetectionWavFiles[0] };
        std::thread t2(playSoundAFunc, sFilePath, sWavFile);
        t2.join();

        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretDetection") != std::string::npos)
    {
        std::string sWavFile{ TurretDetectionWavFiles[0] };
        std::thread t3(playSoundAFunc, sFilePath, sWavFile);
        t3.join();
        for (int i = 0; i < 4; i++)
        {
            std::string sWavFile{ TurretDetectionWavFiles[1] };
            std::thread t4(playSoundAFunc, sFilePath, sWavFile);
            t4.join();
        }
        sWavFile =  TurretDetectionWavFiles[3];
        std::thread t5(playSoundAFunc, sFilePath, sWavFile);
        t5.join();
        sWavFile = TurretDetectionWavFiles[GetRandomNumber(4, TurretDetectionWavFiles.size() - 1)];
        std::thread t6(playSoundAFunc, sFilePath, sWavFile);
        t6.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretActivation") != std::string::npos)
    {  
        std::string sWavFile{ TurretActivationWavFiles[GetRandomNumber(0, TurretActivationWavFiles.size() - 1)] };
        std::thread t7(playSoundAFunc, sFilePath, sWavFile);
        t7.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretSearching") != std::string::npos)
    {
        std::string sWavFile{ TurretSearchingWavFiles[GetRandomNumber(0, TurretSearchingWavFiles.size() - 1)] };
        std::thread t8(playSoundAFunc, sFilePath, sWavFile);
        t8.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretPowerDown") != std::string::npos)
    {
        std::string sWavFile{ TurretPowerDownWavFiles[GetRandomNumber(0, TurretPowerDownWavFiles.size() - 1)] };
        std::thread t9(playSoundAFunc, sFilePath, sWavFile);
        t9.join();
        incomingDataStr.erase();
        sWavFile = TurretPowerDownWavFiles[15];
        std::thread t10(playSoundAFunc, sFilePath, sWavFile);
        t10.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretAttack") != std::string::npos)
    {
        std::string sWavFile{ TurretAttackWavFiles[GetRandomNumber(0, TurretAttackWavFiles.size() - 1)] };
        std::thread t11(playSoundAFunc, sFilePath, sWavFile); 
        t11.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretMoved") != std::string::npos)
    {
        std::string sWavFile{ TurretMovedWavFiles[GetRandomNumber(0, TurretMovedWavFiles.size() - 1)] };
        std::thread t12(playSoundAFunc, sFilePath, sWavFile);
        t12.join();
        incomingDataStr.erase();
    }
    if (incomingDataStr.find("turretDie") != std::string::npos)
    {
        std::string sWavFile{ TurretDie[GetRandomNumber(0, TurretDie.size() - 1)] };
        std::thread t13(playSoundAFunc, sFilePath, sWavFile);
        t13.join();
        incomingDataStr.erase();
    }
}

cSerialPort::~cSerialPort()
{
    // Check if we are connected before trying to disconnect
    if (this->bConnected)
    {
        // We're no longer connected
        this->bConnected = false;
        // Close the serial Handler
        CloseHandle(this->Handler);
    }
}

int cSerialPort::ReadSerialPort(const char* pBuffer, unsigned int iNbOfChars)
{
    // Number of bytes to read. DWORD is needed for usage of Handler.
    DWORD BytesRead;
    // Number of actual bytes we will ask to read
    unsigned int iToRead;
    // Size of buffer paramater
    int iSizeOfIncomingData{ static_cast<int>(sizeof(pBuffer)) };
    // Use ClearCommError() to retrieve information about communications error and report the current status of a communications device
    ClearCommError(this->Handler, &this->Errors, &this->Status);
    // Check if there is something to read
    if (this->Status.cbInQue > 0)
    {
        // If yes, we check if there is enough data to read the required number of chars
        // If not, we will read only the available characters to prevent locking of the application
        if (this->Status.cbInQue > iNbOfChars)
            iToRead = iNbOfChars;
        else
            iToRead = this->Status.cbInQue;
        // Try to read the required number of chars, and return the number of read bytes on success
        if (ReadFile(this->Handler, (LPVOID)pBuffer, iToRead, &BytesRead, NULL))
            return BytesRead;
    }
    // If nothing has been read, or an error was detected, return 0
    return 0;
}

bool cSerialPort::WriteSerialPort(const char* pBuffer, unsigned int iNbOfChars)
{
    DWORD bytesSend;
    // Try to write contents of the buffer on the serial port
    if (!WriteFile(this->Handler, (void*)pBuffer, iNbOfChars, &bytesSend, 0))
    {
        // In case it does not work, get comm error and return false
        ClearCommError(this->Handler, &this->Errors, &this->Status);
        return false;
    }
    else
        // Set bIsDataSent flag true as well 
        bIsDataSent = true;
        return true;
}

bool cSerialPort::IsConnected()
{
    // Return the connection status
    return this->bConnected;
}

void cSerialPort::ExampleReceiveData(void)
{
    int iReadResult = Arduino->cSerialPort::ReadSerialPort(pIncomingData, iMaxDataLength);
    std::string IncomingDataStr(pIncomingData, strnlen_s(pIncomingData, iMaxDataLength));
    // Send "Play" to Arduino
    Arduino->WriteSerialPort(pWriteToArduino, iWriteToArduinoLength);
    // Check if Arduino replies, and play the audio file
    if (IncomingDataStr.find("Play") != std::string::npos)
    {
          Arduino->PlayAudio(pIncomingData, iMaxDataLength, TurretDetectionWavFiles, TurretActivationWavFiles,
            TurretSearchingWavFiles, TurretPowerDownWavFiles, TurretAttackWavFiles, TurretMovedWavFiles, TurretDie);
    }
    // Send "Stop" to Arduino
    Arduino->WriteSerialPort(pStopArduinoOutput, iWriteToArduinoLength);
    Sleep(50);
}

void cSerialPort::AutoConnect(void)
{
    while(1) {
        std::cout << "Searching in progress";
        // Wait until connection is established 
        while (!Arduino->cSerialPort::IsConnected()) {
            Sleep(100);
            std::cout << ".";
            Arduino = new cSerialPort(pPortName);
        }
        // Checking if Arduino is connected or not
        if (Arduino->cSerialPort::IsConnected())
            std::cout  << std::endl << "Connection established at port " << pPortName << std::endl;
        PurgeComm(Arduino->Handler, PURGE_RXCLEAR);
        while(Arduino->cSerialPort::IsConnected()) ExampleReceiveData();     
    }
}

int main(int argc, char* argv[])
{
    // Set initial seed value to system clock
    std::srand(static_cast<unsigned int>(std::time(nullptr))); 
    Arduino = new cSerialPort(pPortName);
    Arduino->AutoConnect();
}
