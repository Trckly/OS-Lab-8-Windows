#pragma once

#include <vector>
#include <ctime>
#include <string>
#include <Windows.h>
#include <aclapi.h>
#include <winnt.h>
#include <tlhelp32.h>
#include <QMessageBox>
#include <QTimer>
#include <QFileInfo>
#include <QtWidgets/QMainWindow>
#include "setprioritywidget.h"
#include "setaffinitywidget.h"
#include "ui_QtTaskManager.h"

#define UI_TERMINATED 100

const LPCTSTR FILEPATH = TEXT("C:\\Log\\Log.txt");

enum E_cpFields
{
    tabulation,
    wmp,
    netstat,
    binsearch
};

class QtTaskManager : public QMainWindow
{
    Q_OBJECT

public:
    QtTaskManager(QWidget *parent = nullptr);
    ~QtTaskManager();

    //Processes functions
    bool CreateTabulation();

    bool CreateWMP();

    bool CreateNetstat();

    bool CreateBinSearch();

    void Suspend(HANDLE hThread);

    void Resume(HANDLE hThread);

    BOOL SetAffinity(HANDLE hProcess, DWORD_PTR AffinityMask);

    BOOL SetPriority(HANDLE hProcess, int Priority);

    BOOL Kill(HANDLE hProcess);

    //Process terminating handler
    void PrExitHandler();

    //Displaying info function
    void DisplayActiveProcesses();

    //Helping functions
    QString GetPriorityName(DWORD dwPriority);

    QString GetReadableAffinity(DWORD_PTR dwAffinMask);

    QString ConvertKernelTime(FILETIME kernelTime);

    bool IsProcessSuspended(HANDLE hThread);

    void OpenLogFileW(LPCTSTR PathToFile);

    void WriteLogToFile(std::string Message);

    std::string GetDateTime();

    PSECURITY_DESCRIPTOR GetFileSecurityDescriptor();

    BOOL GetProcessToken(HANDLE& hToken);

    BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);


private slots:
    void on_cp_button_clicked();
    void on_commit_button_clicked();
    void on_forceUpdateButton_clicked();

    void on_denyUserButton_clicked();

    void on_hideFileButton_clicked();

    void on_readonlyButton_clicked();

private:
    Ui::QtTaskManagerClass ui;

    //Fields for combo menu of create process
    QString Tabulation = "Tabulation", WMP = "Windows Media Player", Netstat = "netstat -n 10 process", BinSearch = "Binary search";
    QString SuspendToggle = "Suspend/Resume", Priority = "Set Priority", Affinity = "Set Affinity", Terminate = "Kill";

    std::vector<PROCESS_INFORMATION> active_processes;

    QTimer *updateTimer, *updateDisplayTimer;

    HANDLE LogFileHandle;

    bool bDenyUser;
    bool bSetHidden;
    bool bReadonly;
};
