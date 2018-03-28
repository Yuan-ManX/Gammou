#ifndef ABSTRACT_VST3_WINDOW_H_
#define ABSTRACT_VST3_WINDOW_H_

#include "public.sdk/source/common/pluginview.h"
#include "../abstract_window.h"

namespace Gammou {

	namespace View {

		class abstract_vst3_view;

		class abstract_vst3_window :  public abstract_window
		{
			friend class abstract_vst3_view;

		public:
			abstract_vst3_window(const unsigned int width, const unsigned int height);
			virtual ~abstract_vst3_window();

			//	To be given to vst3 host
			virtual abstract_vst3_view *create_vst3_view_instance() =0;

			//	
			virtual void open(void *parent_window) =0;
			virtual void close(void) =0;

			//	widget override   (visibilit� ?)
			//	Abstract coordinate (not windows system)
			virtual void resize(const unsigned int width, const unsigned int height) override;

		private:
			abstract_vst3_view *m_current_view;
		};

		class abstract_vst3_view : public Steinberg::CPluginView {

		public:
			abstract_vst3_view(abstract_vst3_window *window);
			virtual ~abstract_vst3_view();

			void resize(const unsigned int width, const unsigned int height);

			// CPluginView override
			Steinberg::tresult PLUGIN_API onWheel(float distance) override;
			Steinberg::tresult PLUGIN_API onKeyDown(Steinberg::char16 key, Steinberg::int16 keyMsg, Steinberg::int16 modifiers) override;
			Steinberg::tresult PLUGIN_API onKeyUp(Steinberg::char16 key, Steinberg::int16 keyMsg, Steinberg::int16 modifiers) override;

			void attachedToParent() SMTG_OVERRIDE;
			void removedFromParent() SMTG_OVERRIDE;
		protected:
			abstract_vst3_window *m_window;
		};

	} /* View */

} /* Gammou */



#endif
