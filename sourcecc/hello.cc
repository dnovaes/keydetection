// hello.cc
#include <node.h>
#include <uv.h>
//#include <windows.h>
#include <tchar.h>
//#include <iostream>
#include <signal.h>

namespace demo {

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
};

typedef struct pos pos;
struct pos{
  int x;
  int y;
};

typedef struct sygPos sygPos;
struct sygPos{
  uv_work_t req;
  pos arrayPos[2];
};

//global variables
Work *workGPos;
sygPos objG, *pobjG;

//register hotkey f10 while worker thread is running asynchronically
static void registerHKF10(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);

  //0x79 = F10
  if(RegisterHotKey(NULL, 1, NULL, 0x79)){
    MSG msg = {0};

    printf("Running as PID=%d\n",getpid());

    while (GetMessage(&msg, NULL, 0, 0) != 0 ){
      if(msg.message == WM_HOTKEY){
        //UnregisterHotKey(NULL, 1);

        //Communication between threads(uv_work_t and uv_async_t);
        work->async.data = (void*) req;
        uv_async_send(&work->async);
      }
    }
  }
}

static void WorkAsyncComplete(uv_work_t *req, int status){
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  Work *work = static_cast<Work*>(req->data);

  //set up return arguments
//  Local<Array> returnArgs = Array::New(isolate);
//  unsigned int i = 1;
//  returnArgs->Set(i,i);

  //Creates a variable of type Number which stores a value of v8::Number with value 1
  //Local variables are just visible to this scope. Handles are visible even in Javascript
  //Local<Number> val = Number::New(isolate, 1);
  //Handle<Value> argv[] = {val};

  //execute the callback
  //Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
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
  uv_queue_work(uv_default_loop(), &work->request, registerHKF10, WorkAsyncComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

static void getCursorPos(uv_work_t *req){

  POINT pt;
  workGPos = static_cast<Work*>(req->data);
  MSG msg = {0};
  unsigned int i=0;
  pos arPos[2];

  pobjG=&objG;
  if(RegisterHotKey(NULL, 1, NULL, 0x52)){

    while(GetMessage(&msg, NULL, 0, 0)){

      if(msg.message == WM_HOTKEY){

        GetCursorPos(&pt);
        _tprintf(_T("Pos: %d, %d\n"), pt.x, pt.y);
        arPos[i].x = pt.x;
        arPos[i].y = pt.y;


        /*
        Local<Object> obj = Object::New(isolate);

        obj->Set(String::NewFromUtf8(isolate, "red"), Number::New(isolate, _red));
        obj->Set(String::NewFromUtf8(isolate, "green"), Number::New(isolate, _green));
        obj->Set(String::NewFromUtf8(isolate, "blue"), Number::New(isolate, _blue));
        */
        objG.arrayPos[i] = arPos[i];

        //int red = obj->Get(String::NewFromUtf8(isolate, "red"))->IntegerValue();
        //printf("Red: %d\n", red);

        i++;

        if(i==2){
          //Communication between threads(uv_work_t and uv_async_t);
          objG.req = *req;

          workGPos->async.data = (void*)&objG;
          uv_async_send(&workGPos->async);

          break;
        }
      }
    }
    UnregisterHotKey(NULL, 1);
  }
}

void sendSygnalCursorPos(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  //Local<Object> colors = static_cast<Object*>(handle->data)->ToObject();
  sygPos *pobj = ((sygPos*) handle->data);
  uv_work_t req = ((uv_work_t) pobj->req);
  Work *work = static_cast<Work*> (req.data);

  Local<Object> objJSON = Object::New(isolate);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, pobj->arrayPos[0].x));
  obj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, pobj->arrayPos[0].y));

  objJSON->Set(String::NewFromUtf8(isolate, "NW"), obj);

  obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, pobj->arrayPos[1].x));
  obj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, pobj->arrayPos[1].y));

  objJSON->Set(String::NewFromUtf8(isolate, "SE"), obj);

  //printf("pointer req: %p\n", &pobj->req);

  Handle<Value> argv[] = {objJSON};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
}

void getCursorPositionAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalCursorPos);
  uv_queue_work(uv_default_loop(), &work->request, getCursorPos, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "registerHKF10Async", registerHKF10Async);
  NODE_SET_METHOD(exports, "getCursorPosition", getCursorPositionAsync);
}

NODE_MODULE(addon, init)

}  // namespace demo
