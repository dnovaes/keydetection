
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <iostream>
#include <string.h>
#include <tlhelp32.h>
#include <limits.h>
#include <stdlib.h> //math functions

namespace battlelistAddon{

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Number;
using v8::Boolean;
using v8::Value;
using v8::Persistent;
using v8::Function;
using v8::Handle;
using v8::Array;

struct pkmUnit{
  DWORD_PTR address;
  char name[15];
};

struct pkmBattleList{
  pkmUnit pkm[1];
  int length;
};

struct Coords{
  int x,y,z;
};

typedef struct listInt{
  int value;
  struct listInt *next;
}cel;

struct WorkHk{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  int keyCode;
};

struct WorkPos{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  DWORD_PTR creatureAddr;
  POINT pos;
};

struct Work{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  DWORD_PTR creatureAddr;
  int fAction; //indicates type of action that is gonna occurs in async sygnal: 1 -> send process adress, 2 -> send creature address
};

struct WorkPkm{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  DWORD_PTR creatureAddr;
  Coords coords[2];
};

struct WorkBl{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  int counter;
};

struct WorkFish{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  byte fishStatus;
};

typedef struct commandList{
  char *cmdType, *clickType;
  int pos[3];
  int value;
}CommandList;

typedef struct contentProfile{
  char *fileName;
  CommandList *commands;
  int sizeCommandList;
}ContentProfile;

struct WorkProfile{
  //cannot be const, has to be editable
  ContentProfile *profile;
  uv_work_t request;
  uv_async_t async;
  int cmdPos;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
};

struct NodeCreature{
  DWORD_PTR addr;
  DWORD_PTR *prev;
  DWORD_PTR *next;
  char name[8];
};

//global var
char mutex = 1; //1 = continue
uv_rwlock_t numlock;
BOOL fDebugWhileEvent = FALSE;
BOOL fPause = FALSE;
BOOL fCaveBot = FALSE;
DWORD pid;
DWORD dwThreadID;
DWORD_PTR moduleAddr;
POINT center, sqm;
HANDLE hThread;
HWND g_hwndGame;
RECT pxgClientCoords;
int SCREEN_X = 0;
int SCREEN_Y = 0;
int g_gamePosX = 0;
int g_gamePosY = 0;
int g_gameWidth = 0;
int g_gameHeight = 0;
POINT pkmSlot;

//C function declaration
Coords getPlayerPosC();
DWORD GetProcessThreadID(DWORD pID);
DWORD_PTR dwGetModuleBaseAddress(DWORD pid, TCHAR *szModuleName);
void printThreads(DWORD pid);
int DNcheckCurrPkmLife(int fElixirHeal);
void pause();
void swapPokemon();
void revivePokemon();
void dragItemtoBellow();
// Util Functions
Coords getGameTargetPos(Coords targetPos, Coords playerPos);

//const

/* DIRECTX9 */
//global allocations (with AOB injection)
const DWORD_PTR BASEADDR_CREATURE_GENERATOR = 0x047C0000; //_creatureBase
//code cave with fixed address of the game
const DWORD_PTR BASEADDR_CREATURE_GEN       = 0x00719001; //space in memory to copy and reference _creatureBase
const DWORD_PTR BASEADDR_CREATURE_COUNTER   = 0x00719005;
// base address pkm disappear from window
const DWORD_PTR BASEADDR_DISAPPEAR          = 0x00719021;
const DWORD_PTR BASEADDR_DISAPPEAR_POSX     = 0x00719025;
const DWORD_PTR BASEADDR_DISAPPEAR_POSY     = 0x00719029;
const DWORD_PTR OFFSET_DISAPPEAR_LIFE       = 0x38;
//wanted wild pokemon (Cyber)
const DWORD_PTR BASEADDR_WANTED             = 0x0071902D;
const DWORD_PTR BASEADDR_WANTED_POSX        = 0x00719031;
const DWORD_PTR BASEADDR_WANTED_POSY        = 0x00719035;
const DWORD_PTR OFFSET_WANTED_LIFE          = 0x38;
//Player's pokemon position
const DWORD_PTR BASEADDR_PLAYER_PKM         = 0x007190C2;
const DWORD_PTR BASEADDR_PLAYER_PKM_POSX    = 0x007190C2+0x4;
const DWORD_PTR BASEADDR_PLAYER_PKM_POSY    = 0x007190C2+0x8;
const DWORD_PTR BASEADDR_PLAYER_PKM_NAME    = 0x007190C2+0x12;

//adress for position change (writes into pokemon pos. when something moves in screen)
//const DWORD_PTR INSTR_POSADDR               = 0x1461A4; 
//use moduleAddress as base (4 bytes)
const DWORD_PTR OFFSET_PLAYER_POSX          = 0x393840;
const DWORD_PTR OFFSET_PLAYER_POSY          = 0x393844;
const DWORD_PTR OFFSET_PLAYER_POSZ          = 0x393848; //1 byte is enough here
//use creatureAddress Name as base
const DWORD_PTR OFFSET_PKM_NAME_LENGTH      = 0x24; //byte
const DWORD_PTR OFFSET_PKM_NAME             = 0x28;
const DWORD_PTR OFFSET_PKM_POSX             = 0xC;
const DWORD_PTR OFFSET_PKM_POSY             = 0x10;
const DWORD_PTR OFFSET_PKM_POSZ             = 0x14;
const DWORD_PTR OFFSET_PKM_LIFE             = 0x38;
const DWORD_PTR OFFSET_ENT_DIRECTION        = 0x3C;
const DWORD_PTR OFFSET_PKM_LOOKTYPE         = 0x1C;
//Addresses to current PKM life in slot (DOUBLE)
const DWORD_PTR BASEADDR_CURR_PKM_LIFE      = 0x00393320;
const DWORD_PTR OFFSET_CURR_PKM_LIFE        = 0x3C8;
//pointer to battlelist counter of active creatures
const DWORD_PTR BASEADDR_BLACOUNT           = 0x0038E820;
const DWORD_PTR OFFSET_BLCOUNT_P1           = 0x33C;
const DWORD_PTR OFFSET_BLCOUNT_P2           = 0xA4;
const DWORD_PTR OFFSET_BLCOUNT_P3           = 0x30;
//pointer to fishing status address (byte)
const DWORD_PTR BASEADDR_FISHING            = 0x0039283C;
const DWORD_PTR OFFSET_FISHING_P1           = 0x54;
const DWORD_PTR OFFSET_FISHING_P2           = 0x4;
//pointer to player's pokemon summoned status address
const DWORD_PTR BASEADDR_PLAYER_PKM_SUMMON  = 0X0038F470;
const DWORD_PTR OFFSET_PLAYER_PKM_SUMMON_P1 = 0x104;
const DWORD_PTR OFFSET_PLAYER_PKM_SUMMON_P2 = 0x54;
const DWORD_PTR OFFSET_PLAYER_PKM_SUMMON_P3 = 0x1C;
const DWORD_PTR OFFSET_PLAYER_PKM_SUMMON_P4 = 0x304;
//pointer to container moveset
const DWORD_PTR BASEADDR_MOVESET  = 0x0;
const DWORD_PTR OFFSET_MOVESET_P1 = 0x0;
const DWORD_PTR OFFSET_MOVESET_P2 = 0x0;
const DWORD_PTR OFFSET_MOVESET_P3 = 0x0;
const DWORD_PTR OFFSET_MOVESET_X  = 0x0;
const DWORD_PTR OFFSET_MOVESET_Y  = 0x0;
//Logged in status
const DWORD_PTR BASEADDR_LOGGEDIN = 0x00393360;
//Game Window
const DWORD_PTR BASEADDR_GAMESCREEN = 0x0031900B;
const DWORD_PTR BASEADDR_GAMESCREEN_HEIGHT = 0x00319007;
const DWORD_PTR OFFSET_GAMESCREEN_HEIGHT_WIDTH = 0x4;
const DWORD_PTR OFFSET_GAMESCREEN_X= 0x0;
const DWORD_PTR OFFSET_GAMESCREEN_Y= 0x4;
//Summon status
const DWORD_PTR BASEADDR_SUMMONSTATUS = 0x0031900F; //byte 0/1
//Address containing adress of selected creature (target)
const DWORD_PTR BASEADDR_TARGET_SELECT = 0x393324;

DWORD_PTR dwGetModuleBaseAddress(DWORD pid, TCHAR *szModuleName)
{

  DWORD_PTR dwModuleBaseAddress = 0;
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
  if (hSnapshot != INVALID_HANDLE_VALUE)
  {
    MODULEENTRY32 ModuleEntry32;
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnapshot, &ModuleEntry32))
    {
        do
        {
            //printf(" Module Name: %s\n", ModuleEntry32.szModule);
            if (_tcsicmp(ModuleEntry32.szModule, szModuleName) == 0)
            {
                dwModuleBaseAddress = (DWORD_PTR)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32));
    }
  }
  CloseHandle(hSnapshot);
  return dwModuleBaseAddress;
}

int privileges(){
  HANDLE Token;
  TOKEN_PRIVILEGES tp;
  if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&Token))
  {
      LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
      tp.PrivilegeCount = 1;
      tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
      if (AdjustTokenPrivileges(Token, 0, &tp, sizeof(tp), NULL, NULL)==0){
        return 1; //FAIL
      }else{
        return 0; //SUCCESS
      }
  }
  return 1;
}

void signal_callback_handler(int signum)
{
  //printf("Caught signal %d\n",signum);
  // Cleanup and close up stuff here
  //
  CONTEXT ctx = {0};

  SuspendThread(hThread);
  ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;
  GetThreadContext(hThread, &ctx);

  ctx.Dr0 = 0x00000000;
  ctx.Dr7 = 0x00000000;

  SetThreadContext(hThread, &ctx);
  ResumeThread(hThread);

  DebugActiveProcessStop(pid);

  Sleep(100);

  // Terminate program
  exit(signum);
}

DWORD getProcessId(char *clientName){
  // Create toolhelp snapshot.
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process;
  ZeroMemory(&process, sizeof(process));
  process.dwSize = sizeof(process);
  DWORD pid;

  // Walkthrough all processes.
  if (Process32First(snapshot, &process)){
      do{
          // Compare process.szExeFile based on format of name, i.e., trim file path
          // trim .exe if necessary, etc.
          std::string processName = std::string(process.szExeFile);
          if (processName == "pxgclient_dx9.exe"){
             pid = process.th32ProcessID;
             //printf("Process %d (0x%04X) \n", pid, pid);
             break;
          }
      } while (Process32Next(snapshot, &process));
  }
  CloseHandle(snapshot);
  return pid;
}

//main function
static void getCreatureAddrSummoned(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  HINSTANCE module = GetModuleHandle(NULL);


  pid = 0;
  pid = getProcessId("pxgclient_dx9.exe");
  printf("PID of the pxgclient=%d\n", pid);

  //When program gets SIGINT, it will trigger the function signal_callback_handler to execute
  // SIGINT = CTRL+C
  signal(SIGINT, signal_callback_handler);
  //printf("pid: %d \n", getpid());

  moduleAddr = 0;
  //get pointer to module adress and also prints module name
  moduleAddr = dwGetModuleBaseAddress(pid, "pxgclient_dx9.exe");

  printf("address of module: 0x%X\n", (DWORD)moduleAddr);

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  DWORD_PTR value;
  //DWORD numBytesRead;

  //read pointer stored in the moduleAddress
  //ReadProcessMemory(handle, (LPVOID)(moduleAddr), &value, sizeof(DWORD), NULL);
  //printf("ModuleAddress value (Entry Point): 0x%X\n", value);

  ReadProcessMemory(handle, (LPVOID)(moduleAddr), &value, sizeof(DWORD), NULL);
  printf("ModuleAddress value: 0x%X\n", (DWORD)value);

  CloseHandle(handle); //close handle of process

  //print all threads
  //printThreads(pid);

  //initiate lock to open another thread async right away
  uv_rwlock_init(&numlock);
  //send back information about pid and moduleAddress
  work->fAction = 1;

  //init baton (work) with static values. Required to run the next command (uv_async_send)
  work->async.data = (void*) req;
  uv_async_send(&work->async);

  while(mutex == 0){
    printf("mutex %d ", mutex);
  }

  HANDLE handlep = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
  if(handlep == INVALID_HANDLE_VALUE){
    printf("Failed to open PID %d, error code: %d\n", pid, GetLastError());
  }else{
    printf("handlep value: %p\n", handlep);
  }

  DWORD_PTR entityAddr, lastEntityAddr;//a just spawed creature
  DWORD_PTR entityCounter, tempCounter;//a just spawed creature
  char entityName[12];

  tempCounter = 0;
  lastEntityAddr = 0;

  printf("iniciando deteccao de criaturas (npc, pokemon, player)\n");
  do{
    //BASEADDR_CREATURE_GEN = creature name
    //ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x1C), &entityNumberBl, 1, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(BASEADDR_CREATURE_COUNTER), &entityCounter, 4, NULL);

    if(entityCounter != tempCounter){
      ReadProcessMemory(handlep, (LPDWORD)(BASEADDR_CREATURE_GEN), &entityAddr, 4, NULL);
      entityAddr = entityAddr-0x28;
      ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x28), &entityName, 12, NULL);

      while(lastEntityAddr == entityAddr){
        ReadProcessMemory(handlep, (LPDWORD)(BASEADDR_CREATURE_GEN), &entityAddr, 4, NULL);
        entityAddr = entityAddr-0x28;
        ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x28), &entityName, 12, NULL);
      }
      printf("\n creatureName: %s, creatureCounter: %d, Address: 0x%X \n", entityName, (int)entityCounter, (DWORD)entityAddr);
      tempCounter = entityCounter;
      lastEntityAddr = entityAddr+0x28;

      work->creatureAddr = entityAddr;
      work->fAction = 2;

      uv_async_send(&work->async);

      while(mutex == 0){
        printf("%d", mutex);
      }
    }
  }while(1);

  CloseHandle(handlep); //close handle of process

/*
  printf("\n ---- DEBUG STARTING ----\n");

  //privileges();
  //adress to put a breakpoint
  //DWORD_PTR address = moduleAddr+0x9D82C; // pxgclient
  BOOL fDebugActive = DebugActiveProcess(pid); // PID of target process

  printf("permission for debug: %d\n", fDebugActive);

  // Avoid killing app on exit
  DebugSetProcessKillOnExit(false);

  // get thread ID of the main thread in process
  dwThreadID = GetProcessThreadID(pid);
  printf("Current Main ThreadID: 0x%X\n", dwThreadID);

  // gain access to the thread
  hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadID);


  CONTEXT ctx = {0};
  DWORD_PTR entityAddr = 0x0;//a just spawed pokemon, npc or player

  if(fDebugActive){

    //ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL | THREAD_SET_CONTEXT;
    DEBUG_EVENT dbgEvent;

    printf("\n");
    printf("EXCEPTION_SINGLE_STEP: 0x%X\n", EXCEPTION_SINGLE_STEP);
    printf("EXCEPTION_DEBUG_EVENT: 0x%X\n", EXCEPTION_DEBUG_EVENT);
    printf("\n");

    //BOOL fBreakPoint = TRUE;
    int fBKcount = 0;

    //set the first breakpoint
    SuspendThread(hThread);
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;
    GetThreadContext(hThread, &ctx);

    //ctx.Dr0 = moduleAddr+0xD73C0;
    ctx.Dr0 = moduleAddr+INSTR_POSADDR;
    ctx.Dr7 = 0x00000001;

    SetThreadContext(hThread, &ctx);
    ResumeThread(hThread);

    work->fAction = 2;

    printf("\nDebug context attached %X \n", moduleAddr+INSTR_POSADDR);

    
    while (!fDebugWhileEvent){

      //Get next event and wait
      if (WaitForDebugEvent(&dbgEvent, INFINITE) == 0){
        printf("w");
        break;
      }

      //PRINT CURRENT DEBUG EVENT #end

      switch (dbgEvent.u.Exception.ExceptionRecord.ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
          printf("\n--> Attempt to %s data at address %X\n",
          dbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[0] ? TEXT("write") : TEXT("read"),
          dbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[1]);
          break;
        default:
          break;
      }

      //check if breakpoint on address target was triggered
      if (dbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&(
        dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP
        ||
        dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001E //Single Step WOW
      )){
        printf("dbgEvent raised....0x%X", (DWORD_PTR)dbgEvent.u.Exception.ExceptionRecord.ExceptionAddress);
        if((DWORD_PTR)dbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == (moduleAddr+INSTR_POSADDR)){

HANDLE handlep = OpenProcess(PROCESS_VM_READ, FALSE, pid);
DebugBreakProcess(hThread);

            fBKcount++;
//            SuspendThread(hThread);
            ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;
            GetThreadContext(hThread, &ctx);

            //entityAddr = (DWORD_PTR)ctx.Ecx; // get Ecx
            entityAddr = (DWORD_PTR)ctx.Ebx; // get Ebx

            work->creatureAddr = entityAddr;

            while(mutex == 0){
              printf("waiting for mutex\n");
              printf("%d", mutex);
            }

char entityName[16];
byte entityType;
ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x28), &entityName, 16, NULL);
ReadProcessMemory(handlep, (LPDWORD)(entityAddr), &entityType, 1, NULL);
CloseHandle(handlep); //close handle of process
printf("\nentityName: %s, Type: %d, Address: 0x%X\n", entityName, entityType, entityAddr);
//            uv_async_send(&work->async);
            entityAddr = 0x0;

            ctx.Dr0 = moduleAddr+INSTR_POSADDR+0x6;
            ctx.Dr7 = 0x00000001;

            while(mutex == 0){
              printf("%d", mutex);
            }

            //ctx.EFlags = 0x100; //raises a Single Step Exception

            SetThreadContext(hThread, &ctx);
//            ResumeThread(hThread);
            //fBreakPoint = TRUE;
        }else if((DWORD_PTR)dbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == (moduleAddr+INSTR_POSADDR+0x6)){
            //update breakpoint
DebugBreakProcess(hThread);
//            SuspendThread(hThread);
            ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;
            GetThreadContext(hThread, &ctx);

            ctx.Dr0 = moduleAddr+INSTR_POSADDR;
            ctx.Dr7 = 0x00000001;

            SetThreadContext(hThread, &ctx);
//            ResumeThread(hThread);
        }
      }else if(dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0xC0000005){ //STATUS_ACCESS_VIOLATION
        printf("STATUS_ACCESS_VIOLATION !!!!\n");
        //close debugger
        fDebugWhileEvent = TRUE;
      }
      ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, DBG_CONTINUE);
    }//end while
    

    printf("skip loopwhile\n");

    //remove debugger
    SuspendThread(hThread);
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER | CONTEXT_CONTROL;
    GetThreadContext(hThread, &ctx);

    ctx.Dr0 = 0x00000000;
    ctx.Dr7 = 0x00000000;

    SetThreadContext(hThread, &ctx);
    ResumeThread(hThread);
    DebugActiveProcessStop(pid);
  }else{
    DWORD error = GetLastError();
    printf("GetLastError: %d(0x%X)\n", error, error);
  }

  printf(" ---- DEBUG ENDED ----\n");
  CloseHandle(hThread);
  */
}

DWORD GetProcessThreadID(DWORD dwProcID){
  DWORD dwMainThreadID = 0;
  ULONGLONG ullMinCreateTime = ULLONG_MAX;

  HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap != INVALID_HANDLE_VALUE) {
    THREADENTRY32 th32;
    th32.dwSize = sizeof(THREADENTRY32);
    BOOL bOK = TRUE;
    for (bOK = Thread32First(hThreadSnap, &th32); bOK;
         bOK = Thread32Next(hThreadSnap, &th32)) {
      if (th32.th32OwnerProcessID == dwProcID) {
        //HANDLE hThreadTemp = OpenThread(THREAD_QUERY_INFORMATION,
        HANDLE hThreadTemp = OpenThread(THREAD_ALL_ACCESS,
                                    FALSE, th32.th32ThreadID);
        printf("%d| checking thread ID: 0x%X\n", dwProcID, th32.th32ThreadID);
        if (hThreadTemp) {
          FILETIME afTimes[4] = {0};
          if (GetThreadTimes(hThreadTemp, &afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3])) {
            //ULONGLONG ullTest = MAKEULONGLONG(afTimes[0].dwLowDateTime, afTimes[0].dwHighDateTime);
            ULONGLONG ullTest = (ULONGLONG)afTimes[0].dwHighDateTime << 32 | afTimes[0].dwLowDateTime;
            if (ullTest && ullTest < ullMinCreateTime) {
              ullMinCreateTime = ullTest;
              dwMainThreadID = th32.th32ThreadID; // let it be main... :)
            }
          }
          CloseHandle(hThreadTemp);
        }
      }
    }
#ifndef UNDER_CE
    CloseHandle(hThreadSnap);
#else
    CloseToolhelp32Snapshot(hThreadSnap);
#endif
  }

  if (dwMainThreadID) {
    return dwMainThreadID;
    //PostThreadMessage(dwMainThreadID, WM_QUIT, 0, 0); // close your eyes...
  }
  return 0;
}

void printThreads(DWORD pid){
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (h != INVALID_HANDLE_VALUE) {
    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (Thread32First(h, &te)) {
      do {
        if((te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID)+
        sizeof(te.th32OwnerProcessID))&&(te.th32OwnerProcessID == pid)){
          printf("Process %d(0x%04X) Thread 0x%04x\n",
                 te.th32OwnerProcessID, te.th32OwnerProcessID, te.th32ThreadID);
          //printf("base priority  = %d\n", te.tpBasePri);
          //printf("delta priority = %d\n", te.tpDeltaPri);
        }
        te.dwSize = sizeof(te);
      }while (Thread32Next(h, &te));
    }
    CloseHandle(h);
  }
}


//also sends info process adresses once when process is starting
void sendSygnalCreatureAddr(uv_async_t *handle) {
  uv_rwlock_rdlock(&numlock);
  mutex = 0;
  HANDLE handlec;

  //printf("uv_asvync_t start\n");
  uv_work_t *req = ((uv_work_t*) handle->data);
  Work *work = static_cast<Work*> (req->data);

  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  /*
  const DWORD_PTR OFFSET_PKM_NAME_LENGTH   = 0x24; //byte
  const DWORD_PTR OFFSET_PKM_NAME   = 0x28; //text
  const DWORD_PTR OFFSET_PKM_POSX   = 0xC; //4bytes
  const DWORD_PTR OFFSET_PKM_POSY   = 0x10; //4bytes
  const DWORD_PTR OFFSET_PKM_POSZ   = 0x14; //byte
  const DWORD_PTR OFFSET_PKM_LIFE   = 0x38; //byte
  */

  Local<Object> obj = Object::New(isolate);

  if(work->fAction == 2){
    DWORD_PTR entityAddr = work->creatureAddr;
    DWORD_PTR entityCounterNumber;
    char entityName[10];
    byte entityType, entityLookType;
//    int errorCode;

    handlec = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
    if(handlec == INVALID_HANDLE_VALUE){
      printf("Failed to open PID %d, error code: %d\n", pid, GetLastError());
    }else{
      printf("handlec value: %p\n", handlec);
    }

    ReadProcessMemory(handlec, (LPDWORD)(entityAddr+0x28), &entityName, 10, NULL);
    ReadProcessMemory(handlec, (LPDWORD)(entityAddr), &entityType, 1, NULL);
/*
printf("memory process (entityType): %d\n", ReadProcessMemory(handlec, (LPDWORD)(entityAddr), &entityType, 1, NULL));
errorCode = GetLastError();
printf("lastErrorCode: %d\n", errorCode);
*/
    ReadProcessMemory(handlec, (LPDWORD)(entityAddr+0x28+OFFSET_PKM_LOOKTYPE), &entityLookType, 1, NULL);
    ReadProcessMemory(handlec, (LPDWORD)(BASEADDR_CREATURE_COUNTER), &entityCounterNumber, 4, NULL);
//    printf("\nSendSygnalCreature:\n creatureName: %s, creatureCounter: %d, Address: 0x%X \n", entityName, entityCounterNumber, entityAddr);
    CloseHandle(handlec); //close handle of process

    obj->Set(String::NewFromUtf8(isolate, "addr"), Number::New(isolate, entityAddr));
    obj->Set(String::NewFromUtf8(isolate, "name"), String::NewFromUtf8(isolate, entityName));
    obj->Set(String::NewFromUtf8(isolate, "type"), Number::New(isolate, entityType));
    obj->Set(String::NewFromUtf8(isolate, "lookType"), Number::New(isolate, entityLookType));
    obj->Set(String::NewFromUtf8(isolate, "counterNumber"), Number::New(isolate, entityCounterNumber));
    obj->Set(String::NewFromUtf8(isolate, "fAction"), Number::New(isolate, 2));

  }else{
    //action == 1. Send process info (pid and moduleAddress)
    obj->Set(String::NewFromUtf8(isolate, "moduleAddr"), Number::New(isolate, moduleAddr));
    obj->Set(String::NewFromUtf8(isolate, "pid"), Number::New(isolate, pid));
    obj->Set(String::NewFromUtf8(isolate, "fAction"), Number::New(isolate, 1));
  }

  Handle<Value> argv[] = {obj};
  //execute the callback
  //takes some time to execute for the first time
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  mutex = 1;
  uv_rwlock_rdunlock(&numlock);
}

void printBattleListAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalCreatureAddr);
  //uv_queue_work(uv_default_loop(), &work->request, printBattleList, NULL);
  uv_queue_work(uv_default_loop(), &work->request, getCreatureAddrSummoned, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

void setScreenConfigSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  //Work* work = new Work();
  //work->request.data = work;

  Local<Object> centerObj = args[0]->ToObject();
  Local<Value> x = centerObj->Get(String::NewFromUtf8(isolate, "x"));
  Local<Value> y = centerObj->Get(String::NewFromUtf8(isolate, "y"));
  center.x = x->Int32Value();
  center.y = y->Int32Value();

  Local<Object> sqmObj = args[1]->ToObject();
  x = sqmObj->Get(String::NewFromUtf8(isolate, "length"));
  y = sqmObj->Get(String::NewFromUtf8(isolate, "height"));
  sqm.x = x->Int32Value();
  sqm.y = y->Int32Value();

  //store the callback from JS in the work package to invoke later
  //Local<Function> callback = Local<Function>::Cast(args[2]);
  //work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //uv_queue_work(uv_default_loop(), &work->request, waitForDeath, waitForDeathComplete);

  Local<Number> val = Number::New(isolate, 1);
  args.GetReturnValue().Set(val);
}

//register hotkey for swapPokemon
static void registerHkDragBox(uv_work_t *req){

  WorkHk *work = static_cast<WorkHk*>(req->data);

  //fill the global var screen resolution
  SCREEN_X = GetSystemMetrics(SM_CXSCREEN);
  SCREEN_Y = GetSystemMetrics(SM_CYSCREEN);

  if(RegisterHotKey(NULL, 1, NULL, work->keyCode)){
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0) != 0 ){
      if(msg.message == WM_HOTKEY){
        //UnregisterHotKey(NULL, 1);

        dragItemtoBellow();
        //Communication between threads(uv_work_t and uv_async_t);
        work->async.data = (void*) req;
        //uv_async_send(&work->async);
      }
    }
  }
}
void registerHkDragBoxAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkHk* work = new WorkHk();
  work->request.data = work;

  work->keyCode = args[0]->Int32Value();
  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //uv_async_init(uv_default_loop(), &work->async, sendSygnalHkDragBox);
  uv_queue_work(uv_default_loop(), &work->request, registerHkDragBox, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

//register hotkey f10 while worker thread is running asynchronically
static void registerHKF10(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);

  // 0x79 121 (F10)
  // 0xBB 187 (=)
  if(RegisterHotKey(NULL, 1, NULL, 0xBB)){
    MSG msg = {0};

    printf("This app is Running as PID=%d\n",getpid());

    while (GetMessage(&msg, NULL, 0, 0) != 0 ){
      if(msg.message == WM_HOTKEY){
        fPause = !fPause;
        //UnregisterHotKey(NULL, 1);

        //Communication between threads(uv_work_t and uv_async_t);
        work->async.data = (void*) req;
        uv_async_send(&work->async);
      }
    }
  }
}
void sendSygnalHK(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  uv_work_t *req = ((uv_work_t*) handle->data);
  Work *work = static_cast<Work*> (req->data);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
}
void registerHKF10Async(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalHK);
  uv_queue_work(uv_default_loop(), &work->request, registerHKF10, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}


static void fish(uv_work_t *req){
  WorkFish *work = static_cast<WorkFish*>(req->data);
  INPUT input;
  byte fishStatus = 2;
  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  int i=-1;

  do{
    i++;
    // Set up a generic keyboard event.
    ::ZeroMemory(&input,sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0; // hardware scan code for key
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    // Press the "CTRL" key
    input.ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    input.ki.dwFlags = 0; // 0 for key press

    SendInput(1, &input, sizeof(INPUT));

    // Press the "Z" key
    input.ki.wVk = 0x5A; // virtual-key code for the "Z" key
    input.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &input, sizeof(INPUT));

    Sleep(50);

    // Release the "Z" key
    input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &input, sizeof(INPUT));

    // Release the "CTRL" key
    input.ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &input, sizeof(INPUT));

    Sleep(50);

    //LEFT CLICK
    // left down
    input.type      = INPUT_MOUSE;
    input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
    SendInput(1,&input,sizeof(INPUT));

    Sleep(50);

    // left up
    input.type      = INPUT_MOUSE;
    input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
    SendInput(1,&input,sizeof(INPUT));

    Sleep(400); //wait for 'fish status' update

    if(fPause){
      printf("bot paused battlelist l-899\n");
      pause();
    }

    DWORD address;

    ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_FISHING), &address, sizeof(DWORD), NULL);
    ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_FISHING_P1), &address, sizeof(DWORD), NULL);
    ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_FISHING_P2), &fishStatus, 1, NULL);
    printf("Fish Status: %d\n", fishStatus);

  }while((fishStatus != 3)&&(i<2)); //3 = fishing, 2 = normal
  work->fishStatus = fishStatus;

  CloseHandle(handle);
}

static void fishComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  WorkFish *work = static_cast<WorkFish*>(req->data);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "fishStatus"), Number::New(isolate, work->fishStatus));
  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void fishAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkFish* work = new WorkFish();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  fPause = FALSE;

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, fish, fishComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void readBlCounter(uv_work_t *req){
  WorkBl *work = static_cast<WorkBl*>(req->data);

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  DWORD address;

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_BLACOUNT), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_BLCOUNT_P1), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_BLCOUNT_P2), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_BLCOUNT_P3), &work->counter, 2, NULL);

  CloseHandle(handle);
}

static void readBlCounterComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  WorkBl*work = static_cast<WorkBl*>(req->data);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "blCounter"), Number::New(isolate, work->counter));
  Handle<Value> argv[] = {obj};

  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void readBlCounterAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkBl* work = new WorkBl();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, readBlCounter, readBlCounterComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void attackPkm(uv_work_t *req){
  WorkPkm *work = static_cast<WorkPkm*>(req->data);
  int x,y;

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  //player pos
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSX), &work->coords[0].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSY), &work->coords[0].y, 4, NULL);

  //pkm pos
  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_POSX), &work->coords[1].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_POSY), &work->coords[1].y, 4, NULL);

  CloseHandle(handle);

  printf("checking if position is valid: %d\n", work->coords[1].y);
  if(work->coords[1].y != 65535){

    x = work->coords[1].x - work->coords[0].x;
    y = work->coords[1].y - work->coords[0].y;

    /*
    printf("center: %d %d, sqm %d %d\n", center.x, center.y, sqm.x, sqm.y);
    */
    printf("setcursoPos at: %d, %d\n", center.x+sqm.x*x, center.y+sqm.y*y);
    SetCursorPos(center.x+sqm.x*x, center.y+sqm.y*y);

    Sleep(50);

    INPUT input;
    ::ZeroMemory(&input,sizeof(INPUT));
    input.type      = INPUT_MOUSE;
    input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1,&input,sizeof(INPUT));

    Sleep(50);

    input.type      = INPUT_MOUSE;
    input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
    SendInput(1,&input,sizeof(INPUT));

    Sleep(50);
  }else{
    printf("invalid position\n");
  }
}

static void attackPkmComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  WorkPkm *work = static_cast<WorkPkm*>(req->data);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void attackPkmAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkPkm* work = new WorkPkm();
  work->request.data = work;

  work->creatureAddr = args[0]->Int32Value();

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, attackPkm, attackPkmComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void isPkmNear(uv_work_t *req){
  WorkPkm *work = static_cast<WorkPkm*>(req->data);

  char name[16];

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_NAME), &name, 16, NULL);
  printf("testing if %s is near\n", name);

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSX), &work->coords[0].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSY), &work->coords[0].y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSZ), &work->coords[0].z, 1, NULL);
  printf("player position: %d, %d, %d\n", work->coords[0].x, work->coords[0].y, work->coords[0].z);

  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_POSX), &work->coords[1].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_POSY), &work->coords[1].y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_POSZ), &work->coords[1].z, 1, NULL);
  printf("pokemon position: %d, %d, %d\n", work->coords[1].x, work->coords[1].y, work->coords[1].z);

  CloseHandle(handle);
}

static void isPkmNearComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  WorkPkm *work = static_cast<WorkPkm*>(req->data);
  int x, y;

  Local<Object> obj = Object::New(isolate);
  x = work->coords[1].x - work->coords[0].x;
  y = work->coords[1].y - work->coords[0].y;

  if(
      (abs(x)<8)&&
      (abs(y)<6)&&
      (abs(work->coords[1].z - work->coords[0].z)==0)
  ){
    obj->Set(String::NewFromUtf8(isolate, "isNear"), Number::New(isolate, 1));
    obj->Set(String::NewFromUtf8(isolate, "posx"), Number::New(isolate, x));
    obj->Set(String::NewFromUtf8(isolate, "posy"), Number::New(isolate, y));
  }else{
    obj->Set(String::NewFromUtf8(isolate, "isNear"), Number::New(isolate, 0));
  }
  Local<Number> error = Number::New(isolate, 0);
  Handle<Value> argv[] = {error, obj};

  printf("executing the call back for isPkmNearComplete\n");
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);

  printf("isPkmNearComplete calculations finished!\n");

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void isPkmNearAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkPkm* work = new WorkPkm();
  work->request.data = work;

  DWORD_PTR pkmAddr = args[0]->Int32Value();
  work->creatureAddr = pkmAddr;
//  printf("pkmAddr: %d\n", pkmAddr);

  //store the callback from JS in the work package to invoke later
  if(!args[1]->IsFunction()){
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Arg1 isn't a function")));
    return;
  }
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, isPkmNear, isPkmNearComplete);

  //Local<Number> num = Number::New(isolate, 1);
  args.GetReturnValue().Set(Undefined(isolate));
}

void isPkmNearSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkPkm* work = new WorkPkm();
  int x, y;
  DWORD_PTR pkmAddr = args[0]->Int32Value();

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSX), &work->coords[0].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSY), &work->coords[0].y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSZ), &work->coords[0].z, 1, NULL);
  printf("player position: %d, %d, %d\n", work->coords[0].x, work->coords[0].y, work->coords[0].z);

  ReadProcessMemory(handle, (LPDWORD)(pkmAddr+OFFSET_PKM_POSX), &work->coords[1].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(pkmAddr+OFFSET_PKM_POSY), &work->coords[1].y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(pkmAddr+OFFSET_PKM_POSZ), &work->coords[1].z, 1, NULL);
  printf("pokemon position: %d, %d, %d\n", work->coords[1].x, work->coords[1].y, work->coords[1].z);

  CloseHandle(handle);

  Local<Object> obj = Object::New(isolate);
  x = work->coords[1].x - work->coords[0].x;
  y = work->coords[1].y - work->coords[0].y;

  if(
      (abs(x)<8)&&
      (abs(y)<6)&&
      (abs(work->coords[1].z - work->coords[0].z)==0)
  ){
    obj->Set(String::NewFromUtf8(isolate, "isNear"), Number::New(isolate, 1));
    obj->Set(String::NewFromUtf8(isolate, "posx"), Number::New(isolate, x));
    obj->Set(String::NewFromUtf8(isolate, "posy"), Number::New(isolate, y));
  }else{
    obj->Set(String::NewFromUtf8(isolate, "isNear"), Number::New(isolate, 0));
  }
  Handle<Value> argv[] = {obj};

  args.GetReturnValue().Set(obj);
}

void isPlayerPkmSummonedSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  DWORD address;
  int statusSummoned;

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_PLAYER_PKM_SUMMON), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_PLAYER_PKM_SUMMON_P1), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_PLAYER_PKM_SUMMON_P2), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_PLAYER_PKM_SUMMON_P3), &address, sizeof(DWORD), NULL);
  ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_PLAYER_PKM_SUMMON_P4), &statusSummoned, 4, NULL);

  CloseHandle(handle);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "status"), Number::New(isolate, statusSummoned));
  Handle<Value> argv[] = {obj};

  args.GetReturnValue().Set(obj);
}

void dragItemtoBellow(){
  INPUT input, input2[2];

  ::ZeroMemory(&input, sizeof(INPUT));

  //move mouse
  input.type = INPUT_MOUSE;
  input.mi.dx = (center.x+sqm.x)*65535/SCREEN_X;
  input.mi.dy = center.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(&input, sizeof(INPUT));
  //move mouse
  input.type = INPUT_MOUSE;
  input.mi.dx = (center.x+sqm.x)*65535/SCREEN_X;
  input.mi.dy = (center.y+sqm.y)*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(&input, sizeof(INPUT)); input.type = INPUT_MOUSE; input.mi.time = 0;
   input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  //SetCursorPos(center.x+sqm.x*cProfile->commands[i].pos[0], center.y+sqm.y*cProfile->commands[i].pos[1]);
}


Coords getPlayerPkmPos(HANDLE handle){
  Coords playerPkmPos;
  DWORD_PTR baseaddr;

  baseaddr=0;

  do{
    /*
    LPDWORD is just a typedef for DWORD* and when a Windows SDK function parameter is a "LPsomething" 
    you generally need to pass a pointer to a "something" (except for the LP[C][W]STR string types).
    */
    printf("\nLooking for the PLAYER'S POKEMON\n");
    ReadProcessMemory(handle, (LPDWORD)(BASEADDR_PLAYER_PKM_POSX), &baseaddr, 4, NULL);
    printf("player's pokemon address: %02X\n", baseaddr);

    if(ReadProcessMemory(handle, (LPDWORD)(baseaddr), &playerPkmPos.x, 4, NULL)){
      printf("playerPkmPos.x: %d\n", playerPkmPos.x);
    }else{
      printf("ReadProcessMemory Failed, error code: %d\n", GetLastError());
      /*
      299 (0x12B)
      Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
      */
    }

    ReadProcessMemory(handle, (LPDWORD)(BASEADDR_PLAYER_PKM_POSY), &baseaddr, 4, NULL); 
    ReadProcessMemory(handle, (LPDWORD)(baseaddr), &playerPkmPos.y, 4, NULL);
  }while(playerPkmPos.x > 65534 || playerPkmPos.x <=0);

  return playerPkmPos;
}

//Gets position of the gobal var BASEADDR_WANTED_POSX in ce
Coords getTargetPkmPos(HANDLE handle){
  Coords targetPkmPos;
  DWORD_PTR baseaddr;

  printf("handle inside function, value: %p\n", handle);
  baseaddr = 0;

  do{
    printf("\nLooking for the wanted pokemon\n");
    ReadProcessMemory(handle, (LPDWORD)(BASEADDR_WANTED_POSX), &baseaddr, 4, NULL);
    printf("targetpkmpos address: %02X\n", baseaddr);

    /*
    LPDWORD is just a typedef for DWORD* and when a Windows SDK function parameter is a "LPsomething" 
    you generally need to pass a pointer to a "something" (except for the LP[C][W]STR string types).
    */

    if(ReadProcessMemory(handle, (LPDWORD)(baseaddr), &targetPkmPos.x, 4, NULL)){
      printf("targetPkmPos.x: %d\n", targetPkmPos.x);
    }else{
      printf("ReadProcessMemory Failed, error code: %d\n", GetLastError());
      /*
      299 (0x12B)
      Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
      */
    }

    ReadProcessMemory(handle, (LPDWORD)(BASEADDR_WANTED_POSY), &baseaddr, 4, NULL); 
    ReadProcessMemory(handle, (LPDWORD)(baseaddr), &targetPkmPos.y, 4, NULL);
  }while(targetPkmPos.x > 65534 || targetPkmPos.x <=0);

  return targetPkmPos;
}

void checkSummonPkm(HANDLE handle, int summonObjective){

  //summonObjective
  // 0 = Not Summoned
  // 1 = Summoned

  int pkmSummonStatus = 0;
  INPUT input, input2[2];

  //printf("\nHandle checksummonpkm: %d, %d\n", handle, &handle);
  do{
    ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_SUMMONSTATUS), &pkmSummonStatus, 1, NULL);
    printf("\nsummonStatus: %d\n", pkmSummonStatus);

    if(pkmSummonStatus != summonObjective){
      ::ZeroMemory(&input, sizeof(INPUT));

      //move mouse to pkmSlot position
      input.type = INPUT_MOUSE;
      input.mi.dx = pkmSlot.x*65535/SCREEN_X;
      input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
      input.mi.time = 0;
      input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);

      ::ZeroMemory(input2, 2*sizeof(INPUT));

      input2[0].type = INPUT_MOUSE;
      input2[0].mi.time = 0;
      input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

      input2[1].type = INPUT_MOUSE;
      input2[1].mi.time = 0;
      input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
      SendInput(2, input2, sizeof(INPUT));
    }

    //sometimes sendInput takes some time to execute. This is a work arround to wait for the sendInput
    Sleep(300);

    if(fPause){
      pause();
    }

  }while(pkmSummonStatus<1); 

}

void revivePokemon(){
  INPUT input, input2[2];

  ::ZeroMemory(&input, sizeof(INPUT));

  //move mouse to pkmSlot position
  input.type = INPUT_MOUSE;
  input.mi.dx = pkmSlot.x*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(input2, 2*sizeof(INPUT));
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  // Press the "CTRL" key
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags = 0; // 0 for key press

  //use revive by Hotkey DEL 
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x2E; // virtual-key code for the "DEL" key
  input2[1].ki.dwFlags = 0; // 0 for key press

  SendInput(2, input2,sizeof(INPUT));

  Sleep(200);

  //Release of "CTRL" key
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

  //Release of "DEL" key
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x2E; // virtual-key code for the "DEL" key
  input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(&input, sizeof(INPUT));

  //move mouse to pkmSlot position
  input.type = INPUT_MOUSE;
  input.mi.dx = pkmSlot.x*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));
 
  Sleep(50);
  
  ::ZeroMemory(input2, 2*sizeof(INPUT));

  //left click to use revive on pkm
  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_LEFTUP;

  SendInput(2,input2,sizeof(INPUT));

  Sleep(240);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;

  SendInput(2, input2, sizeof(INPUT));
}

Coords getGameTargetPos(Coords targetPos, Coords playerPos){
  int sqmDiffX, sqmDiffY; 
  Coords targetScreenPos;

  sqmDiffX = targetPos.x - playerPos.x;
  sqmDiffY = targetPos.y - playerPos.y;
  printf("sqmDiff: %d, %d\n", sqmDiffX, sqmDiffY);
  printf("center: %d, %d\n", center.x, center.y);

  if(sqmDiffX>7){
    sqmDiffX = 7;
  }else if(sqmDiffX<-7){
    sqmDiffX = -7;
  }

  if(sqmDiffY>5){
    sqmDiffY = 5;
  }else if(sqmDiffY<-5){
    sqmDiffY = -5;
  }

  targetScreenPos.x = center.x + (sqmDiffX*(sqm.x));
  targetScreenPos.y = center.y + (sqmDiffY*(sqm.y));
  return targetScreenPos;
}

void swapPokemon(){
  INPUT input, input2[2];

  ::ZeroMemory(&input, sizeof(INPUT));
  //move mouse to pkmSlot position
  input.type = INPUT_MOUSE;
  input.mi.dx = pkmSlot.x*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));
 
  Sleep(50);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);
  
  ::ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  input.mi.time = 0;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(100);

  ::ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dx = (pkmSlot.x+20)*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));

  Sleep(100);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_LEFTUP;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.dx = pkmSlot.x*65535/SCREEN_X;
  input2[1].mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

  SendInput(2,input2,sizeof(INPUT));

  Sleep(100);

  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;

  SendInput(2, input2, sizeof(INPUT));
}

void revivePkmSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  //POINT pkmSlot, reviveSlot;
  INPUT input, input2[2], input3[3];

/*
  Local<Object> centerObj = args[0]->ToObject();
  Local<Value> x = centerObj->Get(String::NewFromUtf8(isolate, "x"));
  Local<Value> y = centerObj->Get(String::NewFromUtf8(isolate, "y"));
  pkmSlot.x = x->Int32Value();
  pkmSlot.y = y->Int32Value();

  Local<Object> sqmObj = args[1]->ToObject();
  x = sqmObj->Get(String::NewFromUtf8(isolate, "x"));
  y = sqmObj->Get(String::NewFromUtf8(isolate, "y"));
  reviveSlot.x = x->Int32Value();
  reviveSlot.y = y->Int32Value();
 */
  ::ZeroMemory(input2, 2*sizeof(INPUT));
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  // Press the "CTRL" key
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags = 0; // 0 for key press

  //use revive by Hotkey DEL 
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x2E; // virtual-key code for the "DEL" key
  input2[1].ki.dwFlags = 0; // 0 for key press

  SendInput(2, input2,sizeof(INPUT));

  //Release of "CTRL" key
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

  //Release of "DEL" key
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x2E; // virtual-key code for the "DEL" key
  input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);

  ::ZeroMemory(&input, sizeof(INPUT));

  //move mouse to pkmSlot position
  input.type = INPUT_MOUSE;
  input.mi.dx = pkmSlot.x*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));
 
  Sleep(50);
  
  ::ZeroMemory(input2, 2*sizeof(INPUT));

  //left click to use revive on pkm
  input2[0].type = INPUT_MOUSE;
  input2[0].mi.time = 0;
  input2[0].mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;

  input2[1].type = INPUT_MOUSE;
  input2[1].mi.time = 0;
  input2[1].mi.dwFlags  = MOUSEEVENTF_LEFTUP;

  SendInput(2,input2,sizeof(INPUT));

  Sleep(230);

  ::ZeroMemory(input3, 3*sizeof(INPUT));

  input3[0].type = INPUT_MOUSE;
  input3[0].mi.time = 0;
  input3[0].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;

  input3[1].type = INPUT_MOUSE;
  input3[1].mi.time = 0;
  input3[1].mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;

  input3[2].type = INPUT_MOUSE;
  input3[2].mi.time = 0;
  input3[2].mi.dwFlags  = MOUSEEVENTF_RIGHTUP;

  SendInput(3, input3, sizeof(INPUT));

  /*OLD Revive function

  //move and click revive in revive slot
  input.type = INPUT_MOUSE;
  input.mi.dx = reviveSlot.x*65535/SCREEN_X;
  input.mi.dy = reviveSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));
  //SetCursorPos(reviveSlot.x, reviveSlot.y);

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(40);

  //point to pkmslot and use selected revive
  //sendinput with mousemovement is smoothly. it simulated mouse movements instead of flickering from a position to another
  input.type = INPUT_MOUSE;
  input.mi.dx = pkmSlot.x*65535/SCREEN_X;
  input.mi.dy = pkmSlot.y*65535/SCREEN_Y;
  input.mi.time = 0;
  input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  SendInput(1, &input, sizeof(INPUT));
  //SetCursorPos(pkmSlot.x, pkmSlot.y);

  Sleep(40);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(40);

  //summon pokemon
  SetCursorPos(pkmSlot.x, pkmSlot.y);

  Sleep(125);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(80);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(1,&input,sizeof(INPUT));
  */

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

static void registerHkRevivePkm(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);

  //fill the global var screen resolution
  SCREEN_X = GetSystemMetrics(SM_CXSCREEN);
  SCREEN_Y = GetSystemMetrics(SM_CYSCREEN);

  //0x2E (delete)
  if(RegisterHotKey(NULL, 1, NULL, 0x2E)){
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0) != 0){
      if(msg.message == WM_HOTKEY){

        //printf("0x%X pressed\n", (LONG)msg.lParam >> 16); //vk information. obs: lParam here has type 32 bits
        //UnregisterHotKey(NULL, 1);

        //Communication between threads(uv_work_t and uv_async_t);
        //work->async.data = (void*) req;
        revivePokemon();
        //uv_async_send(&work->async);
      }
    }
  }
}
void sendSygnalHkRevivePkm(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  uv_work_t *req = ((uv_work_t*) handle->data);
  Work *work = static_cast<Work*> (req->data);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
}
void registerHkRevivePkmAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalHkRevivePkm);
  uv_queue_work(uv_default_loop(), &work->request, registerHkRevivePkm, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

//register hotkey for swapPokemon
static void registerHkSwapPkm(uv_work_t *req){
  WorkHk *work = static_cast<WorkHk*>(req->data);

  //fill the global var screen resolution
  SCREEN_X = GetSystemMetrics(SM_CXSCREEN);
  SCREEN_Y = GetSystemMetrics(SM_CYSCREEN);

  if(RegisterHotKey(NULL, 1, NULL, work->keyCode)){
    MSG msg = {0};

    //printf("Running as PID=%d\n",getpid());

    while (GetMessage(&msg, NULL, 0, 0) != 0 ){
      if(msg.message == WM_HOTKEY){
        //UnregisterHotKey(NULL, 1);

        swapPokemon();
        //Communication between threads(uv_work_t and uv_async_t);
        work->async.data = (void*) req;
        //uv_async_send(&work->async);
      }
    }
  }
}
void registerHkSwapPkmAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkHk* work = new WorkHk();
  work->request.data = work;

  work->keyCode = args[0]->Int32Value();
  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //uv_async_init(uv_default_loop(), &work->async, sendSygnalHKSwapPkm);
  uv_queue_work(uv_default_loop(), &work->request, registerHkSwapPkm, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

void PressHkMedicineLoop(){
  INPUT input2[2];

  while(!fPause){

    ::ZeroMemory(input2, 2*sizeof(INPUT));
    //use Medicine by Hotkey ENTER
    input2[0].type = INPUT_KEYBOARD;
    input2[0].ki.wScan = 0; // hardware scan code for key
    input2[0].ki.time = 0;
    input2[0].ki.dwExtraInfo = 0;
    input2[0].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
    input2[0].ki.dwFlags = 0; // 0 for key press

    input2[1].type = INPUT_KEYBOARD;
    input2[1].ki.wScan = 0; // hardware scan code for key
    input2[1].ki.time = 0;
    input2[1].ki.dwExtraInfo = 0;
    input2[1].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
    input2[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, input2, sizeof(INPUT));

    Sleep(100);

    ::ZeroMemory(input2, 2*sizeof(INPUT));

    input2[0].type = INPUT_MOUSE;
    input2[0].mi.time = 0;
    input2[0].mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;

    input2[1].type = INPUT_MOUSE;
    input2[1].mi.time = 0;
    input2[1].mi.dwFlags  = MOUSEEVENTF_LEFTUP;
    SendInput(2, input2, sizeof(INPUT));

    Sleep(300);
  }
}
static void registerHkMedicineLoop(uv_work_t *req){
  WorkHk *work = static_cast<WorkHk*>(req->data);

  //fill the global var screen resolution
  SCREEN_X = GetSystemMetrics(SM_CXSCREEN);
  SCREEN_Y = GetSystemMetrics(SM_CYSCREEN);

  if(RegisterHotKey(NULL, work->keyCode, NULL, work->keyCode)){
    MSG msg = {0};

    //printf("Running as PID=%d\n",getpid());

    while (GetMessage(&msg, NULL, 0, 0) != 0 ){
      if(msg.message == WM_HOTKEY){
        //UnregisterHotKey(NULL, 1);

        fPause = false;
        PressHkMedicineLoop();
        //Communication between threads(uv_work_t and uv_async_t);
        //work->async.data = (void*) req;
        //uv_async_send(&work->async);
      }
    }
  }
}
void registerHkMedicineLoopAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkHk* work = new WorkHk();
  work->request.data = work;

  work->keyCode = args[0]->Int32Value();
  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //uv_async_init(uv_default_loop(), &work->async, sendSygnalHKSwapPkm);
  uv_queue_work(uv_default_loop(), &work->request, registerHkMedicineLoop, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}


void registerPkmSlotSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();

  Local<Object> centerObj = args[0]->ToObject();
  Local<Value> x = centerObj->Get(String::NewFromUtf8(isolate, "x"));
  Local<Value> y = centerObj->Get(String::NewFromUtf8(isolate, "y"));
  pkmSlot.x = x->Int32Value();
  pkmSlot.y = y->Int32Value();

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

//player pos in the client/game (not in user pc screen coordinates)
Coords getPlayerPosC(){
  Coords coords;
  pid = getProcessId("pxgclient_dx9.exe");
  moduleAddr = dwGetModuleBaseAddress(pid, "pxgclient_dx9.exe");

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSX), &coords.x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSY), &coords.y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSZ), &coords.z, 4, NULL);
  //printf("\n\nplayer position: %d, %d, %d\n", coords.x, coords.y, coords.z);

  CloseHandle(handle);
  return coords;
}

void  tossBallToPokemon(Coords targetPos, Coords playerPos){

  //convert GamePos to ScreenPos
  Coords targetScreenPos;
  INPUT input, input2[2];

  targetScreenPos = getGameTargetPos(targetPos, playerPos);

  printf("capturing pokemon on pos: %d, %d (CurrentPos: %d, %d), (Tossing ball to pos: %d, %d)\n\n", targetScreenPos.x, targetScreenPos.y, playerPos.x, playerPos.y, targetPos.x, targetPos.y);

  SetCursorPos(targetScreenPos.x, targetScreenPos.y);

  Sleep(50);

  ::ZeroMemory(input2, 2*sizeof(INPUT));
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  // Press the "CTRL" key
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags = 0; // 0 for key press

  //use revive by Hotkey F11
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x7A; // virtual-key code for the "F11" key
  input2[1].ki.dwFlags = 0; // 0 for key press

  SendInput(2, input2,sizeof(INPUT));

  Sleep(200);

  //Release of "CTRL" key
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

  //Release of "F11" key
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x7A; // virtual-key code for the "F11" key
  input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  Sleep(50);
  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(40);
}
void sendRightClickToGamePos(Coords targetPos, Coords playerPos){
  //convert GamePos to ScreenPos
  Coords targetScreenPos;
  INPUT input;

  targetScreenPos = getGameTargetPos(targetPos, playerPos);

  printf("Sending Right click to screenPos: %d, %d (CurrentPos: %d, %d), (Going to: %d, %d)\n\n", targetScreenPos.x, targetScreenPos.y, playerPos.x, playerPos.y, targetPos.x, targetPos.y);

  SetCursorPos(targetScreenPos.x, targetScreenPos.y);

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(40);
}
void sendClickToGamePos(Coords targetPos, Coords playerPos){
  //convert GamePos to ScreenPos
  Coords targetScreenPos;
  INPUT input;

  targetScreenPos = getGameTargetPos(targetPos, playerPos);

  printf("Sending Left Click to screenPos: %d, %d (CurrentPos: %d, %d), (Going to: %d, %d)\n\n", targetScreenPos.x, targetScreenPos.y, playerPos.x, playerPos.y, targetPos.x, targetPos.y);

  SetCursorPos(targetScreenPos.x, targetScreenPos.y);

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.time = 0;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(40);
}

void pause(){
  //bot will enter in pause mode if fPause is set to pause
  printf("Bot Paused.\n");
  while(fPause){
    Sleep(1500);
  }
  printf("Bot Resumed.\n");
}

//getPlayerPos return position of the player to the client (electron)
static void getPlayerPos(uv_work_t *req){
  WorkPkm *work = static_cast<WorkPkm*>(req->data);

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  work->coords[0] = getPlayerPosC();
  /* 
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSX), &work->coords[0].x, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSY), &work->coords[0].y, 4, NULL);
  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+OFFSET_PLAYER_POSZ), &work->coords[0].z, 4, NULL);
  */
  printf("player position: %d, %d, %d\n", work->coords[0].x, work->coords[0].y, work->coords[0].z);

  CloseHandle(handle);
}
static void getPlayerPosComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  WorkPkm *work = static_cast<WorkPkm*>(req->data);
  int x, y, z;

  Local<Object> obj = Object::New(isolate);
  x = work->coords[0].x;
  y = work->coords[0].y;
  z = work->coords[0].z;

  obj->Set(String::NewFromUtf8(isolate, "posx"), Number::New(isolate, x));
  obj->Set(String::NewFromUtf8(isolate, "posy"), Number::New(isolate, y));
  obj->Set(String::NewFromUtf8(isolate, "posz"), Number::New(isolate, z));

  //prepare error vars
  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}
void getPlayerPosAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkPkm* work = new WorkPkm();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, getPlayerPos, getPlayerPosComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}



static void runProfile(uv_work_t *req){
  WorkProfile *work = static_cast<WorkProfile*>(req->data);
  work->async.data = (void*) req;

  ContentProfile *cProfile = work->profile;
  int contentLength = cProfile->sizeCommandList;
  Coords playerPos, targetPos;
  fCaveBot = TRUE; //flag mark that indicates that caveBot is running
  //reset fPause to unpaused before start this cavebot
  fPause = FALSE; //flag mark that indicates if bot is paused
  int fcheck = 0;
  BYTE fLogged = 0;

  INPUT input2[2];
  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  //printf("-> Reading file '%s'.json:\n", cProfile->fileName);
  //printf("{\n  fileName: '%s',\n  content:[\n", cProfile->fileName);
  while(fCaveBot){
    for( int i = 0; i < contentLength; i++ ){
      if(fPause){
        pause();
      }
      if(!fCaveBot){
        break;
      }
      printf("\n\nRunning command %d\n", i);
      work->cmdPos = i;
      uv_async_send(&work->async);

      if(strcmp(cProfile->commands[i].cmdType, "sleep") == 0){
        //Sleep Function
        printf("Sleeping...%ds\n", cProfile->commands[i].value);
        Sleep(cProfile->commands[i].value);

      }else if(strcmp(cProfile->commands[i].cmdType, "check") == 0){
        //Move Function 
        //printf("pos: [ ");
        /*
        handleLife = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        ReadProcessMemory(handleLife, (LPDWORD)(moduleAddr+BASEADDR_CURR_PKM_LIFE), &baseCurrPkmLife, 4, NULL);
        ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE), &currPkmLife, 8, NULL);
        ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE+0x8), &currPkmMaxLife, 8, NULL);
        percent = (currPkmLife/currPkmMaxLife)*100;
        printf("\ncurr pkm life: %d, MAX life: %d\n", (int)currPkmLife, (int)currPkmMaxLife);
        printf("current pokemon life in percentual: %d \%\n", percent);
        printf("baseCurrPkmLife: 0x%X\n", baseCurrPkmLife);
        printf("baseCurrPkmLife+3C8: 0x%X\n", baseCurrPkmLife+OFFSET_CURR_PKM_LIFE);
        */
        fcheck = DNcheckCurrPkmLife(0);
        playerPos = getPlayerPosC();
        printf("player position: %d, %d, %d\n", playerPos.x, playerPos.y, playerPos.z);
        if(
          (abs(playerPos.x - cProfile->commands[i].pos[0])<8)&&
          (abs(playerPos.y - cProfile->commands[i].pos[1])<6)&&
          (abs(playerPos.z - cProfile->commands[i].pos[2])==0)
        ){
          printf("coords is close, time to go!\n");
          targetPos.x = cProfile->commands[i].pos[0];
          targetPos.y = cProfile->commands[i].pos[1];
        }else{
          printf("TargetPos: %d, %d\n", cProfile->commands[i].pos[0], cProfile->commands[i].pos[1]);
          printf("too far away! [%d] Restarting Route. Pls go to the first position of the ROUTE\n", i);
          i=-1;
          Sleep(5000);
        }
        while((fCaveBot)&&
            (((abs(playerPos.x - targetPos.x))>0)||
              ((abs(playerPos.y - targetPos.y))>0))
        ){
          printf("ok im inside of the loop time to send click to game. ");
          printf("%d, %d\n", abs(playerPos.x - targetPos.x), abs(playerPos.y - targetPos.y));
          sendClickToGamePos(targetPos, playerPos);

          //Sleep between each "check" movement command
          Sleep(1000);

          playerPos = getPlayerPosC();

          if(fPause){
            pause();
          }

          if(!fCaveBot){
            break;
          }
        }
        /* 
        for(unsigned int j=0; j < 3; j++ ){
          printf("%d", cProfile->commands[i].pos[j]);
          if(j<2){
            printf(",");
          }
        }
        printf(" ]}\n");
        */
      }else if(strcmp(cProfile->commands[i].cmdType, "mouseclick") == 0){

        printf("Running 'mouseclick' function\n");

        ::ZeroMemory(input2, 2*sizeof(INPUT));
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        // Press the "CTRL" key
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags = 0; // 0 for key press

        //Hotkey ]
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xDC; // virtual-key code for the "]" key
        input2[1].ki.dwFlags = 0; // 0 for key press

        SendInput(2, input2,sizeof(INPUT));

        //Release of "CTRL" key
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

        //Release of "]" key
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xDC; // virtual-key code for the "]" key
        input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        SetCursorPos(center.x+sqm.x*cProfile->commands[i].pos[0], center.y+sqm.y*cProfile->commands[i].pos[1]);

        Sleep(500);
        
        ::ZeroMemory(input2, 2*sizeof(INPUT));

        //left click to use item on hotkey
        input2[0].type = INPUT_MOUSE;
        input2[0].mi.time = 0;
        input2[0].mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;

        input2[1].type = INPUT_MOUSE;
        input2[1].mi.time = 0;
        input2[1].mi.dwFlags  = MOUSEEVENTF_LEFTUP;

        SendInput(2, input2, sizeof(INPUT));

        Sleep(500);
      }else if(strcmp(cProfile->commands[i].cmdType, "hotkey") == 0){
        //cProfile->commands[i].value contains the hotkey keyCode
        
        ::ZeroMemory(input2, 2*sizeof(INPUT));

        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = cProfile->commands[i].value; //virtual keycode
        input2[0].ki.dwFlags  = 0;

        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = cProfile->commands[i].value; //virtual keycode
        input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        Sleep(700);
      }else if(strcmp(cProfile->commands[i].cmdType, "login") == 0){
        printf("Login\n");
        //Confirm character and enters the game with "ENTER"
        ::ZeroMemory(input2, 2*sizeof(INPUT));

        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
        input2[0].ki.dwFlags = 0; // 0 for key press

        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
        input2[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        fLogged = 0;

        ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_LOGGEDIN), &fLogged, 1, NULL);
        while(!fLogged){
          ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_LOGGEDIN), &fLogged, 1, NULL);
          printf("Char loggado? => %d\n", fLogged);
          Sleep(1500);
        }

      }else if(strcmp(cProfile->commands[i].cmdType, "logoff") == 0){
        printf("Logout\n");
        //Log out
        ::ZeroMemory(input2, 2*sizeof(INPUT));
        //Release of "CTRL" key
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags  = 0;

        //Release of "q" key
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0x51; // virtual-key code for the "q" key
        input2[1].ki.dwFlags  = 0;

        SendInput(2, input2, sizeof(INPUT));

        Sleep(500);

        //Release of "CTRL" key
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

        //Release of "q" key
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0x51; // virtual-key code for the "q" key
        input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        Sleep(500);

        //Confirm act of logging out
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
        input2[0].ki.dwFlags = 0; // 0 for key press

        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
        input2[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        //Average time to logout from computers... +-2 Seconds
        Sleep(1500);
      }else if(strcmp(cProfile->commands[i].cmdType, "summonPkm") == 0){
        checkSummonPkm(handle, 1);
      }else if(strcmp(cProfile->commands[i].cmdType, "backPkm") == 0){
        checkSummonPkm(handle, 0);
      }
    }
  }
  //printf("  ]\n}\n");
  CloseHandle(handle);
  printf("Profile finished Async\n");
}
static void runProfileComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  WorkProfile *work = static_cast<WorkProfile*>(req->data);

  //prepare error vars
  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}
void sendSygnalProfile(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  uv_work_t *req = ((uv_work_t*) handle->data);
  WorkProfile *work = static_cast<WorkProfile*> (req->data);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "cmdPos"), Number::New(isolate, work->cmdPos));

  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
}
void runProfileAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  WorkProfile* work = new WorkProfile();
  work->request.data = work;

  //Mapping JavaScript object to C++ class
  Local<Object> obj = args[0]->ToObject();

  Local<Value> fileNameValue = obj->Get(String::NewFromUtf8(isolate, "fileName"));
  v8::String::Utf8Value filename_utfValue(fileNameValue);

  Local<Array> contentArray = Local<Array>::Cast(
      obj->Get(
        String::NewFromUtf8(isolate, "content")
      )
  );
  int contentLength = contentArray->Length();

  //malloc ContentProfile
  ContentProfile *cProfile = (ContentProfile*)malloc(sizeof(struct contentProfile));
  //ContentProfile *cProfile = (ContentProfile*)malloc(contentLength*sizeof(struct commandList)+sizeof(struct contentProfile*));
  work->profile = cProfile;

  cProfile->fileName = (char*)malloc(filename_utfValue.length()+1);
  strcpy(cProfile->fileName, std::string(*filename_utfValue, filename_utfValue.length()).c_str());

  cProfile->sizeCommandList = contentLength;

  //malloc CommandList commands*
  cProfile->commands = (CommandList*)malloc(contentLength*sizeof(struct commandList));
  
  Local<Object> arrObj;
  Local<Value> cmdTypeValue, value, clickTypeValue;
  Local<Array> posArray;

  std::string cmdType, clickType;
  for( int i = 0; i < contentLength; i++ ){
    arrObj = Local<Object>::Cast(contentArray->Get(i));

    //read cmdType
    cmdTypeValue = arrObj->Get(String::NewFromUtf8(isolate, "cmdType"));
    v8::String::Utf8Value cmdType_utfValue(cmdTypeValue);
    cmdType = std::string(*cmdType_utfValue, cmdType_utfValue.length()).c_str();
    //malloc cmdType char*
    cProfile->commands[i].cmdType = (char*)malloc(strlen(cmdType.c_str())+1);
    strcpy(cProfile->commands[i].cmdType, cmdType.c_str());

    //check what command you have and execute the command
    if(strcmp(cProfile->commands[i].cmdType, "sleep") == 0){
      value = arrObj->Get(String::NewFromUtf8(isolate, "value"));
      cProfile->commands[i].value = value->Int32Value();
    }else if(strcmp(cProfile->commands[i].cmdType, "check") == 0){
      posArray = Local<Array>::Cast(
        arrObj->Get(
          String::NewFromUtf8(isolate, "pos")
        )
      );

      for(unsigned int j=0; j < posArray->Length(); j++ ){
        value = Local<Value>::Cast(posArray->Get(j));
        cProfile->commands[i].pos[j] = value->Int32Value();
      }
    }else if(strcmp(cProfile->commands[i].cmdType, "mouseclick") == 0){
      //read positions to cProfile struct
      posArray = Local<Array>::Cast(
        arrObj->Get(
          String::NewFromUtf8(isolate, "pos")
        )
      );

      for(unsigned int j=0; j < posArray->Length(); j++ ){
        value = Local<Value>::Cast(posArray->Get(j));
        cProfile->commands[i].pos[j] = value->Int32Value();
      }

      //read clickType (left or right) to cProfile struct
      clickTypeValue = arrObj->Get(String::NewFromUtf8(isolate, "clickType"));
      v8::String::Utf8Value clickType_utfValue(clickTypeValue);
      clickType = std::string(*clickType_utfValue, clickType_utfValue.length()).c_str();
      //malloc clickType char*
      cProfile->commands[i].clickType = (char*)malloc(strlen(clickType.c_str())+1);
      strcpy(cProfile->commands[i].clickType, clickType.c_str());

    }else if(strcmp(cProfile->commands[i].cmdType, "hotkey") == 0){
      value = arrObj->Get(String::NewFromUtf8(isolate, "value"));
      cProfile->commands[i].value = value->Int32Value();
    }

  }

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalProfile);
  uv_queue_work(uv_default_loop(), &work->request, runProfile, runProfileComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}


void sendSygnalCyber(uv_async_t *handle) {
  uv_work_t *req = ((uv_work_t*) handle->data);
  Work *work = static_cast<Work*> (req->data);

  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  Local<Object> obj = Object::New(isolate);

  obj->Set(String::NewFromUtf8(isolate, "countdown"), Number::New(isolate, 1));

  Handle<Value> argv[] = {obj};
  //execute the callback
  //takes some time to execute for the first time
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
}
static void runCyberScriptComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  Work *work = static_cast<Work*>(req->data);

  //prepare error vars
  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}
static void runCyberScript(uv_work_t *req){
  Work *work = static_cast<Work*>(req->data);
  work->async.data = (void*) req;

  BYTE fLogged = 0;
  int i;
  INPUT input, input2[2];
  DWORD_PTR baseaddr;
  Coords corpsePkmPos, targetPkmPos, playerPos, targetPos;
  BYTE prevTargetPkmLife, curTargetPkmLife;
  int hks[10]; //f1 - f10

  //flag the shows condition of running a profile
  //condition necessary to run some functions and being able to stop anytime by the client
  fCaveBot = TRUE;

  hks[0] = 112; //F1
  for(i=1;i<=9;i++){
    hks[i]=hks[i-1]+1;
  }

  //Confirm character and enters the game with "ENTER"
  ::ZeroMemory(input2, 2*sizeof(INPUT));

  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
  input2[0].ki.dwFlags = 0; // 0 for key press

  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
  input2[1].ki.dwFlags = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_LOGGEDIN), &fLogged, 1, NULL);
  while(!fLogged){
    ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_LOGGEDIN), &fLogged, 1, NULL);
    printf("Char loggado? => %d\n", fLogged);
    Sleep(1500);
  }


  //Check and Summon Pokemon
  //init var summon status.  Obs: it has 300 sleep delay after finishing function
  checkSummonPkm(handle, 1);

  //Check and Target Pokemon
  //get position of the wanted pokemon to make target
  do{
    printf("Trying to select the target pkm...\n");
    targetPkmPos = getTargetPkmPos(handle);

    printf("Pkm target found. Pos x: %d, y: %d\n", targetPkmPos.x, targetPkmPos.y);

    playerPos = getPlayerPosC();
    sendRightClickToGamePos(targetPkmPos, playerPos);

    Sleep(150);

    //check again if pkm is summoned
    checkSummonPkm(handle, 1);

    //check if target is selected
    ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_TARGET_SELECT), &baseaddr, 4, NULL);
  }while((baseaddr == 0)&&(fCaveBot));
  printf("Target Pkm selected.\n");


  //Use Pokemon Skills (F9>>F1)
  i=9; //F10
  prevTargetPkmLife = 0;
  printf("Using skills\n");
  do{
    /*if(i==3){ //F4
      i=2;  //skip F4 and goes to F3
    }*/
    //press from F9-F1 (except F4 for Shiny Rhydon)
    input2[0].type = INPUT_KEYBOARD;
    input2[0].ki.wScan = 0; // hardware scan code for key
    input2[0].ki.time = 0;
    input2[0].ki.dwExtraInfo = 0;
    input2[0].ki.wVk = hks[i];
    input2[0].ki.dwFlags = 0; // 0 for key press

    input2[1].type = INPUT_KEYBOARD;
    input2[1].ki.wScan = 0; // hardware scan code for key
    input2[1].ki.time = 0;
    input2[1].ki.dwExtraInfo = 0;
    input2[1].ki.wVk = hks[i];
    input2[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input2, sizeof(INPUT));

    Sleep(100);

    input2[0].type = INPUT_KEYBOARD;
    input2[0].ki.wScan = 0; // hardware scan code for key
    input2[0].ki.time = 0;
    input2[0].ki.dwExtraInfo = 0;
    input2[0].ki.wVk = hks[i];
    input2[0].ki.dwFlags = 0; // 0 for key press

    input2[1].type = INPUT_KEYBOARD;
    input2[1].ki.wScan = 0; // hardware scan code for key
    input2[1].ki.time = 0;
    input2[1].ki.dwExtraInfo = 0;
    input2[1].ki.wVk = hks[i];
    input2[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input2, sizeof(INPUT));

    i--;
    if(i==-1){
      i=9;
    }

    //Check if pokemon is alive, if so, repeat skills until wild pokemon get killed
    ReadProcessMemory(handle, (LPDWORD)(BASEADDR_WANTED), &baseaddr, 4, NULL);
    ReadProcessMemory(handle, (LPDWORD)(baseaddr+OFFSET_WANTED_LIFE), &curTargetPkmLife, 1, NULL);
    printf("current target pkm life: %d\n", curTargetPkmLife);

    if(curTargetPkmLife == prevTargetPkmLife){
      //wait a little more before using next skill, maybe the oponnent is temporary invulnerable
      Sleep(1000);
    }
    prevTargetPkmLife = curTargetPkmLife;

    //Use Revive
    //revivePokemon();

    //use Heal Elixir
    //check if player pokemon life is low, if so, uses elixir of healing
    DNcheckCurrPkmLife(1);

    //check again if pkm is summoned
    checkSummonPkm(handle, 1);
    Sleep(200);

    if(fPause){
      pause();
    }
  }while((curTargetPkmLife>0)&&(fCaveBot));

  //target is dead, restart the countdown:
  //send notification to the interface to restart the countdown
  uv_async_send(&work->async);

  //pull back pokemon into the pokeball
  checkSummonPkm(handle, 0);

  //Take loot of the wild Pokemon
  ReadProcessMemory(handle, (LPDWORD)(BASEADDR_WANTED_POSX), &baseaddr, 4, NULL); 
  ReadProcessMemory(handle, (LPDWORD)(baseaddr), &corpsePkmPos.x, 4, NULL);

  ReadProcessMemory(handle, (LPDWORD)(BASEADDR_WANTED_POSY), &baseaddr, 4, NULL); 
  ReadProcessMemory(handle, (LPDWORD)(baseaddr), &corpsePkmPos.y, 4, NULL);

  printf("corpse pkm:  x %d, y %d\n", corpsePkmPos.x, corpsePkmPos.y);

  playerPos = getPlayerPosC();
  sendRightClickToGamePos(corpsePkmPos, playerPos);

  Sleep(3000);

  //Toss a ball on the wild pokemon
  playerPos = getPlayerPosC();
  tossBallToPokemon(corpsePkmPos, playerPos);

  Sleep(500);

  //Take pokemon out of the pokeball
  checkSummonPkm(handle, 1);

  //Move player back to certain position
  printf("\nSend click movement to specific position [C.O Cyber]\n");
  playerPos = getPlayerPosC();
  targetPos.x = 4289;
  targetPos.y = 3615;
  sendClickToGamePos(targetPos, playerPos); //leftclick
  Sleep(3000);

  //Wait for cooldown to restore
  //Sleep(45000); //Togetic
  Sleep(50000); //C.O


  //Log out
  //Release of "CTRL" key
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags  = 0;

  //Release of "q" key
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x51; // virtual-key code for the "q" key
  input2[1].ki.dwFlags  = 0;

  SendInput(2, input2, sizeof(INPUT));

  Sleep(500);

  //Release of "CTRL" key
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
  input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

  //Release of "q" key
  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0x51; // virtual-key code for the "q" key
  input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  Sleep(500);

  //Confirm act of logging out
  input2[0].type = INPUT_KEYBOARD;
  input2[0].ki.wScan = 0; // hardware scan code for key
  input2[0].ki.time = 0;
  input2[0].ki.dwExtraInfo = 0;
  input2[0].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
  input2[0].ki.dwFlags = 0; // 0 for key press

  input2[1].type = INPUT_KEYBOARD;
  input2[1].ki.wScan = 0; // hardware scan code for key
  input2[1].ki.time = 0;
  input2[1].ki.dwExtraInfo = 0;
  input2[1].ki.wVk = 0xD; // virtual-key code for the "ENTER" key
  input2[1].ki.dwFlags = KEYEVENTF_KEYUP;

  SendInput(2, input2, sizeof(INPUT));

  CloseHandle(handle);
}
void runCyberScriptAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalCyber);
  uv_queue_work(uv_default_loop(), &work->request, runCyberScript, runCyberScriptComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}

//stop actual caveBot profile
void stopProfileSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  fCaveBot = FALSE;
  //remove pause in case of bot paused. Force unpause to finish bot
  fPause = FALSE;
  //printf("fCaveBot: %d\n", fCaveBot);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

void sendKey(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  INPUT input;

  Sleep(1000);

  ::ZeroMemory(&input,sizeof(INPUT));
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = 0; // hardware scan code for key
  input.ki.time = 0;
  input.ki.dwExtraInfo = 0;

  // Press the "END" key
  input.ki.wVk = 0x23;
  input.ki.dwFlags = 0; // 0 for key press

  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  // Release the "END" key
  input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
  SendInput(1, &input, sizeof(INPUT));

  Sleep(50);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

//Decision Node
int DNcheckCurrPkmLife(int fElixirHeal){
  DWORD baseCurrPkmLife;
  DOUBLE currPkmLife, currPkmMaxLife;
  int percent;
  HANDLE handleLife;
  INPUT input2[2];
  //Coords targetPkmPos;
  Coords playerPos, playerPkmPos;

  handleLife = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  currPkmLife = 0;
  currPkmMaxLife = 0;

  if(handleLife != NULL){
    //printf("process openned successfully\n");

    ReadProcessMemory(handleLife, (LPDWORD)(moduleAddr+BASEADDR_CURR_PKM_LIFE), &baseCurrPkmLife, 4, NULL);
    ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE), &currPkmLife, 8, NULL);
    ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE+0x8), &currPkmMaxLife, 8, NULL);

    percent = (currPkmLife/currPkmMaxLife)*100;
    printf("\nplayer pokemon life percent: %d\n\n", percent);

    while((percent<40)&&(fCaveBot)){ 
      //Checking HP part
      ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE), &currPkmLife, 8, NULL);
      ReadProcessMemory(handleLife, (LPDWORD)(baseCurrPkmLife+OFFSET_CURR_PKM_LIFE+0x8), &currPkmMaxLife, 8, NULL);

      percent = (currPkmLife/currPkmMaxLife)*100;
      printf("player pokemon life percent: %d\n", percent);
      /*printf("\ncurr pkm life: %d, MAX life: %d\n", (int)currPkmLife, (int)currPkmMaxLife);
      printf("baseCurrPkmLife: 0x%X\n", baseCurrPkmLife);
      printf("baseCurrPkmLife+3C8: 0x%X\n", baseCurrPkmLife+OFFSET_CURR_PKM_LIFE);*/

      //Heal part:
      if(fElixirHeal){
        printf("Using Heal Elixir\n");

        playerPos = getPlayerPosC();
        //targetPkmPos = getTargetPkmPos(handleLife);
        playerPkmPos = getPlayerPkmPos(handleLife);

        printf("playerPos : %d, %d\n", playerPos.x, playerPos.y);
        //printf("targetPkmPos: %d, %d\n", targetPkmPos.x, targetPkmPos.y);
        printf("playerPkmPos : %d, %d\n", playerPkmPos.x, playerPkmPos.y);

        ::ZeroMemory(input2, 2*sizeof(INPUT));
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        // Press the "CTRL" key
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags = 0; // 0 for key press

        // Press the 'END' key
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xDD; // virtual-key code for the "[" key
        input2[1].ki.dwFlags = 0; // 0 for key press

        SendInput(2, input2,sizeof(INPUT));

        Sleep(200);

        //Release of "CTRL" key
        input2[0].type = INPUT_KEYBOARD;
        input2[0].ki.wScan = 0; // hardware scan code for key
        input2[0].ki.time = 0;
        input2[0].ki.dwExtraInfo = 0;
        input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
        input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

        //Release of "END" key
        input2[1].type = INPUT_KEYBOARD;
        input2[1].ki.wScan = 0; // hardware scan code for key
        input2[1].ki.time = 0;
        input2[1].ki.dwExtraInfo = 0;
        input2[1].ki.wVk = 0xDD; // virtual-key code for the "[" key
        input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

        SendInput(2, input2, sizeof(INPUT));

        Sleep(200);

        //arguments: target, playerPos
        sendClickToGamePos(playerPkmPos, playerPos); //leftclick

        Sleep(200);

      }else{
        //wait for y-regeneration to heal the pokemon
        Sleep(5000);
      }

      //In every loop command, must have this command in case of user wanting to puase the bot
      //same applies to verification of fCaveBot (stopping the profile)
      if(fPause){
        pause();
      }
    }
    CloseHandle(handleLife);
  }else{
    printf("Couldn\'t open process for reading\n");
  }
  return 1;
}

void mapUnownMovementSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  DWORD unownAddress;
  HANDLE handle;

  Local<Object> centerObj = args[0]->ToObject();
  Local<Value> addr = centerObj->Get(String::NewFromUtf8(isolate, "address"));
  unownAddress = addr->Int32Value();
  //pkmSlot.x = x->Int32Value();
  //pkmSlot.y = y->Int32Value();
  printf("unownAddress: 0x%d\n", (int)unownAddress);
  printf("unownAddress: 0x%X\n", unownAddress);

  handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  int i=0, oldPosy=0, newPosy;

  while(i<25){ 
    ReadProcessMemory(handle, (LPDWORD)(unownAddress+OFFSET_PKM_POSY), &newPosy, 4, NULL);
    if(oldPosy==0){
      oldPosy = newPosy;
    }else if(oldPosy!=newPosy){
      if(newPosy>oldPosy){
        printf("newPosy: %d ", newPosy);
        printf("Down\n");
      }else{
        printf("newPosy: %d ", newPosy);
        printf("Up\n");
      }
      oldPosy = newPosy;
      i++;
    }
    if(newPosy==65535){
      //break
      i=25;
    }
  }
  printf("Unown trace finished.\n");

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "pos"), Number::New(isolate, 8));
  Handle<Value> argv[] = {obj};

  args.GetReturnValue().Set(obj);

  //local<number> val = number::new(isolate, 1);
  //handle<value> argv[] = {val};
  //args.GetReturnValue().Set(val);

  /*
  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "fishStatus"), Number::New(isolate, work->fishStatus));
  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
  */
}

void mapIllusionMovementSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  DWORD entAddress;
  HANDLE handle;

  Local<Object> centerObj = args[0]->ToObject();
  Local<Value> addr = centerObj->Get(String::NewFromUtf8(isolate, "address"));
  entAddress = addr->Int32Value();
  //pkmSlot.x = x->Int32Value();
  //pkmSlot.y = y->Int32Value();
  printf("entAddress: 0x%d\n", (int)entAddress);
  printf("entAddress: 0x%X\n", entAddress);

  handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  int fMapping=1, oldDirection=-1, newDirection;

  while(fMapping!=0){ 
    ReadProcessMemory(handle, (LPDWORD)(entAddress+OFFSET_ENT_DIRECTION), &newDirection, 4, NULL);
    if(oldDirection==-1){
      oldDirection = newDirection;
    }else if(oldDirection != newDirection){
      printf("%d ", newDirection);
      switch(newDirection){
        case 0:
          printf("Up\n");
          break;
        case 1:
          printf("Right\n");
          break;
        case 2:
          printf("Down\n");
          break;
        case 3:
          printf("Left\n");
          break;
      }
      oldDirection = newDirection;
    }
  }
  printf("Mapping Illusion finished.\n");

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "pos"), Number::New(isolate, 8));
  Handle<Value> argv[] = {obj};

  args.GetReturnValue().Set(obj);
}


BOOL CALLBACK EnumWindowsProcMy(HWND hwnd,LPARAM lParam)
{
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd,&lpdwProcessId);
    if(lpdwProcessId==lParam)
    {
        g_hwndGame = hwnd;
        return FALSE;
    }
    return TRUE;
}


static void getScreenGamePosComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  Work *work = static_cast<Work*>(req->data);

  //prepare return vars
  //Local<Number> val = Number::New(isolate, 1);
  //Handle<Value> argv[] = {val};
  //execute the callback
  //Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, g_gamePosX));
  obj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, g_gamePosY));
  obj->Set(String::NewFromUtf8(isolate, "w"), Number::New(isolate, g_gameWidth));
  obj->Set(String::NewFromUtf8(isolate, "h"), Number::New(isolate, g_gameHeight));

  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}
static void getScreenGamePos(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  work->async.data = (void*) req;

  HANDLE handle;
  RECT rect; //instead of *rect (pointer to nowhere when only declared)
  int addressGameScreen, initialGameScreenX, initialGameScreenY;

  //printf("\n-------------------\ngetScreenGamePos:\nSearching for hwnd of the process %d\n", pid); 
  EnumWindows(EnumWindowsProcMy, pid);

  //g_hwndGame = FindWindowA(NULL, _T("PXG Client"));
  if(g_hwndGame){
    //printf("Found hWnd 0x%p\n", g_hwndGame);

    //fast explanation here: getwindowrect asks for a LPRECT which is equivalenty a pointer to RECT. 
    //so tdlr, it needs an adress to point and initialize the variable with the result

    do{
      //GetWindowRect(g_hwndGame, &rect)
      if(GetClientRect(g_hwndGame, &rect)){ //working great with MapWindowPoints
        //printf("\nCoodinates (GetWindowRect): top %d, left %d, bottom %d, right %d\n", rect.top, rect.left, rect.bottom, rect.right);

        MapWindowPoints(g_hwndGame, nullptr, reinterpret_cast<POINT*>(&rect), 2);
        //printf("\nConverted (MapWindowPoints):\nleft(x) %d, top(y) %d\n\n", rect.left, rect.top);
      }else{
        printf("GetClientRect Failed, error code: %d\n", GetLastError());
      }

      if(rect.left < 0 || rect.top <0){
        //get and fill the global var screen resolution
        SCREEN_X = GetSystemMetrics(SM_CXSCREEN);
        SCREEN_Y = GetSystemMetrics(SM_CYSCREEN);
        printf("\nMoving GameWindown to pos (0,0) with width: %d and height: %d\n", SCREEN_X/2, SCREEN_Y);
        Sleep(500);
        MoveWindow(g_hwndGame, 0, 0, SCREEN_X/2, SCREEN_Y-30, FALSE);
      }

    }while(rect.left<0 || rect.top<0);

    handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    ReadProcessMemory(handle, (LPWORD)(moduleAddr+BASEADDR_GAMESCREEN), &addressGameScreen, 4, NULL);
    ReadProcessMemory(handle, (LPWORD)(addressGameScreen+OFFSET_GAMESCREEN_X), &initialGameScreenX, 4, NULL);
    ReadProcessMemory(handle, (LPWORD)(addressGameScreen+OFFSET_GAMESCREEN_Y), &initialGameScreenY, 4, NULL);

    //printf("Addresses: 0x%X, 0x%X, 0x%X\n", moduleAddr, BASEADDR_GAMESCREEN, moduleAddr+BASEADDR_GAMESCREEN);
    //printf("gamePos (relative): x%d, y%d\n\n", initialGameScreenX, initialGameScreenY);

    //SetCursorPos(rect.left+initialGameScreenX, rect.top+initialGameScreenY);

    ReadProcessMemory(handle, (LPWORD)(moduleAddr+BASEADDR_GAMESCREEN_HEIGHT), &addressGameScreen, 4, NULL);
    ReadProcessMemory(handle, (LPWORD)(addressGameScreen), &g_gameHeight, 4, NULL);
    ReadProcessMemory(handle, (LPWORD)(addressGameScreen-OFFSET_GAMESCREEN_HEIGHT_WIDTH), &g_gameWidth, 4, NULL);

    g_gamePosX = rect.left+initialGameScreenX;
    g_gamePosY = rect.top+initialGameScreenY;

    //Sleep(2000);
    //SetCursorPos(g_gamePosX+g_gameWidth, g_gamePosY+g_gameHeight);

    CloseHandle(handle);

  }else{
    printf("handle failed in window client\n");
  }

 
  printf("[battlelist] getScreenGamePosSync finished\n");
}
void getScreenGamePosAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, getScreenGamePos, getScreenGamePosComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}

void test(const FunctionCallbackInfo<Value>& args){
  INPUT input2[2];

  Isolate* isolate = args.GetIsolate();

  int i = 0;
  while(i<5){
    ::ZeroMemory(input2, 2*sizeof(INPUT));
    input2[0].type = INPUT_KEYBOARD;
    input2[0].ki.wScan = 0; // hardware scan code for key
    input2[0].ki.time = 0;
    input2[0].ki.dwExtraInfo = 0;
    // Press the "CTRL" key
    input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    input2[0].ki.dwFlags = 0; // 0 for key press

    //use revive by Hotkey DEL 
    input2[1].type = INPUT_KEYBOARD;
    input2[1].ki.wScan = 0; // hardware scan code for key
    input2[1].ki.time = 0;
    input2[1].ki.dwExtraInfo = 0;
    input2[1].ki.wVk = 0xDD; // virtual-key code for the "DEL" key
    input2[1].ki.dwFlags = 0; // 0 for key press

    SendInput(2, input2,sizeof(INPUT));

    Sleep(200);

    //Release of "CTRL" key
    input2[0].type = INPUT_KEYBOARD;
    input2[0].ki.wScan = 0; // hardware scan code for key
    input2[0].ki.time = 0;
    input2[0].ki.dwExtraInfo = 0;
    input2[0].ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    input2[0].ki.dwFlags  = KEYEVENTF_KEYUP;

    //Release of "DEL" key
    input2[1].type = INPUT_KEYBOARD;
    input2[1].ki.wScan = 0; // hardware scan code for key
    input2[1].ki.time = 0;
    input2[1].ki.dwExtraInfo = 0;
    input2[1].ki.wVk = 0xDD; // virtual-key code for the "[" key
    input2[1].ki.dwFlags  = KEYEVENTF_KEYUP;

    SendInput(2, input2, sizeof(INPUT));
    Sleep(2000);
    i++;
  }


  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "setScreenConfig", setScreenConfigSync);
  NODE_SET_METHOD(exports, "registerHKF10Async", registerHKF10Async);
  NODE_SET_METHOD(exports, "getBattleList", printBattleListAsync);
  NODE_SET_METHOD(exports, "fish", fishAsync);
  NODE_SET_METHOD(exports, "readBlCounter", readBlCounterAsync);
  NODE_SET_METHOD(exports, "attackPkm", attackPkmAsync);
  NODE_SET_METHOD(exports, "isPkmNear", isPkmNearAsync);
  NODE_SET_METHOD(exports, "isPkmNearSync", isPkmNearSync);
  NODE_SET_METHOD(exports, "isPlayerPkmSummoned", isPlayerPkmSummonedSync);
  NODE_SET_METHOD(exports, "revivePkm", revivePkmSync);
  NODE_SET_METHOD(exports, "registerHkSwapPkm", registerHkSwapPkmAsync);
  NODE_SET_METHOD(exports, "registerHkRevivePkm", registerHkRevivePkmAsync);
  NODE_SET_METHOD(exports, "registerHkDragBox", registerHkDragBoxAsync);
  NODE_SET_METHOD(exports, "registerHkMedicineLoop", registerHkMedicineLoopAsync);
  NODE_SET_METHOD(exports, "registerPkmSlot", registerPkmSlotSync);
  NODE_SET_METHOD(exports, "getPlayerPos", getPlayerPosAsync);
  NODE_SET_METHOD(exports, "runProfile", runProfileAsync);
  NODE_SET_METHOD(exports, "stopProfileSync", stopProfileSync);
  NODE_SET_METHOD(exports, "mapUnownMovementSync", mapUnownMovementSync);
  NODE_SET_METHOD(exports, "mapIllusionMovementSync", mapIllusionMovementSync);
  NODE_SET_METHOD(exports, "runCyberScript", runCyberScriptAsync);
  NODE_SET_METHOD(exports, "getScreenGamePos", getScreenGamePosAsync);
  NODE_SET_METHOD(exports, "test", test);
//  NODE_SET_METHOD(exports, "sendKey", sendKey);
}

NODE_MODULE(battlelist, init)

}  // namespace battelistAddon




