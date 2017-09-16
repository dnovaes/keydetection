
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>

namespace keyboardn{

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
  void* vk_code;
};

static void pressKbKey(uv_work_t *req){

  INPUT ip;
  Work *work = static_cast<Work*>(req->data);
  char* vk_code = (char*)work->vk_code;

  //printf("key requested: %s\n", vk_code);

    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press the "CTRL" key
    ip.ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Press the "Z" key
    ip.ki.wVk = 0x5A; // virtual-key code for the "Z" key
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    Sleep(300);

    // Release the "Z" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "CTRL" key
    ip.ki.wVk = 0x11; // virtual-key code for the "CTRL" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}

static void pressKbKeyComplete(uv_work_t *req, int status){
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

void pressKbKeyAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  Local<String> arg0 = Local<String>::Cast(args[0]);
  String::Utf8Value value(arg0);
  char* vk_code = *value;
  //printf("key requested: %s\n", vk_code);
  work->vk_code = (void*) vk_code;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[1]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_queue_work(uv_default_loop(), &work->request, pressKbKey, pressKbKeyComplete);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "pressKbKey", pressKbKeyAsync);
}

NODE_MODULE(keyboard, init)

}// namespace keyboardn
