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

#include "pdh.h"  
#pragma comment(lib, "Pdh.lib") //��ʽ����Pdh.lib  

bool bBack=true;
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

bool creatProcess(PROCESS_INFORMATION *pi)
{
	char chPath[301];

	::GetCurrentDirectory(300,(LPTSTR)chPath);//�õ���ǰĿ¼

	char path[200]= "//ToRun.cmd";

	strcat(chPath,path);
	STARTUPINFO si;
	
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
		::ShowWindow(hwnd,SW_HIDE); //��̨����
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

int main(int argc, char* argv[])
{
	bool bCreated = false;
	bool bInactive = false;
	PROCESS_INFORMATION pi;
	
	
	readConfig();
	if(bBack)
	{
		runBack();
	}

	PDH_STATUS status = -1;  
	HQUERY hQuery = NULL;                       //������������  
	HCOUNTER hCounter = NULL;                   //���������  
	PDH_FMT_COUNTERVALUE DisplayValue;          //������ֵ  
	DWORD dwCounterType = 0;                    //����������  
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
		  printf("CPUռ�� �״��ռ�����  \n");  
		  PdhCollectQueryData(hQuery);        //�״��ռ�����  
	  }
	}


    while(1)
    {
        Sleep(CHECK_TIME);

		PdhCollectQueryData(hQuery);    //�ٴ��ռ�����

		status = PdhGetFormattedCounterValue(hCounter,
					PDH_FMT_DOUBLE,   
					&dwCounterType, 
					&DisplayValue);

		//ȡ���ռ�������ֵ
		//printf("CPUռ�� %.2f\n", DisplayValue.doubleValue);  

		bInactive = inactive();
		if(false == bCreated)
		{
			if(bInactive && DisplayValue.doubleValue < CPUUSEAGE)
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
	
	PdhCloseQuery(hQuery);                  //�رռ���������  
	return 0;
}
