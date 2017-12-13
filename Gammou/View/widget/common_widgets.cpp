#include "../../debug.h"
#include "common_widgets.h"

namespace Gammou {

	namespace View {

		// Control implementation

		control::control(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height)
			: widget(x, y, width, height), m_focused(false)
		{

		}

		control::control(const rectangle & rect)
			: widget(rect)
		{
		}


		void control::draw(cairo_t * cr)
		{
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_set_line_width(cr, 1);
			cairo_rectangle(cr, 0, 0, get_width(), get_height());
			cairo_stroke(cr);
		}

		bool control::on_mouse_enter(void)
		{
			m_focused = true;
			return true;
		}

		bool control::on_mouse_exit(void)
		{
			m_focused = false;
			return true;
		}

		bool control::is_focused(void) const
		{
			return m_focused;
		}

		bool control::is_enabled(void) const
		{
			return m_enabled;
		}

		void control::set_enabled(const bool state)
		{
			m_enabled = state;
		}

		// movable control implementation

		movable_control::movable_control(const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height)
			: control(x, y, width, height), m_movable(true)
		{
		}

		movable_control::movable_control(const rectangle & rect)
			: control(rect)
		{
		}

		bool movable_control::on_mouse_drag(const mouse_button button, const int x, const int y, const int dx, const int dy)
		{
			if (m_movable) {
				const rectangle rect = get_rect()->translate(dx, dy);
				if (get_parent()->contains(rect)) {
					set_rect(rect);
					redraw_parent();
				}
				return true;
			}
			else {
				return false;
			}	
		}

		bool movable_control::is_movable() const
		{
			return m_movable;
		}

		void movable_control::set_movable(bool movable)
		{
			m_movable = movable;
		}


		// push_button implementation

		push_button::push_button(std::function<void()> push_action, const std::string& text, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const unsigned int font_size)
			: m_pushed(false),  m_push_action(push_action), 
			m_text(text), m_font_size(font_size), control(x, y, width, height)
		{
		}

		void push_button::draw(cairo_t * cr)
		{
			cairo_text_extents_t te;
			float offset;

			if (m_pushed) {
				cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
				offset = 3;
			}
			else {
				cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
				offset = 2;
			}

			if (is_focused())
				cairo_set_source_rgb(cr, 0.83, 0.86, 0.9);
			else
				cairo_set_source_rgb(cr, 1, 1, 1.0);
			
			cairo_helper::rounded_rectangle(cr, offset / 2, offset / 2, get_width() - offset, get_height() - offset, offset);
			cairo_fill(cr);
			cairo_set_source_rgb(cr, 0, 0, 0);

			cairo_set_line_width(cr, offset);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
			cairo_helper::rounded_rectangle(cr, offset / 2, offset / 2, get_width() - offset, get_height() - offset, offset);
			cairo_stroke(cr);

			cairo_set_font_size(cr, m_font_size);
			cairo_text_extents(cr, m_text.c_str(), &te);
			cairo_move_to(cr, (get_width() - te.width) / 2,
				(get_height() + te.height) / 2);
			cairo_show_text(cr, m_text.c_str());
		}

		bool push_button::on_mouse_drag_end(const mouse_button button, const int x, const int y)
		{
			if (button == mouse_button::LeftButton) {
				if ( is_focused() )
					m_push_action();

				m_pushed = false;
				redraw();
				return true;
			}
			else {
				return false;
			}
		}

		bool push_button::on_mouse_button_down(mouse_button button, const int cx, const int cy)
		{
			if (button == mouse_button::LeftButton) {
				m_pushed = true;
				redraw();
				return true;
			}
			else {
				return false;
			}
		}

		bool push_button::on_mouse_button_up(mouse_button button, const int cx, const int cy)
		{
			if (button == mouse_button::LeftButton && m_pushed) {
				m_pushed = false;
				m_push_action();
				redraw();
				return true;
			}
			else {
				return false;
			}
		}

		bool push_button::on_mouse_enter(void)
		{
			control::on_mouse_enter();
			redraw();
			return true;
		}

		bool push_button::on_mouse_exit(void)
		{
			control::on_mouse_exit();
			redraw();
			return true;
		}

		void push_button::set_text(std::string & text)
		{
			m_text = text;
			redraw();
		}

		bool push_button::is_pushed(void) const
		{
			return m_pushed;
		}



} /* View */

} /* Gammou */


