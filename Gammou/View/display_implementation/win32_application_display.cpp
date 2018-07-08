
#include "win32_application_display.h"

namespace Gammou {

	namespace View {

		win32_application_display::win32_application_display(
			View::widget & root_widget)
		:	abstract_display(root_widget),
			abstract_win32_display(root_widget),
			abstract_application_display(root_widget),
			m_running(false)
		{
		}

		win32_application_display::~win32_application_display()
		{
		}

		void win32_application_display::open(
			const std::string & title)
		{
			m_running = true;
			//m_window_manager = 
			//	std::thread(window_manager, this);

			window_manager(this);
		}

		void win32_application_display::close()
		{
			abstract_win32_display::close();
			//if (m_window_manager.joinable())
			//	m_window_manager.join();
		}

		void win32_application_display::window_manager(
			win32_application_display *self)
		{
			DEBUG_PRINT("Entering Window  MAnager Thread\n");
			//	Create Window
			// TODO handle parent and title
			DEBUG_PRINT("Create Window ..\n");
			self->create_window(nullptr, "");
			DEBUG_PRINT("Ok\n");	

			SetFocus(self->get_window_handle());
			//	Event loop
		
			MSG msg;

			DEBUG_PRINT("Entering Win Event Loop\n");
			
			while (true) {
				DEBUG_PRINT("Getting message.....\n");

				if (GetMessage(&msg, self->get_window_handle(), 0, 0) > 0) {
					DEBUG_PRINT("Got  message\n");
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else {
					DEBUG_PRINT("GEt message error\n");
				}
			}
			
			// TODO destroy win
		}

	}	/* View */

} /* Gammou */