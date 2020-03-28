#ifndef GAMMOU_DESKTOP_APPLICATION_H_
#define GAMMOU_DESKTOP_APPLICATION_H_

#include <RtAudio.h>
#include <RtMidi.h>
#include "synthesizer/synthesizer.h"
#include "view.h"

namespace Gammou {

    class midi_device_widget;

    class desktop_application /* : */{
        friend class midi_device_widget;
    public:
        desktop_application(unsigned int input_count, unsigned int output_count);
        ~desktop_application();

        void run();

    private:
        void _initialize_midi_multiplex();
        void _enable_midi_input(
            unsigned int idx, bool enable = true);
        unsigned int _midi_input_count() const noexcept;

        void _start_audio(
            RtAudio::Api api,
            unsigned int device_index,
            unsigned int sample_rate);
        void _stop_audio();

        // toolbox construction
        std::unique_ptr<View::widget> _make_audio_device_widget();
        std::unique_ptr<View::widget> _make_midi_device_widget();

        /*
         *  Members
         */

        //  Processing
        llvm::LLVMContext _llvm_context;
        synthesizer _synthesizer;

        //  GUI
        std::unique_ptr<View::widget> _window{};
        std::unique_ptr<View::native_application_display> _display{};

        using audio_device_descriptor =
            std::tuple<
                RtAudio::Api, // api
                unsigned int, // device_index
                unsigned int>; // sample_rate

        View::storage_directory_model<std::string, audio_device_descriptor> _audio_device_tree{};

        //  Audio I/O
        std::unique_ptr<RtAudio> _audio_device{};

        //  Midi inputs
        std::vector<RtMidiIn> _midi_inputs;
    };

}

#endif