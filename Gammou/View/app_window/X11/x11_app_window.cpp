

#include <cstring>
#include <chrono>

#include "x11_app_window.h"

#define GAMMOU_X_EVENT_MASK KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | \
                            LeaveWindowMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | \
                            EnterWindowMask

namespace Gammou {

    namespace View {

        x11_app_window::x11_app_window(const unsigned int px_width, const unsigned int px_height)
            : abstract_app_window(px_width, px_height),
                m_running(false)
        {
        }

        x11_app_window::~x11_app_window()
        {
            close();
        }

        void x11_app_window::open()
        {
            if (m_running)
                return;

            const unsigned int px_width = get_width();
            const unsigned int px_height = get_height();

            m_display = XOpenDisplay(nullptr);

            if( m_display == nullptr )
                throw std::runtime_error("Unable to open X Display");
            
            // Check we can use Xbde
            int minor_version = 0, major_version = 0;
            if(!XdbeQueryExtension(m_display, &major_version, &minor_version))
                throw std::runtime_error("Xbde unsuported\n");


            int screen_count = 1;
            m_root_window = DefaultRootWindow(m_display);

            XdbeScreenVisualInfo *info = 
                XdbeGetVisualInfo(m_display, &m_root_window, &screen_count);

            if( info == nullptr || screen_count < 0 || info->count < 1)
                throw std::runtime_error("Xbde unsuported\n");
            
            XVisualInfo xvisual_info_template;

            xvisual_info_template.visualid = info->visinfo[0].visual;
            xvisual_info_template.screen = 0;
            xvisual_info_template.depth = info->visinfo[0].depth;

            int found_count = 0;
                
            m_xvisual_info_found
                = XGetVisualInfo(m_display,
                VisualIDMask | VisualScreenMask | VisualDepthMask, 
                &xvisual_info_template, &found_count);
            
            if( m_xvisual_info_found == nullptr || found_count < 1 )
                throw std::runtime_error("No Visual With Double Buffering\n");

            // Enbale window close event
            m_wm_delete_message = XInternAtom(m_display, "WM_DELETE_WINDOW", false);

            const int screen = DefaultScreen(m_display);

            XSetWindowAttributes xattributs;
            bzero(&xattributs, sizeof(XSetWindowAttributes));
            xattributs.background_pixel = WhitePixel(m_display, screen);

            m_window = XCreateWindow(
                m_display, m_root_window, 
                0, 0, px_width, px_height, 0, 
                CopyFromParent, CopyFromParent, 
                m_xvisual_info_found->visual, 
                CWBackPixel, &xattributs);
            
            // enable window close event
            XSetWMProtocols(m_display, m_window, &m_wm_delete_message, 1);

            // Prevent resizing (not handled on X11 now)
            XSizeHints constrain;
            std::memset(&constrain, 0, sizeof(constrain));

            constrain.flags = PMinSize | PMaxSize;
            constrain.max_width = px_width;
            constrain.min_width = px_width;
            constrain.max_height = px_height;
            constrain.min_height = px_height;

            XSetWMNormalHints(m_display, m_window, &constrain);

            // 

            m_back_buffer = XdbeAllocateBackBufferName(m_display, m_window, XdbeBackground);
            
            //--
            XSelectInput(m_display, m_window, GAMMOU_X_EVENT_MASK);
            XMapWindow(m_display, m_window);

            m_graphic_context = XCreateGC(m_display, m_back_buffer, 0, nullptr);
            XSetForeground(m_display, m_graphic_context, BlackPixel(m_display, screen));

            

            // to be sure that windows is mapped
            for(XEvent e; e.type != MapNotify; XNextEvent(m_display, &e)); 

            // Cairo
            
            m_cairo_surface = cairo_xlib_surface_create(
                m_display, m_back_buffer, 
                m_xvisual_info_found->visual,
                px_width, px_height
            );

            cairo_xlib_surface_set_size(
                m_cairo_surface,
                px_width, px_height
            );

            DEBUG_PRINT("Gui Thread Start\n");

            m_running = true;

            if (m_event_loop_thread.joinable())
                m_event_loop_thread.join();

            m_event_loop_thread = std::thread(x_event_loop, this);
        }

        void x11_app_window::close()
        {
            m_running = false;
            if (m_event_loop_thread.joinable())
                m_event_loop_thread.join();
        }

        bool x11_app_window::is_open()
        {
            return m_running;
        }

        bool x11_app_window::open_file(std::string& path, const std::string& title, const std::string& ext)
        {
            return false;
        }
        
        void x11_app_window::system_redraw_rect(const rectangle& rect)
        {
            if (!m_running)
                return;
                
            XEvent ev;
            std::memset(&ev, 0, sizeof(ev));

            ev.type = Expose;

            ev.xexpose.x = rect.x;
            ev.xexpose.y = rect.y;
            ev.xexpose.width = rect.width;
            ev.xexpose.height = rect.height;
            ev.xexpose.window = m_back_buffer;

            XSendEvent(m_display, m_window, true, ExposureMask, &ev);                      
        }

        // 
        void x11_app_window::redraw(void)
        {
            system_redraw_rect(rectangle(0, 0, get_width(), get_height()));
        }
        
        void x11_app_window::resize(const unsigned int width, const unsigned int height)
        {
            // Not Supported
            throw std::runtime_error("Windos Resizing is not supported with the X Backend\n");
        }

        void x11_app_window::x_event_loop(x11_app_window *self)
        {
            cairo_t *cr = cairo_create(self->m_cairo_surface);

            Time last_time = 0;

            while(self->m_running){
                XEvent event;
                XNextEvent(self->m_display, &event);

                switch( event.type ){

                    case ButtonPress:
            

                        switch(event.xbutton.button){

                            case 1: // left
                            self->sys_mouse_button_down(mouse_button::LeftButton);
                            break;

                            case 2: // wheel
                            self->sys_mouse_button_down(mouse_button::WheelButton);
                            break;

                            case 3: // right
                            self->sys_mouse_button_down(mouse_button::RightButton);
                            break;

                            case 4:
                            self->on_mouse_wheel(1.0f);
                            break;

                            case 5:
                            self->on_mouse_wheel(-1.0f);
                            break;

                        }
                        break;

                    case ButtonRelease:
                        {   
                            const unsigned int button = event.xbutton.button;

                            // Double Click Detection

                            if( button == 1 || button == 2 ){
                                Time now = event.xbutton.time;
                                const Time delta = now - last_time;
                                
                                //  
                                if( delta > 50 && delta < 250 )
                                    self->sys_mouse_dbl_click();
                                
                                last_time = now;
                            }
                            switch(button){

                                case 1: // left
                                self->sys_mouse_button_up(mouse_button::LeftButton);
                                break;

                                case 2: // wheel
                                self->sys_mouse_button_up(mouse_button::WheelButton);
                                break;

                                case 3: // right
                                self->sys_mouse_button_up(mouse_button::RightButton);
                                break;


                            }
                        }
                        break;

                    case KeyPress:
                        // todo
                        break;

                    case KeyRelease:
                        // todo
                        break;

                    case MotionNotify:
                    {
                        self->sys_mouse_move(
                            static_cast<unsigned int>(event.xmotion.x),
                            static_cast<unsigned int>(event.xmotion.y)
                        );
                    }
                        break;

                    case Expose:
                        self->sys_draw(cr);
                        cairo_surface_flush(self->m_cairo_surface);
                        
                        // Swap Buffer
                        XdbeSwapInfo info;
                        info.swap_window = self->m_window;
                        info.swap_action = XdbeBackground;
                        XdbeSwapBuffers(self->m_display, &info, 1);
                        XFlush(self->m_display);
                        break;

                    case EnterNotify:
                        self->sys_mouse_enter();
                        break;
                    
                    case LeaveNotify:
                        self->on_mouse_exit();
                        break;

                    case ClientMessage:
                        if (event.xclient.data.l[0] == self->m_wm_delete_message) {
                            DEBUG_PRINT("Received window delete xclient messge\n"); 
                            self->m_running = false;
                            
                        }
                        break;

                    default:
                        DEBUG_PRINT(" Unkxnow Event\n");
                        break;

                }
            }

            cairo_destroy(cr);
            cairo_surface_destroy(self->m_cairo_surface);
            XFreeGC(self->m_display, self->m_graphic_context);
            XDestroyWindow(self->m_display, self->m_window);
            XCloseDisplay(self->m_display);
            DEBUG_PRINT("Quit thread Xloop funciton\n");
        }


    }   /* VieW */

}   /* Gammou */
