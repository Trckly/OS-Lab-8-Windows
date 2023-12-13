#include "ThreadManager.h"

HANDLE ThreadManager::hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
CRITICAL_SECTION ThreadManager::Crit1cal;
const std::string ThreadManager::Info = "Shlomiak Danylo StudentID:BK13914809\n";

ThreadManager::ThreadManager(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::ThreadManagerClass())
{
    ui->setupUi(this);

    ui->comboBox->addItems({ "1", "2", "4", "8", "16" });

    InitializeCriticalSection(&Crit1cal);

    liDueTime.QuadPart = 0;

    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels({"TID", "Status", "Priority"});

    ui->ThreadOptionsBox->addItems({ "Suspend/Resume", "Set Priority", "Kill" });

    tsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    tsa.bInheritHandle = TRUE;
    tsa.lpSecurityDescriptor = nullptr;

    outputTimer = new QTimer(this);

    connect(outputTimer, &QTimer::timeout, this, &ThreadManager::HandleExit);

    outputTimer->setInterval(1000);
    outputTimer->start();

    ui->priorityBox->addItems({ "Time Critical", "Highest", "Above Normal", "Normal", "Below Normal", "Idle", "Lowest" });

    QLibrary mylib("Dll1");
    if (!mylib.isLoaded())
    {
        QMessageBox::information(this, "Error!", "Can`t open Lib " + mylib.errorString());
    }

    ConvertKernelTime = (MYFUNC)mylib.resolve("ConvertKernelTime");
    if (!ConvertKernelTime)
    {
        QMessageBox::information(this, "Error!", "Can`t bind ConvertKernelTime " + mylib.errorString());
    }
}

ThreadManager::~ThreadManager()
{
    delete ui;
}

void ThreadManager::OutputInfo()
{
    ui->tableWidget->setRowCount(th.size());
    std::vector<QTableWidgetItem*> table_item(ui->tableWidget->columnCount());
    for (int i = 0; i < th.size(); ++i) {
        table_item[0] = new QTableWidgetItem(QString::number(dwThread[i]));

        if (IsThreadSuspended(th[i]))
            table_item[1] = new QTableWidgetItem("Suspended");
        else
            table_item[1] = new QTableWidgetItem("Running");

        table_item[2] = new QTableWidgetItem(GetPriorityName(GetThreadPriority(th[i])));
        for (int j = 0; j < ui->tableWidget->columnCount(); ++j) {
            ui->tableWidget->setItem(i, j, table_item[j]);
        }
    }
}

bool ThreadManager::IsThreadSuspended(HANDLE hThread)
{
    // Suspend the thread temporarily
    DWORD suspendCount = SuspendThread(hThread);

    if (suspendCount == (DWORD)-1) {
        // The thread is not running or an error occurred
        ResumeThread(hThread); // Resume the thread if it was not running
        return false;
    }

    // Resume the thread to its original state
    DWORD resumeCount = ResumeThread(hThread);

    if (resumeCount == (DWORD)-1) {
        // An error occurred while resuming the thread
        return false;
    }

    // Check if the thread was suspended
    return (suspendCount > 0) && (resumeCount > suspendCount);
}

QString ThreadManager::GetPriorityName(int dwPriority)
{
    QString result;
    switch (dwPriority)
    {
    case THREAD_PRIORITY_NORMAL:
        result = "Normal";
        break;

    case THREAD_PRIORITY_ABOVE_NORMAL:
        result = "Above Normal";
        break;

    case THREAD_PRIORITY_BELOW_NORMAL:
        result = "Below Normal";
        break;

    case THREAD_PRIORITY_HIGHEST:
        result = "Highest";
        break;

    case THREAD_PRIORITY_TIME_CRITICAL:
        result = "Time Critical";
        break;

    case THREAD_PRIORITY_IDLE:
        result = "Idle";
        break;

    case THREAD_PRIORITY_LOWEST:
        result = "Lowest";
        break;

    default:
        break;
    }

    return result;
}

void ThreadManager::HandleExit()
{
    if (!th.empty()) {
        for (int i = th.size() - 1; i >= 0; --i)
        {
            if (!IsThreadSuspended(th[i]))
            {
                if (WaitForSingleObject(th[i], 0) == WAIT_OBJECT_0)
                {
                    FILETIME creationTime, exitTime, kernelTime, userTime;
                    GetThreadTimes(th[i], &creationTime, &exitTime, &kernelTime, &userTime);
                    OverallTime += ConvertKernelTime(kernelTime);

                    CloseHandle(th[i]);

                    th.erase(th.begin() + i);
                    dwThread.erase(dwThread.begin() + i);

                    ui->tableWidget->removeRow(i);
                }
            }
        }
        OutputInfo();
    }
    if (ui->tableWidget->rowCount() == 0) {
        ui->TimeOutput->setText(QString::number(OverallTime));
    }
}

int ThreadManager::GetPriorityValue()
{
    switch (ui->priorityBox->currentIndex())
    {
    case 0:
        return THREAD_PRIORITY_TIME_CRITICAL;

    case 1:
        return THREAD_PRIORITY_HIGHEST;

    case 2:
        return THREAD_PRIORITY_ABOVE_NORMAL;

    case 3:
        return THREAD_PRIORITY_NORMAL;

    case 4:
        return THREAD_PRIORITY_BELOW_NORMAL;

    case 5:
        return THREAD_PRIORITY_IDLE;

    case 6:
        return THREAD_PRIORITY_LOWEST;

    default:
        break;
    }
    return -1;
}

DWORD WINAPI ThreadManager::CycleOutputSyncCritical(PVOID pvParam)
{
    DWORD dwResult = 0;
    int* gap = static_cast<int*>(pvParam);
    int counter = *gap;
    EnterCriticalSection(&Crit1cal);
    std::ofstream fout(R"(\\Mac\Home\Documents\OC\OC Lab 3\ThreadManager\Log.txt)", std::ios::app);

    for (int i = 0; i < counter; ++i)
        for (auto Character : Info)
            fout << Character;

    fout.close();
    LeaveCriticalSection(&Crit1cal);
    return dwResult;
}

DWORD WINAPI ThreadManager::CycleOutputSyncTimer(PVOID pvParam)
{

    DWORD dwResult = 0;
    int* gap = static_cast<int*>(pvParam);
    int counter = *gap;
    WaitForSingleObject(hTimer, INFINITE);
    std::ofstream fout(R"(\\Mac\Home\Documents\OC\OC Lab 3\ThreadManager\Log.txt)", std::ios::app);

    for (int i = 0; i < counter; ++i)
        for (auto Character : Info)
            fout << Character;

    fout.close();
    return dwResult;
}

DWORD WINAPI ThreadManager::CycleOutputAsync(PVOID pvParam)
{
    DWORD dwResult = 0;
    int* gap = static_cast<int*>(pvParam);
    int counter = *gap;
    std::ofstream fout(R"(\\Mac\Home\Documents\OC\OC Lab 3\ThreadManager\Log.txt)", std::ios::app);

    for (int i = 0; i < counter; ++i)
        for (auto Character : Info)
            fout << Character;

    fout.close();
    return dwResult;
}

void ThreadManager::on_commitButton_clicked()
{
    bool ok, found = false;
    HANDLE current_th;
    DWORD current_dwThread = ui->lineEdit->text().toInt(&ok);
    if (!ok) {
        QMessageBox::information(this, "Error!", "TID is not valid!");
        return;
    }
    for (int i = 0; i < dwThread.size(); ++i) {
        if (current_dwThread == dwThread[i]) {
            current_th = th[i];
            found = true;
        }
    }

    if (!found) {
        QMessageBox::information(this, "Error!", "TID is not valid!");
        return;
    }

    switch (ui->ThreadOptionsBox->currentIndex()) {
    case CommitAction::Suspend:
        if (IsThreadSuspended(current_th))
            ResumeThread(current_th);
        else
            SuspendThread(current_th);
        break;

    case CommitAction::Priority:
        SetThreadPriority(current_th, GetPriorityValue());
        break;

    case CommitAction::Kill:
        TerminateThread(current_th, 10);
        break;
    }
}

void ThreadManager::on_startButton_clicked() {
    if (!th.empty()) {
        for (int i = 0; i < th.size(); ++i) {
            TerminateThread(th[i], 100);
            CloseHandle(th[i]);
            dwThread.erase(dwThread.begin() + i);
        }
        OutputInfo();
    }

    OverallTime = 0;
    int ThreadNum = ui->comboBox->currentText().toInt();

    std::ofstream fout(R"(\\Mac\Home\Documents\OC\OC Lab 3\ThreadManager\Log.txt)", std::ios::trunc);
    fout.close();

    th.resize(ThreadNum);
    dwThread.resize(ThreadNum);

    if (ThreadNum == 1)
    {
        th[0] = CreateThread(&tsa, 0, CycleOutputAsync, static_cast<LPVOID>(&GeneralGap), 0, &dwThread[0]);
    }
    else
    {
        int Gap = GeneralGap / ThreadNum;
        th.resize(ThreadNum);
        dwThread.resize(ThreadNum);

        bool bCriticalSynchronise = ui->CriticalRadio->isChecked();
        bool bTimerSynchronise = ui->TimerRadio->isChecked();

        if (bCriticalSynchronise)
        {
            for (int i = 0; i < ThreadNum; ++i)
                th[i] = CreateThread(&tsa, 0, CycleOutputSyncCritical, static_cast<LPVOID>(&Gap), 0, &dwThread[i]);
        }
        else if (bTimerSynchronise)
        {
            if (!SetWaitableTimer(hTimer, &liDueTime, 14000 / ThreadNum, NULL, NULL, 0))
            {
                QMessageBox::information(this, "Error!", QString::number(GetLastError()));
                return;
            }
            for (int i = 0; i < ThreadNum; ++i) {
                th[i] = CreateThread(&tsa, 0, CycleOutputSyncTimer, static_cast<LPVOID>(&Gap), 0, &dwThread[i]);
            }
        }
        else
        {
            for (int i = 0; i < ThreadNum; ++i)
                th[i] = CreateThread(&tsa, 0, CycleOutputAsync, static_cast<LPVOID>(&Gap), 0, &dwThread[i]);
        }
    }
}
