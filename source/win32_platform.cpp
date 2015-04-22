#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static 
#define global_variable static 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

// TODO: Global, make it not a global
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

internal void
RenderTrippy(win32_offscreen_buffer Buffer, int XOffset, int YOffset) {
    int Width = Buffer.Width;
    int Height = Buffer.Height;

    int Pitch = Width * Buffer.BytesPerPixel;
    uint8 *Row = (uint8 *)Buffer.Memory;
    for(int Y = 0;
	Y < Buffer.Height;
	++Y)
    {
	uint32 *Pixel = (uint32 *)Row;
	for(int X = 0;
	    X < Buffer.Width;
	    ++X)
	{
	    /*
	    Little Endian so
	    0x xxRRGGBB
	    */
	    uint8 Blue = (X + XOffset);
	    uint8 Green = (Y + YOffset);
	    uint8 Red = 0;
	    
	    *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
	}

	Row += Buffer.Pitch;
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);

    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
	VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * Buffer->BytesPerPixel;

    // TODO: Clear to black

}

internal void
Win32UpdateBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height)
{
    // TODO: Aspect ration correction

    StretchDIBits(DeviceContext,
		  //X, Y, Width, Height,
		  //X, Y, Width, Height,
		  0, 0, WindowWidth, WindowHeight,
		  0, 0, Buffer.Width, Buffer.Height,
		  Buffer.Memory,
		  &Buffer.Info,
		  DIB_RGB_COLORS,
		  SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
			UINT Message,
			WPARAM WParam,
			LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
	{
	} break;
	    
        case WM_CLOSE:
	{
	    // TODO: Send a confirmation message to the user.
	    Running = false;
	} break;

        case WM_DESTROY:
	{
	    // TODO: Handle as an error. Perhaps relaunch window.
	    Running = false;
	} break;
	
        case WM_ACTIVATEAPP:
	{
	    // TODO: Handle focus
	} break;

        case WM_PAINT:
	{
	    PAINTSTRUCT Paint;
	    HDC DeviceContext = BeginPaint(Window, &Paint);
	    int X = Paint.rcPaint.left;
	    int Y = Paint.rcPaint.top;
	    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
	    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
	    
	    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	    Win32UpdateBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, X, Y, Width, Height);
	    EndPaint(Window, &Paint);
	}
	
        default:
	{
	    Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
    }

    return(Result);
}

int CALLBACK 
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int CmdShow) 
{
    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;    
    // WindowClass.hIcon
    WindowClass.lpszClassName = "WindowClass";

    if(RegisterClass(&WindowClass)) 
    {
	HWND Window = 
	    CreateWindowExA(
			   0,
			   WindowClass.lpszClassName,
			   "GameName",
			   WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			   CW_USEDEFAULT,
			   CW_USEDEFAULT,
			   CW_USEDEFAULT,
			   CW_USEDEFAULT,
			   0,
			   0,
			   Instance,
			   0);
	if(Window)
	{
	    Running = true;

	    int XOffset = 0;
	    int YOffset = 0;

	    while(Running)
	    {
		MSG Message;
 
		while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
		    if(Message.message == WM_QUIT)
		    {
			Running = false;
		    }

		    TranslateMessage(&Message);
		    DispatchMessageA(&Message);
		}

		RenderTrippy(GlobalBackbuffer, XOffset, YOffset);
		
		HDC DeviceContext = GetDC(Window);
		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32UpdateBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
		ReleaseDC(Window, DeviceContext);

		++XOffset;
		YOffset += 2;
	    }    
	}
	else
	{
	    // TODO: Log errors
	}
    }
    else 
    {
	// TODO: Log errors
    }
    
    return(0);
}
