#ifndef USER_GUI_COMPONENT_H_
#define USER_GUI_COMPONENT_H_

#include "gui_sound_component.h"
#include "abstract_gui_component_factory.h"

#include "user_sound_component.h"

namespace Gammou {

    namespace Gui {

        class user_gui_component : public gui_sound_component {

            public:
                user_gui_component(
                    user_sound_component *component,
                    const int x, const int y);
                ~user_gui_component() {}

            private:
                user_sound_component *m_component;
        };

        class user_gui_component_factory : public abstract_gui_component_factory {

            public:
                user_gui_component_factory(gui_component_main_factory& factory);
                ~user_gui_component_factory() {}

                std::unique_ptr<gui_sound_component> create_complete_component(
                    const int x, const int y, Sound::data_input_stream& source, const unsigned int channel_count) override;
                std::unique_ptr<gui_sound_component> create_complete_component(
                    const int x, const int y, const Sound::answer_form& answer_form, const unsigned int channel_count) override;

            private:
                gui_component_main_factory& m_factory;
        };


    }   /*  Gui */

} /* Gammou */

#endif  /*  USER_GUI_COMPONENT */
