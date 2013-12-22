// run.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Windows.h"
#include "winuser.h"
#include <Mmsystem.h>
#include "TlHelp32.h"
#include<fstream>
#include<iostream>

using namespace std;

// used for get cpu counting
#include "pdh.h"
#pragma comment(lib, "Pdh.lib") //Pdh.lib  

bool bBack=false;
int WAITE_TIME=600000;
int CHECK_TIME=3000;
int CPUUSEAGE=20;

bool inactive()
{
	LASTINPUTINFO lpi;
	DWORD dwTime = 0;
	lpi.cbSize = sizeof(lpi);
	GetLastInputInfo(&lpi);
	dwTime = ::GetTickCount()-lpi.dwTime;
	if(dwTime > WAITE_TIME)
		return true;
	else
		return false;
}

// run then mine program
bool creatProcess(PROCESS_INFORMATION *pi)
{
	char chPath[301];
	STARTUPINFO si;
	char path[200]= "\\ToRun.bat";

	::GetCurrentDirectory(300,(LPTSTR)chPath);
	strcat(chPath,path);

	ZeroMemory( pi, sizeof(*pi) );
	ZeroMemory( &si, sizeof(si) );

	si.cb = sizeof(si);

	// Start the child process
	if(CreateProcess(chPath, "", NULL, NULL, FALSE, 0, NULL, NULL, &si, pi))
	{
		printf("Created!!!\n");
		return true;
	}
	else
	{
		printf("Created fail!!!\n");
		return false;
	}
}

bool __fastcall KillProcessTree(DWORD myprocID)
{

	PROCESSENTRY32 pe;

	memset(&pe, 0, sizeof(PROCESSENTRY32));
	pe.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnap = :: CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (::Process32First(hSnap, &pe))
	{
		BOOL bContinue = TRUE;

		// kill child processes
		while (bContinue)
		{
			// only kill child processes
			if (pe.th32ParentProcessID == myprocID)
			{
				HANDLE hChildProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

				if (hChildProc)
				{
					::TerminateProcess(hChildProc, 1);
					::CloseHandle(hChildProc);
				}               
			}

			bContinue = ::Process32Next(hSnap, &pe);
		}

		// kill the main process
		HANDLE hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, myprocID);

		if (hProc)
		{
			::TerminateProcess(hProc, 1);
			::CloseHandle(hProc);
		}       
	}
	return true;
}

void runBack()
{
	HWND hwnd;
	if(hwnd=::FindWindow("ConsoleWindowClass",NULL)) //
	{
		::ShowWindow(hwnd,SW_HIDE); //run background
	}
}

void readConfig()
{
	ifstream inf;

    inf.open("Config.txt");
	if(inf)
	{
		inf >> bBack;
		inf >> CHECK_TIME;
		inf >> WAITE_TIME;
		inf >> CPUUSEAGE;
	}
   
	inf.close();
}

// set current dir. Very useful when this program is star with the system.
void setCurrentDir()
{
	char strPath[512];
	string a;
	int j;

	GetModuleFileName(NULL,strPath, 512); // get current file path

	for(j=strlen(strPath);strPath[j]!='\\';j--);
		strPath[j]='\0';

	SetCurrentDirectory(strPath);  // set current working dir
	GetCurrentDirectory(512,strPath);

	return;
}

HQUERY hQuery = NULL;                       //������������  
HCOUNTER hCounter = NULL;                   //���������  
PDH_FMT_COUNTERVALUE DisplayValue;          //������ֵ  
DWORD dwCounterType = 0;                    //����������  

PDH_STATUS initCpuData()
{
	PDH_STATUS status;
	
	status = PdhOpenQuery(NULL, 0, &hQuery);    //�򿪼���������

	if (ERROR_SUCCESS == status)  
	{  
		//PdhAddCounter()������Ӽ�������������Processor�������������(0)��ʾ��1������������ʵ����
		//���������������������������2������������ʵ����(1)��ʾ  
		//% Processor Time��ʾ��������ȡ�ô�����ռ��ʱ�䣩,ע��%�����и��ո�  
		status = PdhAddCounter(hQuery, "\\Processor(0)\\% Processor Time", 0, &hCounter);

		if (ERROR_SUCCESS == status)  
		{  
			// PdhCollectQueryData()�����ռ����������ε���, PdhCollectQueryDataExֻ��һ��
			PdhCollectQueryData(hQuery);        //�״��ռ�����  
		}
	}
	return status;
}

bool getCpuData()
{
	PDH_STATUS status = -1;  
	PdhCollectQueryData(hQuery);    //�ٴ��ռ�����
	status = PdhGetFormattedCounterValue(hCounter,
				PDH_FMT_DOUBLE,   
				&dwCounterType, 
				&DisplayValue);

	return DisplayValue.doubleValue < CPUUSEAGE;
}

int main(int argc, char* argv[])
{
	bool bCreated = false;
	bool bInactive = false;
	bool bUsageLow = true;
	PDH_STATUS cpuStatus = -1;

	PROCESS_INFORMATION pi;
	setCurrentDir();
	
	readConfig();

	if(bBack)
	{
		runBack();
	}

	if(CPUUSEAGE < 100)
	{
		cpuStatus = initCpuData();
	}

    while(1)
    {
        Sleep(CHECK_TIME);

		//get CPU usage
		if(ERROR_SUCCESS == cpuStatus)
		{
			bUsageLow = getCpuData();
		}

		bInactive = inactive();
		if(false == bCreated)
		{
			if(bInactive && bUsageLow)
			{
				bCreated = creatProcess(&pi);
			}
		}
		else
		{
			if(!bInactive)
			{
				
				if(KillProcessTree(pi.dwProcessId))
				{
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
					printf("Stoped!!!\n");
					bCreated = false;
				}
			}
		}

    }
	
	PdhCloseQuery(hQuery);                  //close counting
	return 0;
}
