
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

static void setCursorPos(uv_work_t *req){

  //POINT pt;
  Work *work = static_cast<Work*>(req->data);

  SetCursorPos(work->pos.x, work->pos.y);
  printf("Mouse at pos: %d, %d\n", work->pos.x, work->pos.y);

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
  //uv_async_init(uv_default_loop(), &work->async, sendSygnalCursorPos);
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

  Sleep(500);

  // left up
  ::ZeroMemory(&input,sizeof(INPUT));
  input.type      = INPUT_MOUSE;
  input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
  SendInput(1,&input,sizeof(INPUT));

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

void init(Local<Object> exports) {
  //NODE_SET_METHOD(exports, "getCursorPosition", getCursorPositionAsync);
  NODE_SET_METHOD(exports, "setCursorPos", setCursorPosAsync);
  NODE_SET_METHOD(exports, "leftClick", leftClickAsync);
}

NODE_MODULE(mouse, init)

}  // namespace mousen
