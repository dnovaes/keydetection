// hello.cc
#include <node.h>
#include <uv.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <iostream>
#include <string>

namespace windowAddon{

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

typedef struct vec vec;
struct vec{
  int x;
  int y;
  int w;
  int h;
};

typedef struct sygData sygData;
struct sygData{
  uv_work_t req;
  vec data;
};

#define TIMER_ID 1
#define NSAMPLESZOOM 17;
const int CZOOMFACTOR=12;
BOOL fDraw = FALSE;
BOOL fbtDown= FALSE;
BOOL fprtScreen = FALSE;
POINT ptLC, ptLCrelease;

//Foward declarations of functions included in this module
LRESULT CALLBACK WndProc(
  _In_ HWND hWnd,
  _In_ UINT uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
);
int CaptureAnImage(HWND hWnd);
bool LoadBMPIntoDC(HDC hDC, LPCTSTR bmpfile, HWND hWnd);
void drawMagnefier(HWND hWnd, HDC hdcMem);
void paintPixels(HDC hdcWindow, int dx, int dy, COLORREF cPixel, int xOffset, int yOffset);
int winmain(HINSTANCE module, HINSTANCE, LPSTR pCmdLine, int nCmdShow, uv_work_t *req);

//int __stdcall winmain(HINSTANCE module, HINSTANCE, LPSTR pCmdLine, int nCmdShow)
//int __stdcall winmain(HINSTANCE module, HINSTANCE, LPSTR pCmdLine, int nCmdShow, uv_work_t *req){
int winmain(HINSTANCE module, HINSTANCE, LPSTR pCmdLine, int nCmdShow, uv_work_t *req){
  WNDCLASS wc = {};
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hInstance = module;
  //wc.lpszClassName = L"window";
  wc.lpszClassName = _T("window");
  wc.lpfnWndProc = WndProc;

  RegisterClass(&wc);

  //Local<Number> val = Number::New(isolate, 1);
  //Handle<Value> argv[] = {val};
  //execute the callback
  //Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);

  //WS_MAXIMIZE, WS_VISIBLE, WS_POPUPWINDOW
/*
  HWND hWnd = CreateWindow(wc.lpszClassName, _T("LeftClick on your screen corners"),
      (WS_VISIBLE| WS_POPUPWINDOW), 0, 0, width, height, NULL, NULL, wc.hInstance, NULL);
*/
  HWND hWnd = CreateWindowEx(
		WS_EX_TOPMOST, wc.lpszClassName, _T("LeftClick on your screen corners"),
   (WS_POPUPWINDOW | WS_VISIBLE ), 0, 0,
  	width, height, NULL, NULL, wc.hInstance, NULL);

  if (!hWnd){
    MessageBox(NULL,
        _T("Call to CreateWindow failed!"),
        _T("Win32 Guided Tour"),
        NULL);
    return 1;
  }

  SetFocus(hWnd);
  BringWindowToTop(hWnd);
  SetActiveWindow(hWnd);
  EnableWindow(hWnd, true);
  MSG msg;

  RegisterHotKey(hWnd, 1, NULL, VK_LEFT);
  RegisterHotKey(hWnd, 2, NULL, VK_UP);
  RegisterHotKey(hWnd, 3, NULL, VK_RIGHT);
  RegisterHotKey(hWnd, 4, NULL, VK_DOWN);
  //HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
  //wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
  //SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(brush));

  //SetWindowLongPtr(hWnd, GWL_STYLE, 0);
  //SetWindowPos(hwnd01, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  //ShowWindow(hWnd, SW_SHOW);

  //SetTimer(hWnd, TIMER_ID, 50, (TIMERPROC) NULL);

  Work *work = static_cast<Work*>(req->data);
  sygData obj;
  sygData *pobj;
  pobj=&obj;
  while(TRUE){
  //while(GetMessage(&msg, 0, 0, 0) > 0){
    while(PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)){
      DispatchMessage(&msg);
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    //ShowWindow(hWnd, nCmdShow);

    if(msg.message == WM_QUIT){
      break;
    }
    if(((msg.message == WM_KEYDOWN)&&(msg.wParam == VK_ESCAPE))||(msg.message == WM_LBUTTONUP)){
      pobj->req = *req;
      pobj->data.x = ptLC.x;
      pobj->data.y = ptLC.y;
      pobj->data.w = ptLCrelease.x - ptLC.x;
      pobj->data.h = ptLCrelease.y - ptLC.y;

      work->async.data = (void*)pobj;
      uv_async_send(&work->async);
      //CloseWindow(hWnd);
      UnregisterHotKey(hWnd, 1);
      UnregisterHotKey(hWnd, 2);
      UnregisterHotKey(hWnd, 3);
      UnregisterHotKey(hWnd, 4);
      //kill process window by force
      DestroyWindow(hWnd);
      break;
    }
  }
  return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){

    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, World!");
    POINT lp;

    switch (msg){
      /*
      case WM_TIMER:
        InvalidateRect(hWnd, NULL, TRUE); //force a WM_PAINT message and updates the window
        break;
      */
      case WM_HOTKEY:
        switch(wParam){
          case 1: //arrow left
            GetCursorPos(&lp);
            SetCursorPos(lp.x-1, lp.y);
            break;
          case 2: //arrow up
            GetCursorPos(&lp);
            SetCursorPos(lp.x, lp.y-1);
            break;
          case 3: //arrow right
            GetCursorPos(&lp);
            SetCursorPos(lp.x+1, lp.y);
            break;
          case 4: //arrow down
            GetCursorPos(&lp);
            SetCursorPos(lp.x, lp.y+1);
            break;
        }
        break;
      case WM_KILLFOCUS:
        SetFocus(hWnd);
        BringWindowToTop(hWnd);
        SetActiveWindow(hWnd);
        EnableWindow(hWnd, true);
        break;
      case WM_KEYDOWN:
        //ptLCrelease indicates pos of mouse movement
        switch(wParam){
          case VK_ESCAPE:
            //PostQuitMessage(0);
            break;
          case VK_LEFT:
            GetCursorPos(&lp);
            SetCursorPos(lp.x-1, lp.y);
            break;
          case VK_RIGHT:
            GetCursorPos(&lp);
            SetCursorPos(lp.x+1, lp.y);
            break;
          case VK_UP:
            GetCursorPos(&lp);
            SetCursorPos(lp.x, lp.y-1);
            break;
          case VK_DOWN:
            GetCursorPos(&lp);
            SetCursorPos(lp.x, lp.y+1);
            break;
        }
        break;
      case WM_LBUTTONDOWN:
        {
          fDraw = false;
          ptLC.x = GET_X_LPARAM(lParam);
          ptLC.y = GET_Y_LPARAM(lParam);
          fbtDown = true;

          //HINSTANCE hInstance = GetModuleHandle(NULL);
          //GetModuleFileName(hInstance, szFileName, MAX_PATH);
          //MessageBox(hWnd, strMsg, "LEFTCLICKED!", MB_OK | MB_ICONINFORMATION);
          break;
        }
      case WM_LBUTTONUP:
        {
          fbtDown = false;
          fDraw = false;
          fprtScreen = FALSE;
          break;
        }
      case WM_MOUSEMOVE:
        {
          //ptLCrelease indicates pos of mouse movement
          ptLCrelease.x = (GET_X_LPARAM(lParam));
          ptLCrelease.y = (GET_Y_LPARAM(lParam));
          if(fbtDown){
            fDraw = true;
          }
          InvalidateRect(hWnd, NULL, TRUE); //force a WM_PAINT message and updates the window
          break;
        }
      case WM_PAINT:
        {
          hdc = BeginPaint(hWnd, &ps);
          HPEN hPen;

          if(!fprtScreen){
            CaptureAnImage(hWnd);
          }
          HDC hdcWindow = GetDC(hWnd);
          LoadBMPIntoDC(hdcWindow, _T("captureqwsx.bmp"), hWnd);
          ReleaseDC(hWnd, hdcWindow);

          if(fDraw){
            char strMsg[MAX_PATH];
            strcpy(strMsg, "X: ");
            std::string var = std::to_string(ptLC.x);
            strcat(strMsg, var.c_str());

            strcat(strMsg, _T(" Y: "));
            var = std::to_string(ptLC.y);
            strcat(strMsg, var.c_str());

            strcat(strMsg, _T(" W: "));
            var = std::to_string(ptLCrelease.x - ptLC.x);
            strcat(strMsg, var.c_str());

            strcat(strMsg, _T(" H: "));
            var = std::to_string(ptLCrelease.y - ptLC.y);
            strcat(strMsg, var.c_str());

            TextOut(hdc, ptLC.x, ptLC.y-15, strMsg, _tcslen(strMsg));

            hPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
            SelectObject(hdc, hPen);
            SetBkColor(hdc, RGB(0,0,0)); //background of dashed lines

            MoveToEx(hdc, ptLC.x, ptLC.y, NULL);
            LineTo(hdc, ptLC.x, ptLCrelease.y);
            LineTo(hdc, ptLCrelease.x, ptLCrelease.y);

            MoveToEx(hdc, ptLC.x, ptLC.y, NULL);
            LineTo(hdc, ptLCrelease.x, ptLC.y);
            LineTo(hdc, ptLCrelease.x, ptLCrelease.y);

          }

          EndPaint(hWnd, &ps);
          break;
        }
      /*case WM_DESTROY:
          break;*/
      default:
          return DefWindowProc(hWnd, msg, wParam, lParam);
          break;
    }
    return 0;

}

void drawMagnefier(HWND hWnd, HDC hdcMem){
  int i, j;
  int nSamples = NSAMPLESZOOM;
  //HDC hdcWindow = GetDC(hWnd);
  COLORREF cPixel;

  RECT rcClient;
  GetClientRect(hWnd, &rcClient);

  POINT dMagnefier;

  //nSamples/2 is the distance from the corner to the center
  for(i=0;i<nSamples;i++){ //lines
    for(j=0;j<nSamples;j++){ //columns
      cPixel = GetPixel(hdcMem, ptLCrelease.x-(nSamples/2)+i, ptLCrelease.y-(nSamples/2)+j);
      if(i == (nSamples/2) || j == (nSamples/2)){
        //ligthen tose pixels
        cPixel = RGB(GetRValue(cPixel)*1.7, GetGValue(cPixel)*1.7, GetBValue(cPixel)*1.7);
      }
      if(ptLCrelease.x>rcClient.right/2){ dMagnefier.x = -rcClient.right/10; }
      if(ptLCrelease.x<rcClient.right/2){ dMagnefier.x = rcClient.right/10; }
      if(ptLCrelease.y>rcClient.bottom/2){ dMagnefier.y = -rcClient.bottom/10; }
      if(ptLCrelease.y<rcClient.bottom/2){ dMagnefier.y = rcClient.bottom/10; }
      paintPixels(hdcMem, dMagnefier.x, dMagnefier.y, cPixel, i, j);
    }
  }

  char strMsg[MAX_PATH];
  strcpy(strMsg, _T("X: "));
  std::string var = std::to_string(ptLCrelease.x);
  strcat(strMsg, var.c_str());

  strcat(strMsg, _T(" Y:  "));
  var = std::to_string(ptLCrelease.y);
  strcat(strMsg, var.c_str());

  TextOut(hdcMem, ptLCrelease.x-3*CZOOMFACTOR+dMagnefier.x,
      ptLCrelease.y+(nSamples/2)*CZOOMFACTOR+dMagnefier.y, strMsg, _tcslen(strMsg));
  //ReleaseDC(hWnd, hdcWindow);
}

//xOffset = position x of the current pixel (normal scale)
//dx and dy are x and y positions where the magnefier should be
void paintPixels(HDC hdcWindow, int dx, int dy, COLORREF cPixel, int xOffset, int yOffset){
  //int i,j;
  int nSamples= NSAMPLESZOOM;
  int posx, posy;
  bool fCenter = false;

  if((xOffset == (nSamples/2)) && (yOffset == (nSamples/2))){
    //Center square of the magnifier
    fCenter = true;
  }

  /*
  for(i=0;i<CZOOMFACTOR;i++){ //lines
    for(j=0;j<CZOOMFACTOR;j++){ //columns
      posx = ptLCrelease.x-(nSamples/2)*CZOOMFACTOR+xOffset*CZOOMFACTOR+dx+i;
      posy = ptLCrelease.y-(nSamples/2)*CZOOMFACTOR+yOffset*CZOOMFACTOR+dy+j;
      SetPixel(hdcWindow, posx, posy, cPixel);
    }
  }
  */

  const int
    width = CZOOMFACTOR,
    height = CZOOMFACTOR,
    size = width * height * 3;
  byte * data;
  //data = new byte[size];
  data = (byte*) malloc (size*sizeof(byte));
  for (int i = 0; i < size; i += 3){
    //each line/column has CZOOMFACTOR*3-1 bytes or CZOOMFACTOR pixels
    if(fCenter&&(((i>=0)&&(i<=(width*3-1)))||(i>=(height-1)*width*3 && i<=(height-1)*(width*4-1)))){
      data[i] = 255;
      data[i + 1] = 255;
      data[i + 2] = 255;
    }else if(
        (fCenter)
        &&(
          ((i % (width*3)) == 0)
          ||
          ((i % (width*3)) == 33)
        )){
      data[i] = 255;
      data[i + 1] = 255;
      data[i + 2] = 255;
    }else{
      //seems like the DIB is printing BGR instead of RGB...
      data[i] = GetBValue(cPixel);
      data[i + 1] = GetGValue(cPixel);
      data[i + 2] = GetRValue(cPixel);
    }
  }

  BITMAPINFOHEADER bmih;
  bmih.biBitCount = 24;
  bmih.biClrImportant = 0;
  bmih.biClrUsed = 0;
  bmih.biCompression = BI_RGB;
  bmih.biWidth = width;
  bmih.biHeight = height;
  bmih.biPlanes = 1;
  bmih.biSize = 40;
  bmih.biSizeImage = size;

  BITMAPINFO bmpi;
  bmpi.bmiHeader = bmih;

  posx = ptLCrelease.x-((nSamples/2)*CZOOMFACTOR)+(xOffset*CZOOMFACTOR)+dx;
  posy = ptLCrelease.y-(nSamples/2)*CZOOMFACTOR+yOffset*CZOOMFACTOR+dy;

  SetDIBitsToDevice(hdcWindow, posx, posy, width, height, 0, 0, 0, height, data, &bmpi, DIB_RGB_COLORS);

  //delete[] data;
  free(data);
}

//return the BITMAP loaded in the device context (hdc)
bool LoadBMPIntoDC(HDC hDC, LPCTSTR bmpfile, HWND hWnd){

  // check if params are valid
	if ( ( NULL == hDC  ) || ( NULL == bmpfile ) )
		return false;

	// load bitmap into a bitmap handle
	HANDLE hBmp = LoadImage ( NULL, bmpfile, IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE );

	if ( NULL == hBmp )
		return false;        // failed to load image

	// bitmaps can only be selected into memory dcs:
	HDC dcmem = CreateCompatibleDC ( NULL );

	// now select bitmap into the memory dc
	if ( NULL == SelectObject ( dcmem, hBmp ) )
	{	// failed to load bitmap into device context
		DeleteDC ( dcmem );
		return false;
	}

	// now get the bmp size
	BITMAP bm;
	GetObject ( hBmp, sizeof(bm), &bm );

  drawMagnefier(hWnd, dcmem);

	// and blit it to the visible dc
	if ( BitBlt ( hDC, 0, 0, bm.bmWidth, bm.bmHeight, dcmem,
		0, 0, SRCCOPY ) == 0 )
	{	// failed the blit
		DeleteDC ( dcmem );
		return false;
	}

  DeleteObject(hBmp); // clear memory form the handle which has the loaded image
	DeleteDC(dcmem);  // clear up the memory dc
  return true;
}

//a Painting Function
int CaptureAnImage(HWND hWnd)
{
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HDC hdcScreen;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    std::string x, y;

    // Retrieve the handle to a display device context for the client
    // area of the window.
    hdcScreen = GetDC(NULL);
    hdcWindow = GetDC(hWnd);

    // Create a compatible DC which is used in a BitBlt from the window DC
    hdcMemDC = CreateCompatibleDC(hdcWindow);

    if(!hdcMemDC)
    {
        MessageBox(hWnd, _T("CreateCompatibleDC has failed"), _T("Failed"), MB_OK);
        goto done;
    }

    // Get the client area for size calculation
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    //This is the best stretch mode
    //SetStretchBltMode(hdcWindow,HALFTONE);

    /*
    char strMsg[MAX_PATH];
    strcpy(strMsg, "rcClient.right: ");
    x = std::to_string(rcClient.right);
    strcat(strMsg, x.c_str());

    strcat(strMsg, _T("\n rcClient.bottom: "));
    y = std::to_string(rcClient.bottom);
    strcat(strMsg, y.c_str());

    TextOut(hdc, 5, 20, strMsg, _tcslen(greeting));
    */


    //The source DC is the entire screen and the destination DC is the current window (HWND)
    //if(!StretchBlt(hdcWindow,
    if(!BitBlt(hdcWindow,
               0, 0,
               rcClient.right, rcClient.bottom,
               hdcScreen, 0, 0,
               SRCCOPY))
    {
        MessageBox(hWnd, _T("StretchBlt has failed"), _T("Failed"), MB_OK);
        goto done;
    }
    fprtScreen = true;

    // Create a compatible bitmap from the Window DC
    hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);

    if(!hbmScreen)
    {
        MessageBox(hWnd, _T("CreateCompatibleBitmap Failed"), _T("Failed"), MB_OK);
        goto done;
    }

    // Select the compatible bitmap into the compatible memory DC.
    SelectObject(hdcMemDC,hbmScreen);

    // Bit block transfer into our compatible memory DC.
    if(!BitBlt(hdcMemDC,
               0,0,
               rcClient.right, rcClient.bottom,
               hdcWindow,
               0,0,
               SRCCOPY))
    {
        MessageBox(hWnd, _T("BitBlt has failed"), _T("Failed"), MB_OK);
        goto done;
    }


    // Get the BITMAP from the HBITMAP
    GetObject(hbmScreen,sizeof(BITMAP),&bmpScreen);

    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that
    // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc
    // have greater overhead than HeapAlloc.
    HANDLE hDIB = GlobalAlloc(GHND,dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);

    // Gets the "bits" from the bitmap and copies them into a buffer
    // which is pointed to by lpbitmap.
    GetDIBits(hdcWindow, hbmScreen, 0,
        (UINT)bmpScreen.bmHeight,
        lpbitmap,
        (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    // A file is created, this is where we will save the screen capture.
    HANDLE hFile = CreateFile(_T("captureqwsx.bmp"),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    // Add the size of the headers to the size of the bitmap to get the total file size
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    //Size of the file
    bmfHeader.bfSize = dwSizeofDIB;

    //bfType must always be BM for Bitmaps
    bmfHeader.bfType = 0x4D42; //BM

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    //Unlock and Free the DIB from the heap
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    //Close the handle for the file that was created
    CloseHandle(hFile);

    //Clean up
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(NULL,hdcScreen);
    ReleaseDC(hWnd,hdcWindow);

    return 0;
}

static void getScreenReso(uv_work_t *req){

  Work *work = static_cast<Work*>(req->data);
  HINSTANCE module = GetModuleHandle(NULL);

  char *text = _T("getScreen");
  size_t size = strlen(text)+1;
  wchar_t *wtext = new wchar_t[size];
  size_t outSize;

  mbstowcs_s(&outSize, wtext, size, text, size-1);
  //LPSTR pcmdLine = wtext;
  winmain(module, NULL, text, SW_SHOWMAXIMIZED, &work->request);
}

void sendSygnalResolution(uv_async_t *handle) {
  Isolate* isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);

  sygData *pobj = ((sygData*) handle->data);
  uv_work_t req = ((uv_work_t) pobj->req);
  Work *work = static_cast<Work*> (req.data);

  Local<Object> obj = Object::New(isolate);
  obj->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, pobj->data.x));
  obj->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, pobj->data.y));
  obj->Set(String::NewFromUtf8(isolate, "w"), Number::New(isolate, pobj->data.w));
  obj->Set(String::NewFromUtf8(isolate, "h"), Number::New(isolate, pobj->data.h));

  Handle<Value> argv[] = {obj};
  //execute the callback
  Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  //Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void getScreenResoAsync(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();

  Work* work = new Work();
  work->request.data = work;

  //store the callback from JS in the work package to invoke later
  Local<Function> callback = Local<Function>::Cast(args[0]);
  work->callback.Reset(isolate, callback);

  //worker thread using libuv
  uv_async_init(uv_default_loop(), &work->async, sendSygnalResolution);
  uv_queue_work(uv_default_loop(), &work->request, getScreenReso, NULL);

  args.GetReturnValue().Set(Undefined(isolate));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "getScreenResolution", getScreenResoAsync);
}

NODE_MODULE(sharex, init)

}  // namespace windowAddon
