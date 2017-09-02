// hello.cc
#include <node.h>
#include <windows.h>
//#include <tchar.h>
//#include <iostream>

namespace demo {

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Boolean;
using v8::Value;

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
        args.GetReturnValue().Set(Boolean::New(isolate, true));
        break;
      }
    }
  }
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "registerHotKeyF10", registerHotKeyF10);
}

NODE_MODULE(addon, init)

}  // namespace demo
