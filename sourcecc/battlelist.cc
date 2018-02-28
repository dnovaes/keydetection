
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

//global var
char mutex = 1; //1 = continue
uv_rwlock_t numlock;
BOOL fDebugWhileEvent = FALSE;
DWORD pid;
DWORD dwThreadID;
DWORD_PTR moduleAddr;
POINT center, sqm;
HANDLE hThread;
RECT pxgCoords;
int m1 = 0;
int m2 = 0;
int m3 = 0;
int m4 = 0;

//const

/* DIRECTX9 */
//global allocations (with AOB injection)
const DWORD_PTR BASEADDR_CREATURE_GENERATOR = 0x05AF0000; //_creatureBase
//adress for position change (writes into pokemon pos. when something moves in screen)
const DWORD_PTR INSTR_POSADDR               = 0x1461A4;
//use moduleAddress as base
const DWORD_PTR OFFSET_PLAYER_POSX          = 0x38F840;
const DWORD_PTR OFFSET_PLAYER_POSY          = 0x38F844;
const DWORD_PTR OFFSET_PLAYER_POSZ          = 0x38F848;
//use creatureAddress as base
const DWORD_PTR OFFSET_PKM_NAME_LENGTH      = 0x24; //byte
const DWORD_PTR OFFSET_PKM_NAME             = 0x28;
const DWORD_PTR OFFSET_PKM_POSX             = 0xC;
const DWORD_PTR OFFSET_PKM_POSY             = 0x10;
const DWORD_PTR OFFSET_PKM_POSZ             = 0x14;
const DWORD_PTR OFFSET_PKM_LIFE             = 0x38;
//pointer to battlelist counter of active creatures
const DWORD_PTR BASEADDR_BLACOUNT           = 0x0038E820;
const DWORD_PTR OFFSET_BLCOUNT_P1           = 0x33C;
const DWORD_PTR OFFSET_BLCOUNT_P2           = 0xA4;
const DWORD_PTR OFFSET_BLCOUNT_P3           = 0x30;
//pointer to fishing status address
const DWORD_PTR BASEADDR_FISHING            = 0x0038E83C;
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

DWORD GetProcessThreadID(DWORD pID);
void printThreads(DWORD pid);

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
            //printTCHAR(ModuleEntry32.szModule);
            printf(" Module Name: %s\n", ModuleEntry32.szModule);
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

//main function
static void printBattleList(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  HINSTANCE module = GetModuleHandle(NULL);


  pid = 0;
  // Create toolhelp snapshot.
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process;
  ZeroMemory(&process, sizeof(process));
  process.dwSize = sizeof(process);

  // Walkthrough all processes.
  if (Process32First(snapshot, &process)){
      do{
          // Compare process.szExeFile based on format of name, i.e., trim file path
          // trim .exe if necessary, etc.
          std::string processName = std::string(process.szExeFile);
          //if (processName == "MineSweeper.exe"){ //#mine
          if (processName == "pxgclient_dx9.exe"){
             pid = process.th32ProcessID;
             printf("Process %d (0x%04X) \n", pid, pid);
             break;
          }
      } while (Process32Next(snapshot, &process));
  }
  CloseHandle(snapshot);

  //When program gets SIGINT, it will trigger the function signal_callback_handler to execute
  // SIGINT = CTRL+C
  signal(SIGINT, signal_callback_handler);
  //printf("pid: %d \n", getpid());

  moduleAddr = 0;
  //get pointer to module adress and also prints module name
  moduleAddr = dwGetModuleBaseAddress(pid, "pxgclient_dx9.exe");
  //moduleAddr = dwGetModuleBaseAddress(pid, "MineSweeper.exe"); //#mine

  printf("address of module: 0x%X\n", (DWORD)moduleAddr);

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);

  DWORD_PTR value;
  //DWORD numBytesRead;

  //read pointer stored in the moduleAddress
  //ReadProcessMemory(handle, (LPVOID)(moduleAddr), &value, sizeof(DWORD), NULL);
  //printf("ModuleAddress value (Entry Point): 0x%X\n", value);

  ReadProcessMemory(handle, (LPVOID)(moduleAddr), &value, sizeof(DWORD), NULL);
  printf("ModuleAddress value: 0x%X\n", value);

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

  HANDLE handlep = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  char entityName[16];
  byte entityType, entityNumLetters;
  byte entityNumberBl, oldEntityNumberBl = 0x0;

  DWORD_PTR entityAddr;//a just spawed pokemon, npc or player

  //read pointer

  printf("iniciando deteccao de pokemons\n");
  do{

    ReadProcessMemory(handlep, (LPDWORD)(BASEADDR_CREATURE_GENERATOR), &entityAddr, 4, NULL);
    entityAddr = entityAddr-0x28;
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x24), &entityNumLetters, 1, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x28), &entityName, 16, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr), &entityType, 1, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x1C), &entityNumberBl, 1, NULL);

    entityName[entityNumLetters] = 0;

    if(entityNumberBl != oldEntityNumberBl){
      //printf("\nentityName: %s %d, Type: %d, creatureCounter: %d, Address: 0x%X \n", entityName, entityNumLetters, entityType, entityNumberBl, entityAddr);
      oldEntityNumberBl = entityNumberBl;

      work->creatureAddr = entityAddr;
      work->fAction =2;

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
          printf("base priority  = %d\n", te.tpBasePri);
          printf("delta priority = %d\n", te.tpDeltaPri);
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
    char entityName[16];
    byte entityType, entityNumberBl;

    HANDLE handlep = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr+0x28), &entityName, 16, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(entityAddr), &entityType, 1, NULL);
    ReadProcessMemory(handlep, (LPDWORD)(entityName-0x0C), &entityNumberBl, 1, NULL);
    CloseHandle(handlep); //close handle of process

    obj->Set(String::NewFromUtf8(isolate, "addr"), Number::New(isolate, entityAddr));
    obj->Set(String::NewFromUtf8(isolate, "name"), String::NewFromUtf8(isolate, entityName));
    obj->Set(String::NewFromUtf8(isolate, "type"), Number::New(isolate, entityType));
    obj->Set(String::NewFromUtf8(isolate, "counterBl"), Number::New(isolate, entityNumberBl));
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
  uv_queue_work(uv_default_loop(), &work->request, printBattleList, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

void setScreenConfigSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

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
  Local<Function> callback = Local<Function>::Cast(args[2]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //uv_queue_work(uv_default_loop(), &work->request, waitForDeath, waitForDeathComplete);

  Local<Number> val = Number::New(isolate, 1);
  args.GetReturnValue().Set(val);
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

static void waitForDeath(uv_work_t *req){
  Work *work = static_cast<Work*>(req->data);

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  INPUT input;
  char life;

  do{
    Sleep(800);
    ReadProcessMemory(handle, (LPDWORD)(work->creatureAddr+OFFSET_PKM_LIFE), &life, 1, NULL);

    if(m1){
      // Set up a generic keyboard event.
      input.type = INPUT_KEYBOARD;
      input.ki.wScan = 0; // hardware scan code for key
      input.ki.time = 0;
      input.ki.dwExtraInfo = 0;

      // Press the "F1" key
      input.ki.wVk = VK_F1; // virtual-key code for the "F1" key
      input.ki.dwFlags = 0; // 0 for key press
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);

      input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);
    }
    if(m2){
      // Set up a generic keyboard event.
      input.type = INPUT_KEYBOARD;
      input.ki.wScan = 0; // hardware scan code for key
      input.ki.time = 0;
      input.ki.dwExtraInfo = 0;

      input.ki.wVk = VK_F2; // virtual-key code for the "F2" key
      input.ki.dwFlags = 0; // 0 for key press
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);

      input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);
    }

    if(m3){
      // Set up a generic keyboard event.
      input.type = INPUT_KEYBOARD;
      input.ki.wScan = 0; // hardware scan code for key
      input.ki.time = 0;
      input.ki.dwExtraInfo = 0;

      input.ki.wVk = VK_F3; // virtual-key code for the "F3" key
      input.ki.dwFlags = 0; // 0 for key press
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);

      input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);
    }

    if(m4){
      // Set up a generic keyboard event.
      input.type = INPUT_KEYBOARD;
      input.ki.wScan = 0; // hardware scan code for key
      input.ki.time = 0;
      input.ki.dwExtraInfo = 0;

      input.ki.wVk = VK_F4; // virtual-key code for the "F4" key
      input.ki.dwFlags = 0; // 0 for key press
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);

      input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
      SendInput(1, &input, sizeof(INPUT));

      Sleep(50);
    }
    //printf("pokemon currently life: %d\n", life);
  }while(life > 0);

  CloseHandle(handle);
}

static void waitForDeathComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  Work *work = static_cast<Work*>(req->data);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void waitForDeathAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  work->creatureAddr = args[0]->Int32Value();
  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  //attackPkmComplete here is a default funciton that returns 1 when complete. just reusing code
  uv_queue_work(uv_default_loop(), &work->request, waitForDeath, waitForDeathComplete);

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

//  printf("isPkmNearComplete calculations started\n");

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

void revivePkmSync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  POINT pkmSlot, reviveSlot;

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

  //select and click revive in revive slot
  SetCursorPos(reviveSlot.x, reviveSlot.y);

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

  //point to pkmslot and use selected revive
  SetCursorPos(pkmSlot.x, pkmSlot.y);

  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  //summon pokemon
  SetCursorPos(pkmSlot.x, pkmSlot.y);

  Sleep(100);

  ::ZeroMemory(&input,sizeof(INPUT));
  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(50);

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  args.GetReturnValue().Set(val);
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "setScreenConfig", setScreenConfigSync);
  NODE_SET_METHOD(exports, "getBattleList", printBattleListAsync);
  NODE_SET_METHOD(exports, "fish", fishAsync);
  NODE_SET_METHOD(exports, "readBlCounter", readBlCounterAsync);
  NODE_SET_METHOD(exports, "attackPkm", attackPkmAsync);
  NODE_SET_METHOD(exports, "waitForDeath", waitForDeathAsync);
  NODE_SET_METHOD(exports, "isPkmNear", isPkmNearAsync);
  NODE_SET_METHOD(exports, "isPkmNearSync", isPkmNearSync);
  NODE_SET_METHOD(exports, "isPlayerPkmSummoned", isPlayerPkmSummonedSync);
  NODE_SET_METHOD(exports, "revivePkm", revivePkmSync);
}

NODE_MODULE(battlelist, init)

}  // namespace battelistAddon




