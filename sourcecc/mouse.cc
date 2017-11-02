
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>

namespace mousen{

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

struct Work{
  //pointer when starting worker threads
  uv_work_t request;
  uv_async_t async;
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
  POINT pos;
};

//global var

//Directx9
//pointer to fishing status address (copied from battlelist.cc)
const DWORD_PTR BASEADDR_FISHING  = 0x002EA83C;
const DWORD_PTR OFFSET_FISHING_P1 = 0x54;
const DWORD_PTR OFFSET_FISHING_P2 = 0x4;

int pid;
DWORD_PTR moduleAddr;

static void setCursorPos(uv_work_t *req){

  //POINT pt;
  Work *work = static_cast<Work*>(req->data);

  SetCursorPos(work->pos.x, work->pos.y);
  Sleep(100);

  //work->async.data = (void*)&obj;
  //uv_async_send(&work->async);
}

static void setCursorPosComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  Work *work = static_cast<Work*>(req->data);

  //set up return arguments
//  Local<Array> returnArgs = Array::New(isolate);
//  unsigned int i = 1;
//  returnArgs->Set(i,i);

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

void setCursorPosAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  Local<Object> objPos = Local<Object>::Cast(args[0]);
  Local<Value> x = objPos->Get(String::NewFromUtf8(isolate, "x"));
  Local<Value> y = objPos->Get(String::NewFromUtf8(isolate, "y"));
  //printf("coords X: %d\n", x->IntegerValue()); //returns a int64_t
  //printf("coords X: %d\n", x->Uint32Value());
  work->pos.x = x->Uint32Value();
  work->pos.y = y->Uint32Value();

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, setCursorPos, setCursorPosComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void leftClick(uv_work_t *req){

  INPUT input;
  Work *work = static_cast<Work*>(req->data);

  // left down
  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(100);

  // left up
  ::ZeroMemory(&input,sizeof(INPUT));
  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

  Sleep(100);
}

static void leftClickComplete(uv_work_t *req, int status){
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

void leftClickAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, leftClick, leftClickComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void getColorFishing(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  int i=0, j;
  int NSAMPLES = 210;
  int _red[5];
  int _green[5];
	  //int _blue[2];

  HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, pid);
  DWORD address;
  int x, y;
  byte fishStatus = 3; //3 = fishing, 2 = normal

  //SetCursorPos(work->pos.x, work->pos.y);
  x = work->pos.x;
  y = work->pos.y;
  Sleep(6000);

  HDC dc = GetDC(NULL);
  COLORREF _pixel[5];

  while(i<NSAMPLES && fishStatus==3){


    _pixel[0] = GetPixel(dc, x-2, y);
    _pixel[1] = GetPixel(dc, x-1, y);
    _pixel[2] = GetPixel(dc, x, y);
    _pixel[3] = GetPixel(dc, x+1, y);
    _pixel[4] = GetPixel(dc, x+2, y);

    for(j=0;j<5;j++){
      _red[j] = GetRValue(_pixel[j]);
      _green[j] = GetGValue(_pixel[j]);
      //printf("Red: %d, Green: %d\n", _red[j], _green[j]);
      //printf("%d, %d\n", x, y);
      if(((_red[j] < 20)&&(_green[j] > 100))||(_green[j] == 255)){
        //printf("Red: 0x%02x %d\n", _red[0], _red[0]);
        //printf("Green: 0x%02x %d\n", _green[0], _green[0]);
        i=NSAMPLES;
        break;
      }else{
        Sleep(50);
      }
    }
    //_blue[0] = GetBValue(_pixel);

    ReadProcessMemory(handle, (LPDWORD)(moduleAddr+BASEADDR_FISHING), &address, sizeof(DWORD), NULL);
    ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_FISHING_P1), &address, sizeof(DWORD), NULL);
    ReadProcessMemory(handle, (LPDWORD)(address+OFFSET_FISHING_P2), &fishStatus, 1, NULL);

    //printf("fishing status [mouse]: %d\n", fishStatus);
    //printf("%d, %d\n", work->pos.x, work->pos.y);
    i++;
  }
  printf("fishing status [mouse]: %d\n", fishStatus);
  //printf("fishing finished");
  ReleaseDC(NULL, dc);
}

static void getColorFishingComplete(uv_work_t *req, int status){
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

void getColorFishingAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  Local<Object> posObj = Local<Object>::Cast(args[0]);
  Local<Value> x = posObj->Get(String::NewFromUtf8(isolate, "x"));
  work->pos.x = x->Uint32Value();
  Local<Value> y = posObj->Get(String::NewFromUtf8(isolate, "y"));
  work->pos.y = y->Uint32Value();

  pid = args[1]->Int32Value();
  moduleAddr = args[2]->Int32Value();

  //posObj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, x->Uint32Value()));
  //posObj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, y->Uint32Value()));
  //work->ppos.Reset(isolate, posObj);

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[3]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, getColorFishing, getColorFishingComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void getCursorPos(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  MSG msg = {0};

  //0x52 = R key
  if(RegisterHotKey(NULL, 1, NULL, 0x52)){

    while(GetMessage(&msg, NULL, 0, 0)){

      if(msg.message == WM_HOTKEY){

        GetCursorPos(&work->pos);
/*
        int i;
        for(i=0;i<6;i++){
          SetCursorPos(work->pos.x+i*32, work->pos.y);
          Sleep(1000);
        }
*/
        break;
      }
    }
    UnregisterHotKey(NULL, 1);
  }
}

static void getCursorPosComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

//  printf("isPkmNearComplete calculations started\n");

  Work *work = static_cast<Work*>(req->data);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, work->pos.x));
  obj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, work->pos.y));

  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void getCursorPosAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, getCursorPos, getCursorPosComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "getCursorPosbyClick", getCursorPosAsync);
  NODE_SET_METHOD(exports, "setCursorPos", setCursorPosAsync);
  NODE_SET_METHOD(exports, "leftClick", leftClickAsync);
  NODE_SET_METHOD(exports, "getColorFishing", getColorFishingAsync);
}

NODE_MODULE(mouse, init)

}  // namespace mousen
