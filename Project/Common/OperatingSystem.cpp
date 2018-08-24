////////////////////////////////////////////////////////////////////////////////
// Copyright 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <chrono>
#include "OperatingSystem.h"

namespace ApiWithoutSecrets {

  namespace OS {

    ProjectBase::ProjectBase() :
      CanRender( false ) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VulkanLibrary = LoadLibrary( "vulkan-1.dll" );
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
      VulkanLibrary = dlopen( "libvulkan.so.1", RTLD_NOW );
#endif
    }

    ProjectBase::~ProjectBase() {
          if( VulkanLibrary ) {
      #if defined(VK_USE_PLATFORM_WIN32_KHR)
            FreeLibrary( VulkanLibrary );
      #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
            dlclose( VulkanLibrary );
      #endif
          }
    }

    bool ProjectBase::ReadyToDraw() const {
      return CanRender;
    }

    ProjectBase::MouseInputStateData::MouseInputStateData() {
      Buttons[0].IsPressed = false;
      Buttons[0].WasClicked = false;
      Buttons[0].WasRelease = false;
      Buttons[1].IsPressed = false;
      Buttons[1].WasClicked = false;
      Buttons[1].WasRelease = false;
      Position.X = 0;
      Position.Y = 0;
      Position.Delta.X = 0;
      Position.Delta.Y = 0;
      Available = true;
    }

    ProjectBase::MouseInputStateData::~MouseInputStateData() {
    }

    void ProjectBase::MouseMove( int x, int y ) {
      MouseState.Position.Delta.X = x - MouseState.Position.X;
      MouseState.Position.Delta.Y = y - MouseState.Position.Y;
      MouseState.Position.X = x;
      MouseState.Position.Y = y;
    }

    void ProjectBase::MouseClick( size_t button, bool pressed ) {
      if( button < 2 ) {
        MouseState.Buttons[button].IsPressed = pressed;
        MouseState.Buttons[button].WasClicked = pressed;
        MouseState.Buttons[button].WasRelease = !pressed;
      }
    }

    void ProjectBase::ResetMouse() {
      MouseState.Position.Delta.X = 0;
      MouseState.Position.Delta.Y = 0;
      MouseState.Buttons[0].WasClicked = false;
      MouseState.Buttons[0].WasRelease = false;
      MouseState.Buttons[1].WasClicked = false;
      MouseState.Buttons[1].WasRelease = false;
      MouseState.Available = true;
    }

    Window::Window() :
      Parameters() {
    }

    WindowParameters Window::GetParameters() const {
      return Parameters;
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)

    namespace {
#define SERIES_NAME "API without Secrets: Introduction to Vulkan"

      enum UserMessage {
        USER_MESSAGE_RESIZE = WM_USER + 1,
        USER_MESSAGE_QUIT,
        USER_MESSAGE_MOUSE_MOVE,
        USER_MESSAGE_MOUSE_CLICK
      };
    }

    LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
      switch( message ) {
      case WM_SIZE:
      case WM_EXITSIZEMOVE:
        PostMessage( hWnd, USER_MESSAGE_RESIZE, wParam, lParam );
        break;
      case WM_KEYDOWN:
      case WM_CLOSE:
        PostMessage( hWnd, USER_MESSAGE_QUIT, wParam, lParam );
        break;
      case WM_MOUSEMOVE:
        PostMessage( hWnd, USER_MESSAGE_MOUSE_MOVE, LOWORD( lParam ), HIWORD( lParam ) );
        break;
      case WM_LBUTTONDOWN:
        PostMessage( hWnd, USER_MESSAGE_MOUSE_CLICK, 0, 1 );
        break;
      case WM_LBUTTONUP:
        PostMessage( hWnd, USER_MESSAGE_MOUSE_CLICK, 0, 0 );
        break;
      case WM_RBUTTONDOWN:
        PostMessage( hWnd, USER_MESSAGE_MOUSE_CLICK, 1, 1 );
        break;
      case WM_RBUTTONUP:
        PostMessage( hWnd, USER_MESSAGE_MOUSE_CLICK, 1, 0 );
        break;
      default:
        return DefWindowProc( hWnd, message, wParam, lParam );
      }
      return 0;
    }

    Window::~Window() {
      if( Parameters.Handle ) {
        DestroyWindow( Parameters.Handle );
      }

      if( Parameters.Instance ) {
        UnregisterClass( SERIES_NAME, Parameters.Instance );
      }
    }

    void Window::Create( const char * title, int width, int height ) {
      Parameters.Instance = GetModuleHandle( nullptr );

      // Register window class
      WNDCLASSEX wcex;

      wcex.cbSize = sizeof(WNDCLASSEX);

      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = WndProc;
      wcex.cbClsExtra = 0;
      wcex.cbWndExtra = 0;
      wcex.hInstance = Parameters.Instance;
      wcex.hIcon = NULL;
      wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
      wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
      wcex.lpszMenuName = NULL;
      wcex.lpszClassName = SERIES_NAME;
      wcex.hIconSm = NULL;

      if( !RegisterClassEx( &wcex ) ) {
        throw std::exception( "Could not register window class!" );
      }

      // Create window
      Parameters.Handle = CreateWindow( SERIES_NAME, title, WS_OVERLAPPEDWINDOW, 20, 20, width, height, nullptr, nullptr, Parameters.Instance, nullptr );
      if( !Parameters.Handle ) {
        throw std::exception( "Could not create application window!" );
      }
    }

    void Window::RenderingLoop( ProjectBase &project ) const {
      // Display window
      ShowWindow( Parameters.Handle, SW_SHOWNORMAL );
      UpdateWindow( Parameters.Handle );

      // Main message loop
      MSG message;
      bool loop = true;
      bool resize = false;

      while( loop ) {
        if( PeekMessage( &message, NULL, 0, 0, PM_REMOVE ) ) {
          // Process events
          switch( message.message ) {
          // Resize
          case USER_MESSAGE_RESIZE:
            resize = true;
            break;
          // Close
          case USER_MESSAGE_QUIT:
            loop = false;
            break;
          // Mouse move
          case USER_MESSAGE_MOUSE_MOVE:
            project.MouseMove( static_cast<int>(message.wParam), static_cast<int>(message.lParam) );
            break;
          // Mouse click
          case USER_MESSAGE_MOUSE_CLICK:
            project.MouseClick( static_cast<size_t>(message.wParam), message.lParam > 0 );
            break;
          }
          TranslateMessage( &message );
          DispatchMessage( &message );
        } else {
          // Resize
          if( resize ) {
            resize = false;
            project.OnWindowSizeChanged();
          }
          // Draw
          if( project.ReadyToDraw() ) {
            project.Draw();
          } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
          }
          // Reset mouse state
          project.ResetMouse();
        }
      }
    }

#elif defined(VK_USE_PLATFORM_XCB_KHR)

    Window::~Window() {
      xcb_destroy_window( Parameters.Connection, Parameters.Handle );
      xcb_disconnect( Parameters.Connection );
    }

    bool Window::Create( const char *title, int width, int height ) {
      int screen_index;
      Parameters.Connection = xcb_connect( nullptr, &screen_index );

      if( !Parameters.Connection ) {
        return false;
      }

      const xcb_setup_t *setup = xcb_get_setup( Parameters.Connection );
      xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator( setup );

      while( screen_index-- > 0 ) {
        xcb_screen_next( &screen_iterator );
      }

      xcb_screen_t *screen = screen_iterator.data;

      Parameters.Handle = xcb_generate_id( Parameters.Connection );

      uint32_t value_list[] = {
        screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
      };

      xcb_create_window(
        Parameters.Connection,
        XCB_COPY_FROM_PARENT,
        Parameters.Handle,
        screen->root,
        20,
        20,
        width,
        height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
        value_list );

      xcb_flush( Parameters.Connection );

      xcb_change_property(
        Parameters.Connection,
        XCB_PROP_MODE_REPLACE,
        Parameters.Handle,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen( title ),
        title );

      return true;
    }

    bool Window::RenderingLoop( ProjectBase &project ) const {
      // Prepare notification for window destruction
      xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom( Parameters.Connection, 1, 12, "WM_PROTOCOLS" );
      xcb_intern_atom_reply_t  *protocols_reply  = xcb_intern_atom_reply( Parameters.Connection, protocols_cookie, 0 );
      xcb_intern_atom_cookie_t  delete_cookie    = xcb_intern_atom( Parameters.Connection, 0, 16, "WM_DELETE_WINDOW" );
      xcb_intern_atom_reply_t  *delete_reply     = xcb_intern_atom_reply( Parameters.Connection, delete_cookie, 0 );
      xcb_change_property( Parameters.Connection, XCB_PROP_MODE_REPLACE, Parameters.Handle, (*protocols_reply).atom, 4, 32, 1, &(*delete_reply).atom );
      free( protocols_reply );

      // Display window
      xcb_map_window( Parameters.Connection, Parameters.Handle );
      xcb_flush( Parameters.Connection );

      // Main message loop
      xcb_generic_event_t *event;
      bool loop = true;
      bool resize = false;
      bool result = true;

      while( loop ) {
        event = xcb_poll_for_event( Parameters.Connection );

        if( event ) {
          // Process events
          switch (event->response_type & 0x7f) {
            // Resize
          case XCB_CONFIGURE_NOTIFY: {
              xcb_configure_notify_event_t *configure_event = (xcb_configure_notify_event_t*)event;
              static uint16_t width = configure_event->width;
              static uint16_t height = configure_event->height;

              if( ((configure_event->width > 0) && (width != configure_event->width)) ||
                ((configure_event->height > 0) && (height != configure_event->height)) ) {
                resize = true;
                width = configure_event->width;
                height = configure_event->height;
              }
            }
            break;
            // Close
          case XCB_CLIENT_MESSAGE:
            if( (*(xcb_client_message_event_t*)event).data.data32[0] == (*delete_reply).atom ) {
              loop = false;
              free( delete_reply );
            }
            break;
          case XCB_KEY_PRESS:
            loop = false;
            break;
          }
          free( event );
        } else {
          // Draw
          if( resize ) {
            resize = false;
            if( !project.OnWindowSizeChanged() ) {
              result = false;
              break;
            }
          }
          if( project.ReadyToDraw() ) {
            if( !project.Draw() ) {
              result = false;
              break;
            }
          } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
          }
        }
      }

      return result;
    }

#elif defined(VK_USE_PLATFORM_XLIB_KHR)

    Window::~Window() {
      XDestroyWindow( Parameters.DisplayPtr, Parameters.Handle );
      XCloseDisplay( Parameters.DisplayPtr );
    }

    bool Window::Create( const char *title, int width, int height ) {
      Parameters.DisplayPtr = XOpenDisplay( nullptr );
      if( !Parameters.DisplayPtr ) {
        return false;
      }

      int default_screen = DefaultScreen( Parameters.DisplayPtr );

      Parameters.Handle = XCreateSimpleWindow(
        Parameters.DisplayPtr,
        DefaultRootWindow( Parameters.DisplayPtr ),
        20,
        20,
        width,
        height,
        1,
        BlackPixel( Parameters.DisplayPtr, default_screen ),
        WhitePixel( Parameters.DisplayPtr, default_screen ) );

      // XSync( Parameters.DisplayPtr, false );
      XSetStandardProperties( Parameters.DisplayPtr, Parameters.Handle, title, title, None, nullptr, 0, nullptr );
      XSelectInput( Parameters.DisplayPtr, Parameters.Handle, ExposureMask | KeyPressMask | StructureNotifyMask );

      return true;
    }

    bool Window::RenderingLoop( ProjectBase &project ) const {
      // Prepare notification for window destruction
      Atom delete_window_atom;
      delete_window_atom = XInternAtom( Parameters.DisplayPtr, "WM_DELETE_WINDOW", false );
      XSetWMProtocols( Parameters.DisplayPtr, Parameters.Handle, &delete_window_atom, 1);

      // Display window
      XClearWindow( Parameters.DisplayPtr, Parameters.Handle );
      XMapWindow( Parameters.DisplayPtr, Parameters.Handle );

      // Main message loop
      XEvent event;
      bool loop = true;
      bool resize = false;
      bool result = true;

      while( loop ) {
        if( XPending( Parameters.DisplayPtr ) ) {
          XNextEvent( Parameters.DisplayPtr, &event );
          switch( event.type ) {
            //Process events
          case ConfigureNotify: {
              static int width = event.xconfigure.width;
              static int height = event.xconfigure.height;

              if( ((event.xconfigure.width > 0) && (event.xconfigure.width != width)) ||
                ((event.xconfigure.height > 0) && (event.xconfigure.width != height)) ) {
                width = event.xconfigure.width;
                height = event.xconfigure.height;
                resize = true;
              }
            }
            break;
          case KeyPress:
            loop = false;
            break;
          case DestroyNotify:
            loop = false;
            break;
          case ClientMessage:
            if( static_cast<unsigned int>(event.xclient.data.l[0]) == delete_window_atom ) {
              loop = false;
            }
            break;
          }
        } else {
          // Draw
          if( resize ) {
            resize = false;
            if( !project.OnWindowSizeChanged() ) {
              result = false;
              break;
            }
          }
          if( project.ReadyToDraw() ) {
            if( !project.Draw() ) {
              result = false;
              break;
            }
          } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
          }
        }
      }

      return result;
    }

#endif

  } // namespace OS

} // namespace ApiWithoutSecrets
