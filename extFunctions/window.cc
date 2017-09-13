// hello.cc
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>

namespace focusAddon{

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Number;
using v8::Value;
using v8::Persistent;
using v8::Function;
using v8::Handle;


struct Work{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
};

// Structure used to communicate data from and to enumeration procedure
struct EnumData {
    DWORD dwProcessId;
    HWND hWnd;
};

HWND SelectWindowByProcessName(std::string Name);
HWND FindWindowByPid( DWORD dwProcessId );
BOOL CALLBACK EnumProc( HWND hWnd, LPARAM lParam );

//PCSTR name
HWND SelectWindowByProcessName(std::string name){
  DWORD pid = 0;
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
            if (processName == name){
               pid = process.th32ProcessID;
               break;
            }
        } while (Process32Next(snapshot, &process));
    }
    CloseHandle(snapshot);

    if(pid != 0){
      HWND hWnd = FindWindowByPid(pid);
      return hWnd;
      //return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    return NULL;
}

HWND FindWindowByPid( DWORD dwProcessId ) {
    EnumData ed = { dwProcessId };
    if ( !EnumWindows( EnumProc, (LPARAM)&ed ) && ( GetLastError() == ERROR_SUCCESS ) ) {
      return ed.hWnd;
    }
    return NULL;
}

// Application-defined callback for EnumWindows
BOOL CALLBACK EnumProc( HWND hWnd, LPARAM lParam ) {
    // Retrieve storage location for communication data
    EnumData& ed = *(EnumData*)lParam;
    DWORD dwProcessId = 0x0;
    // Query process ID for hWnd
    GetWindowThreadProcessId( hWnd, &dwProcessId );
    // Apply filter - if you want to implement additional restrictions,
    // this is the place to do so.
    //printf("ENUMPROC: Comparing pid %ld == %ld ?\n", ed.dwProcessId, dwProcessId);
    if ( ed.dwProcessId == dwProcessId ) {
        // Found a window matching the process ID
        ed.hWnd = hWnd;
        // Report success
        SetLastError( ERROR_SUCCESS );
        // Stop enumeration
        return FALSE;
    }
    // Continue enumeration
    return TRUE;
}

static void focusWindow(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  HINSTANCE module = GetModuleHandle(NULL);

  //LPSTR pcmdLine = wtext;
  HWND hWndPXG = SelectWindowByProcessName("pxgclient.exe");
  SetForegroundWindow(hWndPXG);
  //while(SetForegroundWindow(hWndPXG)==0){}
}

static void focusWindowComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  Work *work = static_cast<Work*>(req->data);

  //Creates a variable of type Number which stores a value of v8::Number with value 1
  //Local variables are just visible to this scope. Handles are visible even in Javascript
  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = {val};

  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void focusWindowAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, focusWindow, focusWindowComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "focusWindow", focusWindowAsync);
}

NODE_MODULE(focus, init)

}  // namespace focusAddon
