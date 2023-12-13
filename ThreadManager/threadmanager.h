#pragma once

#include <Windows.h>
#include <fstream>
#include <QMessagebox>
#include <QTimer>
#include <QtWidgets/QMainWindow>
#include <QLibrary>
#include "ui_ThreadManager.h"

typedef ULONGLONG (__cdecl *MYFUNC)(FILETIME kernelTime);

QT_BEGIN_NAMESPACE
namespace Ui { class ThreadManagerClass; }
QT_END_NAMESPACE

class ThreadManager : public QMainWindow
{
    Q_OBJECT

public:
    ThreadManager(QWidget* parent = nullptr);
    ~ThreadManager();

    void OutputInfo();

    bool IsThreadSuspended(HANDLE hThread);

    QString GetPriorityName(int dwPriority);

    void HandleExit();

    MYFUNC ConvertKernelTime;

    int GetPriorityValue();

    static DWORD WINAPI CycleOutputSyncCritical(PVOID pvParam);
    static DWORD WINAPI CycleOutputSyncTimer(PVOID pvParam);
    static DWORD WINAPI CycleOutputAsync(PVOID pvParam);

private slots:
    void on_startButton_clicked();
    void on_commitButton_clicked();

private:
    enum CommitAction {
        Suspend,
        Priority,
        Kill
    };

    Ui::ThreadManagerClass* ui;

    static const std::string Info;

    static CRITICAL_SECTION Crit1cal;

    static HANDLE hTimer;

    int GeneralGap = 1000000;

    LARGE_INTEGER liDueTime;

    SECURITY_ATTRIBUTES tsa;

    std::vector<HANDLE> th;

    std::vector<DWORD> dwThread;

    std::chrono::high_resolution_clock::time_point StartTime, EndTime;
    std::chrono::microseconds Duration;

    QTimer* outputTimer;

    unsigned long long OverallTime = 0;
};
