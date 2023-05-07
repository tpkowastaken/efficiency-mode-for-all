#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <unistd.h>
#include <signal.h> 
#include <string.h>

void padString(char* str) {
    int len = strlen(str);
    if (len < 50) {
        for (int i = len; i < 50; i++) {
            str[i] = ' ';
        }
        str[50] = '\0'; // Add null terminator
    }
}

// Sets the process priority to IDLE_PRIORITY_CLASS.
void set_process_priority(HANDLE hProcess)
{
    SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
}

// Enables EcoQos to reduce the clock speed.
void enable_ecoqos(HANDLE hprocess)
{
    PROCESS_POWER_THROTTLING_STATE PowerThrottling = { 0 };
    PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    PowerThrottling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

    SetProcessInformation(hprocess, ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling));
}
int main()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPosition;
    cursorPosition.X = 0;
    cursorPosition.Y = 1;
    printf("hold X to exit.\n");
    FILE *fp;
    fp = fopen("exceptions.conf", "r");
    if (fp == NULL){
        printf("open file failed\n");
        return -1;
    }
    char programExceptions[1024][1024];
    char line[1024];
    {
        int i = 0;
        while (fgets(line, 1024, fp) != NULL){
            if (line[0] == '#')
                continue;
            printf("%s\n", line);
            strcpy(programExceptions[i], line);
            i++;
        }
    }
    fclose(fp);
    while(1){
        if (GetAsyncKeyState('X') & 0x8000)
        {
            printf("Interrupt signal received.\n");
            break; // Exit the loop
        }
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            printf("Failed to create process snapshot\n");
            return 1;
        }

        PROCESSENTRY32 processEntry = { 0 };
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(snapshot, &processEntry))
        {
            do
            {
                HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID);
                TCHAR exeName[MAX_PATH];
                DWORD len = GetProcessImageFileName(processHandle, exeName, MAX_PATH);
                if (len > 0)
                {
                    // extract the executable name from the path
                    TCHAR* exeFileName = _tcsrchr(exeName, '\\');
                    if (exeFileName != NULL)
                    {
                        SetConsoleCursorPosition(hConsole, cursorPosition);
                        exeFileName++;
                        char str[1024];
                        sprintf(str, "setting process to efficiency: %s\n", exeFileName);
                        printf("\033[K");
                        printf("%s", str);
                    }
                    //printf("Executable name: %s\n", programExceptions[0]);
                    for(int i = 0; i < 1024; i++){
                        if (_tcscmp(exeFileName, programExceptions[i]) != 0)continue;
                        else{
                            processHandle = NULL;
                        }
                    }
                }
                if (processHandle != NULL)
                {
                    set_process_priority(processHandle);
                    enable_ecoqos(processHandle);
                    
                    CloseHandle(processHandle);
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        else
        {
            printf("Failed to enumerate processes\n");
            CloseHandle(snapshot);
            return 1;
        }
        CloseHandle(snapshot);

        {
            SetConsoleCursorPosition(hConsole, cursorPosition);
            char str[1024];
            printf("\033[K");
            sprintf(str, "waiting 2 seconds...\n");
            printf("%s", str);
            sleep(2);
        }
    }
    return 0;
}
