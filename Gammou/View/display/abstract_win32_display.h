#ifndef ABSTRACT_WIN32_DISPLAY_H_
#define ABSTRACT_WIN32_DISPLAY_H_

#include <Windows.h>
#include <windowsx.h>
#undef min
#undef max

#include "cairo_definition.h"
#include "abstract_display.h"

namespace Gammou {

	namespace View {

		class abstract_win32_display : 
			virtual public abstract_display {

		public:
			abstract_win32_display(View::widget& root_widget);
			virtual  ~abstract_win32_display();

			void close() override;

		protected:
			void create_window(HWND parent_window, const std::string& title);
			void sys_redraw_rect(const rectangle& rect);

		private:
			static LRESULT CALLBACK windowProc(
				HWND window,
				UINT msg,
				WPARAM w_param,
				LPARAM l_param);

			HWND m_window_handle;
			bool m_has_focus;
		};

	} /* View */

} /* Gammou */

#endif