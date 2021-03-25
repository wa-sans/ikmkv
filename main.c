#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdbool.h>
#include <math.h>

HWND hwnd;

static char g_szClassName[] = "MyWindowClass";
static HINSTANCE g_hInst = NULL;

const int TimerIdUpdate = 1;
UINT TimerDelayUpdate = 8;

const int TimerIdDraw = 2;
UINT TimerDelayDraw = 16;

double IkmIndex=2;
char *keyQueue;

HBITMAP ikmBitmap[6];

int windowWidth=512;
int windowHeight=512;

bool isWindowMove=false;
int windowMoveBaseMouseX=0;
int windowMoveBaseMouseY=0;

void DrawIkm(HDC hdc)
{
   HDC hdcMemory;
   hdcMemory = CreateCompatibleDC(hdc);

   SelectObject(hdcMemory, ikmBitmap[(int)floor(IkmIndex)]);
   StretchBlt(hdc, 0, 0, windowWidth, windowHeight, hdcMemory, 0, 0, 512,512,SRCCOPY);

   DeleteDC(hdcMemory);
}

bool keyQueueLock=false;

void UpdateIkm(HWND hwnd)
{
   while(keyQueueLock);
   keyQueueLock=true;
   int size=_msize(keyQueue);
   int length=size;
   for(int i=0;i<size;i++)
   {
      if(keyQueue[i]==0)
      {
         length=i;
         break;
      }
   }
   
   bool isNext=!(floor(IkmIndex)==5||floor(IkmIndex)==2);
   if(isNext&&length>1)
   {
      IkmIndex=fmod(IkmIndex+3*length,sizeof(ikmBitmap)/sizeof(HBITMAP));
      free(keyQueue);
      keyQueue=calloc(sizeof(char),1);
   }
   else if(isNext)
   {
      IkmIndex=fmod((IkmIndex+1),sizeof(ikmBitmap)/sizeof(HBITMAP));
   }
   else if(length>0)
   {
      for(int i=0;i<size-1;i++)
      {
         keyQueue[i]=keyQueue[i+1];
      }
      keyQueue[size-1]=0;
      IkmIndex=fmod((IkmIndex+1),sizeof(ikmBitmap)/sizeof(HBITMAP));
   }
   keyQueueLock=false;
}

HHOOK hHookKeyboard,hHookMouse;

bool pressKey[256]={false,};

void KeyboardWork(int k,int o)
{
   if(o==WM_KEYDOWN&&pressKey[k]==false)
   {
      pressKey[k]=true;
      while(keyQueueLock);
      keyQueueLock=true;
      int size=_msize(keyQueue);
      bool isSet=false;
      for(int i=0;i<size;i++)
      {
         if(keyQueue[i]==0)
         {
            keyQueue[i]=1;
            isSet=true;
            break;
         }
      }
      if(!isSet)
      {
         keyQueue=realloc(keyQueue,size+sizeof(char));
         keyQueue[size]=1;
      }
      keyQueueLock=false;
   }
   if(o==WM_KEYUP)
   {
      pressKey[k]=false;
   }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
   PKBDLLHOOKSTRUCT pKey = (PKBDLLHOOKSTRUCT)lParam;
   if (nCode >= 0)
   {
      KeyboardWork((int)pKey->vkCode,wParam);
   }
   return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
   MOUSEHOOKSTRUCT* pMouse = (MOUSEHOOKSTRUCT*)lParam;
   switch(wParam)
   {
      case WM_MOUSEMOVE:
         if(isWindowMove)
         {
            MoveWindow(hwnd,pMouse->pt.x-windowMoveBaseMouseX,pMouse->pt.y-windowMoveBaseMouseY,windowWidth,windowHeight,true);
         }
      break;
      case WM_LBUTTONUP:
         isWindowMove=false;
      break; 
   }
   
   return CallNextHookEx(hHookMouse, nCode, wParam, lParam);;
}
 
void SetHook()
{
   HMODULE hInstance = GetModuleHandle(NULL);
   hHookKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, NULL);
   hHookMouse = SetWindowsHookEx( WH_MOUSE_LL, MouseProc, hInstance, NULL );
}
 
void UnHook()
{
   UnhookWindowsHookEx(hHookKeyboard);
   UnhookWindowsHookEx(hHookMouse);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
   switch(Message)
   {
      case WM_CREATE:
         for(int i=0;i<sizeof(ikmBitmap)/sizeof(HBITMAP);i++)
         {
            char *buff=calloc(snprintf(NULL,0,"FRAME%dBMP",i)+1,sizeof(char));
            sprintf(buff,"FRAME%dBMP",i);
            ikmBitmap[i] = LoadBitmap(g_hInst,buff);
            if(!ikmBitmap[i])
            {
               buff=calloc(snprintf(NULL,0,"Load of %s failed.",buff)+1,sizeof(char));
               MessageBox(hwnd,buff, "Error", MB_OK | MB_ICONEXCLAMATION);
               exit(1);
               return -1;
            }
         }
         SetTimer(hwnd, TimerIdUpdate, TimerDelayUpdate, NULL);
         SetTimer(hwnd, TimerIdDraw, TimerDelayDraw, NULL);
         /*
         AllocConsole();
         _tfreopen("CONOUT$","w",stdout);
         _tfreopen("CONIN$","r",stdin);
         _tfreopen("CONERR$","w",stderr);*/
      break;
      case WM_TIMER:
         {
            //printf("%d\n",wParam);
            switch(wParam)
            {
               case 1:
                  UpdateIkm(hwnd);
               break;
               case 2:
                  {
                     HDC hdcWindow;
                     hdcWindow = GetDC(hwnd);
            
                     DrawIkm(hdcWindow);
            
                     ReleaseDC(hwnd, hdcWindow);
                  }
               break;
            }
         }
      break;
      case WM_LBUTTONDOWN:
         {
            POINT p;
            GetCursorPos(&p);
            isWindowMove=true;
            RECT r;
            GetWindowRect(hwnd,&r);
            windowMoveBaseMouseX=p.x-r.left;
            windowMoveBaseMouseY=p.y-r.top;
         }
      break;
      case WM_MOUSEWHEEL:
         {
            short wheelSpeed=(SHORT)HIWORD(wParam);
            POINT p;
            GetCursorPos(&p);
            RECT r;
            GetWindowRect(hwnd,&r);
            //printf("%d ",r.left);
            /*
            double posX=(double)(p.x-r.left)/windowWidth;
            double posY=(double)(p.y-r.top)/windowHeight;*/
            windowWidth+=wheelSpeed;
            windowHeight+=wheelSpeed;
            windowWidth=max(windowWidth,60);
            windowHeight=max(windowHeight,60);
            //printf("%d\n",r.left);
            if(!(r.right-r.left==windowWidth&&r.bottom-r.top==windowHeight))
            {
               MoveWindow(hwnd,r.left-(p.x-r.left)*((double)windowWidth/(windowWidth-wheelSpeed)-1),r.top-(p.y-r.top)*((double)windowHeight/(windowHeight-wheelSpeed)-1),windowWidth,windowHeight,true);
            }
         }
      break;
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdcWindow;
            hdcWindow = BeginPaint(hwnd, &ps);
   
            DrawIkm(hdcWindow);
               
            EndPaint(hwnd, &ps);
         }
      break;
      case WM_CLOSE:
         //FreeConsole();
         DestroyWindow(hwnd);
      break;
      case WM_DESTROY:
         KillTimer(hwnd, TimerDelayUpdate);
         KillTimer(hwnd, TimerDelayDraw);
         
         for(int i=0;i<sizeof(ikmBitmap)/sizeof(ikmBitmap);i++)
         {
            DeleteObject(ikmBitmap[i]);
         }
         PostQuitMessage(0);
      break;
      default:
         return DefWindowProc(hwnd, Message, wParam, lParam);
   }
   return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
   LPSTR lpCmdLine, int nCmdShow)
{
   WNDCLASSEX WndClass;
   MSG Msg;

   g_hInst = hInstance;

   WndClass.cbSize        = sizeof(WNDCLASSEX);
   WndClass.style         = 0;
   WndClass.lpfnWndProc   = WndProc;
   WndClass.cbClsExtra    = 0;
   WndClass.cbWndExtra    = 0;
   WndClass.hInstance     = g_hInst;
   WndClass.hIcon         = LoadIcon(hInstance, "A");
	WndClass.hIconSm       = LoadIcon(hInstance, "A");
   WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
   WndClass.lpszMenuName  = NULL;
   WndClass.lpszClassName = g_szClassName;
   WndClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

   if(!RegisterClassEx(&WndClass))
   {
      MessageBox(0, "Window Registration Failed!", "Error!",
         MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
      return 0;
   }
   
   keyQueue=calloc(sizeof(char),1);

   hwnd = CreateWindowEx(
      WS_EX_TOPMOST|WS_EX_LAYERED,
      g_szClassName,
      "Anonymous Keyviewer",
      WS_VISIBLE|WS_POPUP,
      CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
      NULL, NULL, g_hInst, NULL);

   if(hwnd == NULL)
   {
      MessageBox(0, "Window Creation Failed!", "Error!",
         MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
      return 0;
   }
   
   SetLayeredWindowAttributes(hwnd,RGB(0,255,0),0,LWA_COLORKEY);
   
   SetHook();

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   while(GetMessage(&Msg, NULL, 0, 0))
   {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
   }
   
   UnHook();
   
   return Msg.wParam;
}

