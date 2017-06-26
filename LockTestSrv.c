#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <Wtsapi32.h>
#include <Winuser.h>

#define SLEEP_TIME 5000
#define LOG_FILE "c:\\LockTestSrvLog.txt"
#define SERVICE_NAME    "LockTestSrv"
#define SERVICE_DESC    "Lock"
#define SERVICE_DISPLAY_NAME "Lock"
#pragma comment(lib,"Wtsapi32.lib")
SERVICE_STATUS    ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
SC_HANDLE scm;
SC_HANDLE scv;

void ServiceMain(int argc, char** argv);
void ControlHandler(DWORD request);
VOID HandlerEx(DWORD controlCode,   DWORD dwEventType,   LPVOID lpEventData,   LPVOID lpContext);
void Log(char* filename);
int startFunc();
void OnStart();
void OnCreate();
void OnDelete();
void OnStop();

int main(int argc, char* argv[])
{
    // Service Name:MemoryStatus
    // Service Handle Function: ServiceMain()
    SERVICE_TABLE_ENTRY ServiceTable[2] =
    {

        { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },

        { NULL,NULL}
    };

    if(argc == 2)
    {
        if(!stricmp(argv[1],"-create"))
        {
            OnCreate();
            return 0;
        }
        else if(!stricmp(argv[1],"-delete"))
        {
            OnDelete();
            return 0;
        }
        else if(!stricmp(argv[1],"-start"))
        {
            OnStart();
            return 0;
        }
        else if(!stricmp(argv[1],"-stop"))
        {
            OnStop();
            return 0;
        }
        else
        {
            printf("invailid parameter\n");
            return 0;
        }
    }


    StartServiceCtrlDispatcher(ServiceTable);
	//Connects the main thread of a service process to the service control manager, which causes the thread to be the service control dispatcher thread for the calling process.
    return 0;
}

void Log(char* str)
{
    FILE* fp = fopen(LOG_FILE, "a+");
    if(fp == NULL)
    {
        printf("error to open file: %d\n", GetLastError());
        return;
    }

    fprintf(fp, "%s\n", str);
    fflush(fp);
    fclose(fp);
}

void ServiceMain(int argc, char** argv)
{
    BOOL bRet;
    int result;

    bRet = TRUE;

    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwCheckPoint   = 0;
    ServiceStatus.dwWaitHint   = 0;
    ServiceStatus.dwServiceType   = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;//dwControlsAccepted返回的是是否服务能被停止、暂停和恢复。
    ServiceStatus.dwServiceSpecificExitCode = 0;

    hStatus = RegisterServiceCtrlHandlerEx(SERVICE_NAME, (LPHANDLER_FUNCTION_EX)HandlerEx,NULL);
    //此处必须用RegisterServiceCtrlHandlerEx，不能用RegisterServiceCtrlHandler，因为RegisterServiceCtrlHandler对应的Handler不支持SERVICE_CONTROL_SESSIONCHANGE；
    //而RegisterServiceCtrlHandlerEx对应的HandlerEx支持SERVICE_CONTROL_SESSIONCHANG。
    //hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, HandlerEx,NULL);
	//RegisterServiceCtrlHandler : Registers a function to handle service control requests.
	//This function has been superseded by the RegisterServiceCtrlHandlerEx function.
	//If the function succeeds, the return value is a service status handle.
	//调用RegisterServiceCtrlHandlerEx 函数去注册服务的句柄控制请求，返回值就是服务的状态句柄，可以用来通知SCM服务的状态。
    if(hStatus == (SERVICE_STATUS_HANDLE)0)
    {
        // log failed
        return;
    }
    //service status update
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    while(ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        //result = startFunc();
        if(result)
        {
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceStatus.dwWin32ExitCode = -1;
            SetServiceStatus(hStatus, &ServiceStatus);
            return;
        }
    }
}

int startFunc()
{
    MessageBox(NULL, "startFunc", SERVICE_NAME, MB_OK);
    return 0;
}



void OnCreate()
{
    char filename[MAX_PATH];
    DWORD dwErrorCode;
    GetModuleFileName(NULL, filename, MAX_PATH);
    printf("Creating Service .... ");
    //OpenSCManager function:Establishes a connection to the service control manager on the specified computer and opens the specified service control manager database.
    scm = OpenSCManager(0/*localhost*/,
                        NULL/*SERVICES_ACTIVE_DATABASE*/,
                        SC_MANAGER_ALL_ACCESS/*ACCESS*/);
    if (scm == NULL)
    {
        printf("OpenSCManager error:%d\n", GetLastError());
        return;
    }
    //CreateService function : Creates a service object and adds it to the specified service control manager database.
    scv = CreateService(scm,//句柄
    SERVICE_NAME,//服务开始名
    SERVICE_DISPLAY_NAME,//显示服务名
    SERVICE_ALL_ACCESS, //服务访问类型
    SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,//服务类型
    SERVICE_AUTO_START, //自动启动服务
    SERVICE_ERROR_IGNORE,//忽略错误
    filename,//启动的文件名
    NULL,//name of load ordering group (载入组名)
    NULL,//标签标识符
    NULL,//相关性数组名
    NULL,//帐户(当前)
    NULL); //密码(当前)

    if (scv == NULL)
    {
        dwErrorCode = GetLastError();
        if(dwErrorCode!=ERROR_SERVICE_EXISTS)
        {
            printf("Failure !\n");
            CloseServiceHandle(scm);
            return ;
        }
        else
        {
            printf("already Exists !\n");
        }
    }
    else
    {
        printf("Success !\n");
        CloseServiceHandle(scv);

    }

    CloseServiceHandle(scm);
    scm = scv = NULL;
}

void OnDelete()
{
    scm=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);//the OpenSCManager function returns handle of the service control manager database
    if (scm!=NULL)
    {
        scv=OpenService(scm,SERVICE_NAME,SERVICE_ALL_ACCESS);//OpenService:Opens an existing service
        if (scv != NULL)
        {
            QueryServiceStatus(scv,&ServiceStatus);
            if (ServiceStatus.dwCurrentState==SERVICE_RUNNING)
            {
                ControlService(scv,SERVICE_CONTROL_STOP,&ServiceStatus);//服务控制程序通过ControlService来发送一个控制请求给运行着的服务。这个函数指定控制值传递给指定服务的HandlerEx 函数。
            }
        DeleteService(scv);
        CloseServiceHandle(scv);
        }
    CloseServiceHandle(scm);
    }
    scm = scv = NULL;
}


void OnStart()
{
    DWORD dwErrorCode;
    //Starting Service
    printf("Starting Service .... ");
    scm = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(scm != NULL)
    {
        scv = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
        if (scv != NULL)
        {
            if(StartService(scv, 0, NULL)==0)
            {
                dwErrorCode = GetLastError();
                if(dwErrorCode == ERROR_SERVICE_ALREADY_RUNNING)
                {
                    printf("already Running !\n");
                    CloseServiceHandle(scv);
                    CloseServiceHandle(scm);
                    return ;
                }
            }
        else
        {
            printf("Pending ... ");
        }

        //wait until the servics started
        while(QueryServiceStatus(scv,&ServiceStatus)!=0)
        {
            if(ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
            {
                Sleep(100);

            }
            else
            {
                break;

            }
        }

        CloseServiceHandle(scv);
        }
        else
        {
            //error to OpenService
            printf("error to OpenService\n");
        }

        CloseServiceHandle(scm);
    }
    else
    {
        //fail to OpenSCManager
    }
    /*
    if(InstallServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        printf("Failure !\n");
    }
    else
    {
        printf("Success !\nDumping Description to Registry...\n");
        RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\NtBoot",
        0,
        KEY_ALL_ACCESS,
        &hkResult);
        RegSetValueEx(hkResult,
        "Description",
        0,
        REG_SZ,
        (unsigned char *)"Driver Booting Service",
        23);

        RegCloseKey(hkResult);
    }

    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    }//
    */

    scm = scv = NULL;
}


void OnStop()
{
    scm = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(scm != NULL)
    {
        scv = OpenService(scm,SERVICE_NAME,SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (scv!=NULL)
        {
            QueryServiceStatus(scv,&ServiceStatus);
            if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
            {
                ControlService(scv,SERVICE_CONTROL_STOP,&ServiceStatus);
            }
            CloseServiceHandle(scv);
        }
        CloseServiceHandle(scm);
    }
    scm = scv = NULL;
}

VOID HandlerEx(DWORD controlCode,   DWORD dwEventType,   LPVOID lpEventData,   LPVOID lpContext)
{
    switch(controlCode)
    {
        case SERVICE_CONTROL_STOP:
            Log("The Server stopped because of SERVICE_CONTROL_STOP");
            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus (hStatus, &ServiceStatus);
            return;

        case SERVICE_CONTROL_SHUTDOWN:
            Log("The Server stopped because of SERVICE_CONTROL_SHUTDOWN.");
            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus (hStatus, &ServiceStatus);
            return;

        case SERVICE_CONTROL_SESSIONCHANGE:
            switch(dwEventType)
            {

                    case 0x7: //WTS_SESSION_LOCK
                        Log("Locking start .");
                        ServiceStatus.dwWin32ExitCode = 0;
                        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                        SetServiceStatus (hStatus, &ServiceStatus);
                    default:
                        break;
            }

        default:
            break;

    }
    SetServiceStatus (hStatus, &ServiceStatus);//SetServiceStatus:Updates the service control manager's status information for the calling service
}
