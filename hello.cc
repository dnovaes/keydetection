// hello.cc
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>

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
  //stores the javascript callback function, persistent means: store in the heap
  Persistent<Function> callback;
};


void registerHotKeyF10(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  //args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));

 // Check the number of arguments passed.

  /*
  if (args.Length() < 1) {
    // Throw an Error that is passed back to JavaScript
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }
  */

  //VK_F10 == 0x79 = F10 key
  if(RegisterHotKey(NULL, 1, NULL, 0x79)){
//    _tprintf(_T("Hotkey 'F10' registered!\n"));

//    char val[25] = "This is a test with cout";
//    std::cout << val << std::endl;

    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0) != 0){
      if(msg.message == WM_HOTKEY){

//        _tprintf(_T("WM_HOTKEY received\n"));

        UnregisterHotKey(NULL, 1);
        //return text message
        args.GetReturnValue().Set(Boolean::New(isolate, "v8::True()"));
        break;
      }
    }
  }
}

//register hotkey f10 while worker thread is running asynchronically
static void registerHKF10(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);

  if(RegisterHotKey(NULL, 1, NULL, 0x79)){
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0) != 0){
      if(msg.message == WM_HOTKEY){

//        _tprintf(_T("WM_HOTKEY received\n"));
        UnregisterHotKey(NULL, 1);
        //args.GetReturnValue().Set(Boolean::New(isolate, true));
        //std::transform(Boolean::New(isolate, true));
        break;
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

  Local<Number> val = Number::New(isolate, 1);
  Handle<Value> argv[] = { val};
  //Local<Boolean> b = Boolean::New(isolate, true);
  //argv[1] = {b};

  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void registerHKF10Async(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, registerHKF10, WorkAsyncComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  //NODE_SET_METHOD(exports, "registerHotKeyF10", registerHotKeyF10);
  NODE_SET_METHOD(exports, "registerHKF10Async", registerHKF10Async);
}

NODE_MODULE(addon, init)

}  // namespace demo
