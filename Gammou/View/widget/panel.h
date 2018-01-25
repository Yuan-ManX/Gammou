
#ifndef GAMMOU_PANNEL_H_
#define GAMMOU_PANNEL_H_

#include <deque>
#include <type_traits>
#include "widget.h"


namespace Gammou {

	namespace View {

		class abstract_panel : public widget {

		public:
			abstract_panel(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const color background);
			abstract_panel(const rectangle& rect, const color background);

			virtual ~abstract_panel() {}
			virtual void redraw_rect(rectangle& rect);

			void set_background_color(const color c);
			color get_background_color(void);

		protected:
			virtual widget *get_widget_at_position(const unsigned int x, const unsigned int y) const =0;
			virtual widget *get_focused_widget(void) const =0;
			virtual widget *get_draging_widget(void) const =0;
			virtual widget *get_draging_widget(mouse_button& button) const = 0;
		private:
			color m_background_color;
		};

		template<class widget_type = widget>
		class panel : public abstract_panel {

		public:
			static_assert(std::is_base_of<widget, widget_type>::value, "widget_type must inherit from widget");

			panel(const unsigned int x, const unsigned int y, const unsigned int width,
				const unsigned int height, const color background = cl_white);
			panel(const rectangle& rect, const color background = cl_white);

			virtual ~panel();

			virtual void redraw_rect(rectangle& rect) override;

		

			// widget override
			virtual bool on_key_up(const keycode key) override;
			virtual bool on_key_down(const keycode key) override;
			virtual bool on_mouse_enter(void) override;
			virtual bool on_mouse_exit(void) override;
			virtual bool on_mouse_move(const int x, const int y) override;
			virtual bool on_mouse_button_down(const mouse_button button, const int x, const int y) override;
			virtual bool on_mouse_button_up(const mouse_button button, const int x, const int y) override;
			virtual bool on_mouse_dbl_click(const int x, const int y) override;
			virtual bool on_mouse_wheel(const float distance) override;

			virtual bool on_mouse_drag_start(const mouse_button button, const int x, const int y) override;
			virtual bool on_mouse_drag(const mouse_button button, const int x, const int y,
				const int dx, const int dy) override;
			virtual bool on_mouse_drag_end(const mouse_button button, const int x, const int y) override;

		protected:
			virtual void draw(cairo_t *cr) override;

			virtual void draw_background(cairo_t *cr);
			virtual void draw_widgets(cairo_t *cr);
			
			void add_widget(widget_type *w);
			void remove_widget(widget_type *w);

			widget_type *get_widget_at_position(const unsigned int x, const unsigned int y) const override;
			widget_type *get_focused_widget(void) const override;
			widget_type *get_draging_widget(void) const override;
			widget_type *get_draging_widget(mouse_button& button) const override;
			
			std::deque<widget_type*> m_widgets;

		private:
			widget_type *m_focused_widget;	//	under cursor
			widget_type *m_draging_widget;
			mouse_button m_draging_button;
		};

		template<class widget_type>
		panel<widget_type>::panel(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const color background)
			: abstract_panel(x, y, width, height, background),
			m_focused_widget(nullptr),
			m_draging_widget(nullptr),
			m_draging_button(mouse_button::LeftButton)
		{

		}

		template<class widget_type>
		inline panel<widget_type>::panel(const rectangle & rect, const color background)
			: abstract_panel(rect, background)
		{
		}

		template<class widget_type>
		inline panel<widget_type>::~panel()
		{
			for (widget *w : m_widgets)
				delete w;
		}

		template<class widget_type>
		inline void panel<widget_type>::redraw_rect(rectangle & rect)
		{
			if (m_parent != nullptr)
				m_parent->redraw_rect(rect);
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_key_up(const keycode key)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_key_up(key);
			else
				return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_key_down(const keycode key)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_key_down(key);
			else
				return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_enter(void)
		{
			return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_exit(void)
		{
			if (m_focused_widget != nullptr) {
				m_focused_widget->on_mouse_exit();
				m_focused_widget = nullptr;
			}

			if (m_draging_widget != nullptr) {
				m_draging_widget->on_mouse_drag_end(m_draging_button, 0, 0);
				m_draging_widget = nullptr;
			}

			return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_move(const int x, const int y)
		{
			widget_type *w = get_widget_at_position(x, y);

			bool ret = false;

			if (m_focused_widget != w) {
				if (m_focused_widget != nullptr)
					ret = m_focused_widget->on_mouse_exit();
				if (w != nullptr)
					ret |= w->on_mouse_enter();
			}
			else if (m_focused_widget != nullptr) {
				ret = m_focused_widget->on_mouse_move(
					x - m_focused_widget->get_x(),
					y - m_focused_widget->get_y());
			}

			m_focused_widget = w;
			redraw();
			return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_button_down(const mouse_button button, const int x, const int y)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_mouse_button_down(button,
					x - m_focused_widget->get_x(),
					y - m_focused_widget->get_y());
			else
				return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_button_up(const mouse_button button, const int x, const int y)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_mouse_button_up(button,
					x - m_focused_widget->get_x(),
					y - m_focused_widget->get_y());
			else
				return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_dbl_click(const int x, const int y)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_mouse_dbl_click(
					x - m_focused_widget->get_x(),
					y - m_focused_widget->get_y());
			else
				return false;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_wheel(const float distance)
		{
			if (m_focused_widget != nullptr)
				return m_focused_widget->on_mouse_wheel(distance);
			else
				return false;
		}

		template<class widget_type>
		inline void panel<widget_type>::draw_background(cairo_t * cr)
		{
			cairo_helper::set_source_color(cr, get_background_color());
			cairo_rectangle(cr, 0, 0, get_width(), get_height());
			cairo_fill(cr);
		}

		template<class widget_type>
		inline void panel<widget_type>::draw_widgets(cairo_t * cr)
		{
			for (widget *w : m_widgets) {
				cairo_save(cr);
				cairo_translate(cr, w->get_x(), w->get_y());
				w->draw(cr);
				cairo_restore(cr);
			}
		}

		template<class widget_type>
		inline void panel<widget_type>::draw(cairo_t * cr)
		{
			draw_background(cr);
			draw_widgets(cr);
		}

		template<class widget_type>
		bool panel<widget_type>::on_mouse_drag_start(const mouse_button button, const int x, const int y)
		{
			m_draging_button = button;

			if (m_focused_widget != nullptr) {
				m_draging_widget = m_focused_widget;
				return m_draging_widget->on_mouse_drag_start(button,
					x - m_draging_widget->get_x(),
					y - m_draging_widget->get_y());
			}
			else {
				return false;
			}
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_drag(const mouse_button button, const int x, const int y, const int dx, const int dy)
		{
			widget_type *w = get_widget_at_position(x, y);
			bool ret = false;

			if (m_focused_widget != w) {
				if (m_focused_widget != nullptr)
					ret = m_focused_widget->on_mouse_exit();
				if (w != nullptr)
					ret |= w->on_mouse_enter();
				m_focused_widget = get_widget_at_position(x, y);
			}

			if (m_draging_widget != nullptr)
				ret |= m_draging_widget->on_mouse_drag(button,
					x - m_draging_widget->get_x(),
					y - m_draging_widget->get_y(),
					dx, dy);

			return ret;
		}

		template<class widget_type>
		inline bool panel<widget_type>::on_mouse_drag_end(const mouse_button button, const int x, const int y)
		{
			if (m_draging_widget != nullptr) {
				bool ret = m_draging_widget->on_mouse_drag_end(button,
					x - m_draging_widget->get_x(),
					y - m_draging_widget->get_y());
				m_draging_widget = nullptr;
				return ret;
			}
			else {
				return false;
			}
		}

		template<class widget_type>
		inline void panel<widget_type>::add_widget(widget_type * w)
		{
			if (w == nullptr)
				return;
			m_widgets.push_back(w);
			w->m_parent = this;
			redraw();
		}

		template<class widget_type>
		inline void panel<widget_type>::remove_widget(widget_type * w)
		{
			m_widgets.erase(std::remove(m_widgets.begin(), m_widgets.end(), w), m_widgets.end());

			if (m_focused_widget == w)
				m_focused_widget = nullptr;
			if (m_draging_widget == w)
				m_draging_widget = nullptr;

			redraw();
		}

		template<class widget_type>
		inline widget_type * panel<widget_type>::get_widget_at_position(const unsigned int x, const unsigned int y) const
		{
			for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
				widget_type *const w = (*it);
				if (w->contains(x - w->get_x(), y - w->get_y()))
					return w;
			}

			return nullptr;
		}

		template<class widget_type>
		inline widget_type * panel<widget_type>::get_focused_widget(void) const
		{
			return m_focused_widget;
		}

		template<class widget_type>
		inline widget_type * panel<widget_type>::get_draging_widget(void) const
		{
			return m_draging_widget;
		}

		template<class widget_type>
		inline widget_type * panel<widget_type>::get_draging_widget(mouse_button & button) const
		{
			button = m_draging_button;
			return m_draging_widget;
		}

} /* View */
} /* Gammou */

#endif
