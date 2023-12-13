#include "mainwindow.h"

QtTaskManager::QtTaskManager(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.comboBox->addItems({ Tabulation, WMP, Netstat, BinSearch });
    ui.processActionBox->addItems({ SuspendToggle, Priority, Affinity, Terminate });
    ui.processTable->setColumnCount(7);
    ui.processTable->setHorizontalHeaderLabels({ "Process Name", "PID", "Thread ID", "Status", "Priority", "Affinity Mask", "CPU Time" });

    updateTimer = new QTimer(this);

    connect(updateTimer, &QTimer::timeout, this, &QtTaskManager::PrExitHandler);

    updateTimer->setInterval(5000);
    updateTimer->start();

    bDenyUser = true;
    bReadonly = true;
    bSetHidden = true;

    HANDLE hToken;
    GetProcessToken(hToken);
}

QtTaskManager::~QtTaskManager()
{}

void QtTaskManager::on_cp_button_clicked()
{
    int cp_name = ui.comboBox->currentIndex();
    ui.processTable->insertRow(ui.processTable->rowCount());

    switch (cp_name)
    {
    case 0 :
        CreateTabulation();

        break;

    case 1:
        CreateWMP();

        break;

    case 2:
        CreateNetstat();

        break;

    case 3:
        CreateBinSearch();

        break;

    default:
        ui.processTable->removeRow(ui.processTable->rowCount());
        break;
    }
    PrExitHandler();
    DisplayActiveProcesses();
}

BOOL QtTaskManager::GetProcessToken(HANDLE& hToken) {
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        QMessageBox::information(this, "Error!", "Failed to get process token");
        return FALSE;
    }
    return TRUE;
}

BOOL QtTaskManager::SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup
            &luid ) )        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError() );
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            (PTOKEN_PRIVILEGES) NULL,
            (PDWORD) NULL) )
    {
        printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}

DWORD AddAceToObjectsSecurityDescriptor (
    LPTSTR pszObjName,          // name of object
    SE_OBJECT_TYPE ObjectType,  // type of object
    LPTSTR pszTrustee,          // trustee for new ACE
    TRUSTEE_FORM TrusteeForm,   // format of trustee structure
    DWORD dwAccessRights,       // access mask for new ACE
    ACCESS_MODE AccessMode,     // type of ACE
    DWORD dwInheritance         // inheritance flags for new ACE
    )
{
    DWORD dwRes = 0;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;

    if (NULL == pszObjName)
        return ERROR_INVALID_PARAMETER;

    // Get a pointer to the existing DACL.

    dwRes = GetNamedSecurityInfo(pszObjName, ObjectType,
                                 DACL_SECURITY_INFORMATION,
                                 NULL, NULL, &pOldDACL, NULL, &pSD);
    if (ERROR_SUCCESS != dwRes) {
        printf( "GetNamedSecurityInfo Error %u\n", dwRes );
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for the new ACE.

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = dwAccessRights;
    ea.grfAccessMode = AccessMode;
    ea.grfInheritance= dwInheritance;
    ea.Trustee.TrusteeForm = TrusteeForm;
    ea.Trustee.ptstrName = pszTrustee;

    // Create a new ACL that merges the new ACE
    // into the existing DACL.

    dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes)  {
        printf( "SetEntriesInAcl Error %u\n", dwRes );
        goto Cleanup;
    }

    // Attach the new ACL as the object's DACL.

    dwRes = SetNamedSecurityInfo(pszObjName, ObjectType,
                                 DACL_SECURITY_INFORMATION,
                                 NULL, NULL, pNewDACL, NULL);
    if (ERROR_SUCCESS != dwRes)  {
        printf( "SetNamedSecurityInfo Error %u\n", dwRes );
        goto Cleanup;
    }

Cleanup:

    if(pSD != NULL)
        LocalFree((HLOCAL) pSD);
    if(pNewDACL != NULL)
        LocalFree((HLOCAL) pNewDACL);

    return dwRes;
}

bool QtTaskManager::CreateTabulation()
{
    int LimitA = 0, LimitB = 10000;
    float step = 1.f;

    std::string cmdlinestr = R"(\\Mac\Home\Documents\OC\OC_Lab_3\build-ParentTabulation-Desktop_Qt_6_6_1_MinGW_64_bit-Debug\debug\ParentTabulation.exe )" + std::to_string(LimitA) + ' ' + std::to_string(LimitB) + ' ' + std::to_string(step);
    std::wstring cmdline_w(cmdlinestr.cbegin(), cmdlinestr.cend());
    wchar_t* cmdline = const_cast<wchar_t*>(cmdline_w.data());

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE | INHERIT_PARENT_AFFINITY, NULL, NULL, &si, &pi))
    {
        QMessageBox::information(this, "Error!", "Failed to create child process!");
        ExitProcess(-1);
        return false;
    }
    std::string LogMessage = "\t" + GetDateTime() + "\npid = " + std::to_string(pi.dwProcessId) + "\ntid = " + std::to_string(pi.dwThreadId) + "\n\n";
    WriteLogToFile(LogMessage);
    printf("pid=%d, t1d=%d\n", pi.dwProcessId, pi.dwThreadId);
    active_processes.emplace_back(pi);

    getchar();

    return true;
}

bool QtTaskManager::CreateWMP()
{
    std::string cmdlinestr = R"(C:\Program Files (x86)\Windows Media Player\wmplayer.exe)";
    std::wstring cmdline_w(cmdlinestr.cbegin(), cmdlinestr.cend());
    wchar_t* cmdline = const_cast<wchar_t*>(cmdline_w.data());

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, INHERIT_PARENT_AFFINITY, NULL, NULL, &si, &pi))
    {
        QMessageBox::information(this, "Error!", "Failed to create child process!");
        ExitProcess(-1);
        return false;
    }

    std::string LogMessage = "\t" + GetDateTime() + "\npid = " + std::to_string(pi.dwProcessId) + "\ntid = " + std::to_string(pi.dwThreadId) + "\n\n";
    WriteLogToFile(LogMessage);

    active_processes.emplace_back(pi);

    return true;
}

bool QtTaskManager::CreateNetstat()
{
    std::string cmdlinestr = R"(netstat -n 10)";
    std::wstring cmdline_w(cmdlinestr.cbegin(), cmdlinestr.cend());
    wchar_t* cmdline = const_cast<wchar_t*>(cmdline_w.data());

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE | INHERIT_PARENT_AFFINITY, NULL, NULL, &si, &pi))
    {
        QMessageBox::information(this, "Error!", "Failed to create child process!");
        ExitProcess(-1);
        return false;
    }

    std::string LogMessage = "\t" + GetDateTime() + "\npid = " + std::to_string(pi.dwProcessId) + "\ntid = " + std::to_string(pi.dwThreadId) + "\n\n";
    WriteLogToFile(LogMessage);

    active_processes.emplace_back(pi);

    return true;
}

bool QtTaskManager::CreateBinSearch()
{
    std::string cmdlinestr = R"(C:\Users\illya\source\repos\binsearch\x64\Debug\binsearch.exe)";
    std::wstring cmdline_w(cmdlinestr.cbegin(), cmdlinestr.cend());
    wchar_t* cmdline = const_cast<wchar_t*>(cmdline_w.data());

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE | INHERIT_PARENT_AFFINITY, NULL, NULL, &si, &pi))
    {
        QMessageBox::information(this, "Error!", "Failed to create child process!");
        ExitProcess(-1);
        return false;
    }

    std::string LogMessage = "\t" + GetDateTime() + "\npid = " + std::to_string(pi.dwProcessId) + "\ntid = " + std::to_string(pi.dwThreadId) + "\n\n";
    WriteLogToFile(LogMessage);

    active_processes.emplace_back(pi);

    return true;
}

void QtTaskManager::Suspend(HANDLE hThread)
{
    SuspendThread(hThread);
}


void QtTaskManager::Resume(HANDLE hThread)
{
    ResumeThread(hThread);
}

BOOL QtTaskManager::SetAffinity(HANDLE hProcess, DWORD_PTR AffinityMask)
{
    return SetProcessAffinityMask(hProcess, AffinityMask);
}

BOOL QtTaskManager::SetPriority(HANDLE hProcess, int Priority)
{
    DWORD dwPriorityClass = 0;
    switch (Priority)
    {
    case 0:
        dwPriorityClass = 0x00000100;
        break;

    case 1:
        dwPriorityClass = 0x00000080;
        break;

    case 2:
        dwPriorityClass = 0x00008000;
        break;

    case 3:
        dwPriorityClass = 0x00000020;
        break;

    case 4:
        dwPriorityClass = 0x00004000;
        break;

    case 5:
        dwPriorityClass = 0x00000040;
        break;

    default:
        break;
    }

    return SetPriorityClass(hProcess, dwPriorityClass);
}

BOOL QtTaskManager::Kill(HANDLE hProcess)
{
    return TerminateProcess(hProcess, UI_TERMINATED);
}

void QtTaskManager::PrExitHandler()
{
    bool terminated = false;
    if (!active_processes.empty())
    {
        for (int i = active_processes.size() - 1; i >= 0; --i)
        {
            if (!IsProcessSuspended(active_processes[i].hThread))
            {
                if (terminated = ::WaitForSingleObject(active_processes[i].hProcess, 0) == WAIT_OBJECT_0)
                {
                    CloseHandle(active_processes[i].hProcess);
                    CloseHandle(active_processes[i].hThread);

                    active_processes.erase(active_processes.begin() + i);

                    ui.processTable->removeRow(i);

                    DisplayActiveProcesses();

                    terminated = false;
                }
            }
        }
    }
}

void QtTaskManager::DisplayActiveProcesses()
{
    if (!active_processes.empty())
    {
        size_t size = active_processes.size();

        for (int i = 0; i < size; ++i)
        {
            std::vector<QTableWidgetItem*> table_item;
            PROCESS_INFORMATION pi = active_processes[i];

            TCHAR szExeName[MAX_PATH];
            DWORD dwSize = sizeof(szExeName);

            DWORD dwCharsWritten = QueryFullProcessImageName(pi.hProcess, 0, szExeName, &dwSize);

            if (dwCharsWritten != 0) {
                QString pr_name = QString::fromWCharArray(szExeName);
                QFileInfo fileInfo(pr_name);
                QString executableName = fileInfo.fileName();

                table_item.push_back(new QTableWidgetItem(executableName));
            }

            QString pr_pid = QString::number(pi.dwProcessId);
            table_item.push_back(new QTableWidgetItem(pr_pid));

            QString pr_tid = QString::number(pi.dwThreadId);
            table_item.push_back(new QTableWidgetItem(pr_tid));

            bool bSuspended = IsProcessSuspended(pi.hThread);
            QString pr_status;
            if (bSuspended)
                pr_status = "Suspended";
            else
                pr_status = "Running";

            table_item.push_back(new QTableWidgetItem(pr_status));

            DWORD dwPriorityClass = GetPriorityClass(pi.hProcess);
            table_item.push_back(new QTableWidgetItem(GetPriorityName(dwPriorityClass)));

            DWORD_PTR processAffinityMask, systemAffinityMask;
            if (!GetProcessAffinityMask(pi.hProcess, &processAffinityMask, &systemAffinityMask))
                QMessageBox::information(this, "Error!", "Failed to retrieve process affinity mask!");
            else
            {
                QString bin = GetReadableAffinity(processAffinityMask);
                table_item.push_back(new QTableWidgetItem(bin));
            }

            FILETIME creationTime, exitTime, kernelTime, userTime;

            if (GetProcessTimes(pi.hProcess, &creationTime, &exitTime, &kernelTime, &userTime))
            {
                QString CPU_time = ConvertKernelTime(kernelTime);
                table_item.push_back(new QTableWidgetItem(CPU_time));
            }
            else
            {
                QMessageBox::information(this, "Error!", "Failed to retrieve process times!");
            }

            for (int j = 0; j < ui.processTable->columnCount(); ++j)
            {
                ui.processTable->setItem(i, j, table_item[j]);
            }
        }
    }
}

QString QtTaskManager::GetPriorityName(DWORD dwPriority)
{
    QString result;
    switch (dwPriority)
    {
    case NORMAL_PRIORITY_CLASS:
        result = "Normal";
        break;

    case ABOVE_NORMAL_PRIORITY_CLASS:
        result = "Above Normal";
        break;

    case BELOW_NORMAL_PRIORITY_CLASS:
        result = "Below Normal";
        break;

    case HIGH_PRIORITY_CLASS:
        result = "High";
        break;

    case REALTIME_PRIORITY_CLASS:
        result = "Realtime";
        break;

    case IDLE_PRIORITY_CLASS:
        result = "Idle";
        break;

    default:
        break;
    }

    return result;
}

QString QtTaskManager::GetReadableAffinity(DWORD_PTR dwAffinMask)
{

    QString result, temp = QString::number(static_cast<int>(dwAffinMask), 2);

    // Ensure the binary string is exactly 4 characters long by adding leading zeros.
    while (temp.length() < 4) {
        temp.prepend('0');
    }

    for (int i = temp.length() - 1; i >= 0; --i) {
        if (temp[i] == '1') {
            result.append(QString::number(3 - i) + " ");
        }
    }

    result = result.trimmed(); // Remove trailing space

    return result;
}

bool QtTaskManager::IsProcessSuspended(HANDLE hThread)
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

void QtTaskManager::on_commit_button_clicked()
{
    if (!active_processes.empty()) {

        bool b_pid_exists = false;
        QString spid = ui.pidLine->text();
        PROCESS_INFORMATION current_pi;

        for (const auto& pi : active_processes) {
            if (spid == QString::number(pi.dwProcessId)) {
                current_pi = pi;
                b_pid_exists = true;
            }
        }

        if (!b_pid_exists)
        {
            QMessageBox::information(this, "Error!", "Provided PID doesn't exist!");
        }
        else
        {
            SetPriorityWidget setPriorityWidget(this);
            SetAffinityWidget setAffinityWidget(this);
            switch (ui.processActionBox->currentIndex())
            {
            case 0:
                if (IsProcessSuspended(current_pi.hThread)) {
                    Resume(current_pi.hThread);
                }
                else {
                    Suspend(current_pi.hThread);
                }
                break;

            case 1:
                int inputIndex;
                if (setPriorityWidget.exec() == QDialog::Accepted) {
                    // User clicked OK
                    inputIndex = setPriorityWidget.GetSelectedIndex();
                    if (BOOL Success = SetPriority(current_pi.hProcess, inputIndex) == FALSE) {
                        QMessageBox::information(this, "Error!", "Failed to set desired priority!");
                    }
                }
                break;

            case 2:
                DWORD_PTR dwAffinityMask;
                if (setAffinityWidget.exec() == QDialog::Accepted) {
                    dwAffinityMask = static_cast<DWORD_PTR>(setAffinityWidget.GetAffinityMask());
                    if (dwAffinityMask == 0)
                        QMessageBox::information(this, "Error!", "Failed to set affinity! It cannot be none!");
                    else
                        SetAffinity(current_pi.hProcess, dwAffinityMask);
                }
                break;

            case 3:
                if (Kill(current_pi.hProcess) == FALSE)
                    QMessageBox::information(this, "Error!", "Failed to terminate process!");
                break;

            default:
                break;
            }
            DisplayActiveProcesses();
        }
    }
}

void QtTaskManager::on_forceUpdateButton_clicked()
{
    DisplayActiveProcesses();
}

QString QtTaskManager::ConvertKernelTime(FILETIME kernelTime)
{
    ULONGLONG microseconds, milliseconds, seconds, minutes;
    ULONGLONG kernelTimeIn100NanoSeconds = ((ULONGLONG)kernelTime.dwHighDateTime << 32) + kernelTime.dwLowDateTime;


    // Convert 100-nanosecond intervals to microseconds.
    microseconds = kernelTimeIn100NanoSeconds / 10;

    // Calculate minutes, seconds, and milliseconds from microseconds.
    milliseconds = microseconds / 1000;
    seconds = milliseconds / 1000;
    minutes = seconds / 60;

    // Reduce seconds, milliseconds, and microseconds accordingly.
    seconds %= 60;
    milliseconds %= 1000;
    microseconds %= 1000;

    QString result;
    return result = QString::number(minutes) + ':' + QString::number(seconds) + ':' + QString::number(milliseconds) + ':' + QString::number(microseconds);
}

std::string QtTaskManager::GetDateTime(){
    std::time_t CurrentTime = std::time(nullptr);
    return std::ctime(&CurrentTime);
}

void QtTaskManager::OpenLogFileW(LPCTSTR PathToFile){
    if((LogFileHandle = CreateFile(PathToFile, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE){
        QMessageBox::information(this, "Error!", "Failed to open log file for write!");

        if((LogFileHandle = CreateFile(PathToFile, GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE){
            QMessageBox::information(this, "Error!", "Failed to open log file!");
        }
    }
}

void QtTaskManager::WriteLogToFile(std::string Message){
    OpenLogFileW(FILEPATH);
    if(LogFileHandle != INVALID_HANDLE_VALUE){
        const char* Message_c_str = Message.c_str();
        DWORD BytesWritten;
        SetFilePointer(LogFileHandle, 0, NULL, FILE_END);
        if(!WriteFile(LogFileHandle, Message_c_str, strlen(Message_c_str), &BytesWritten, 0)){
            QMessageBox::information(this, "Error!", "Failed to write into log file!");
        }
        CloseHandle(LogFileHandle);
    }
}

void QtTaskManager::on_denyUserButton_clicked()
{
    LPTSTR Trustee = TEXT("TestUser");
    if(bDenyUser){
        AddAceToObjectsSecurityDescriptor(const_cast<LPTSTR>(FILEPATH), SE_FILE_OBJECT, Trustee, TRUSTEE_IS_NAME, FILE_GENERIC_READ, DENY_ACCESS, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        ui.denyUserButton->setText("Allow Access");
    }
    else{
        AddAceToObjectsSecurityDescriptor(const_cast<LPTSTR>(FILEPATH), SE_FILE_OBJECT, Trustee, TRUSTEE_IS_NAME, FILE_GENERIC_READ, GRANT_ACCESS, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        ui.denyUserButton->setText("Deny Access");
    }
    bDenyUser = !bDenyUser;
}


PSECURITY_DESCRIPTOR QtTaskManager::GetFileSecurityDescriptor(){
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD dwBytesNeeded;

    if (!GetFileSecurity(
            FILEPATH,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            NULL,
            0,
            &dwBytesNeeded
            )) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            QMessageBox::information(this, "Error getting file security information. ", QString::number(GetLastError()));
        }
    }

    pSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(dwBytesNeeded);
    if (pSecurityDescriptor == NULL) {
        QMessageBox::information(this, "Error allocating memory for security descriptor. ", QString::number(GetLastError()));
    }

    if (!GetFileSecurity(
            FILEPATH,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            pSecurityDescriptor,
            dwBytesNeeded,
            &dwBytesNeeded
            )) {
        QMessageBox::information(this, "Error getting file security information. ", QString::number(GetLastError()));
        free(pSecurityDescriptor);
    }
    return pSecurityDescriptor;
}

void QtTaskManager::on_hideFileButton_clicked()
{
    if(bSetHidden){
        SetFileAttributesW(FILEPATH, FILE_ATTRIBUTE_HIDDEN);
        ui.hideFileButton->setText("Unhide File");
    }
    else{
        SetFileAttributesW(FILEPATH, FILE_ATTRIBUTE_NORMAL);
        ui.hideFileButton->setText("Hide File");
    }
    bSetHidden = !bSetHidden;
}


void QtTaskManager::on_readonlyButton_clicked()
{
    if(bReadonly){
        SetFileAttributesW(FILEPATH, FILE_ATTRIBUTE_READONLY);
        ui.readonlyButton->setText("Disable Read-only");
    }
    else{
        SetFileAttributesW(FILEPATH, FILE_ATTRIBUTE_NORMAL);
        ui.readonlyButton->setText("Enable Read-only");
    }
    bReadonly = !bReadonly;
}

