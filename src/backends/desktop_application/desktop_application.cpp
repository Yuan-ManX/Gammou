
#include "desktop_application.h"
#include "gui/main_gui.h"
#include "synthesizer/midi_parser.h"

namespace Gammou {

    desktop_application::desktop_application(unsigned int input_count, unsigned int output_count)
    : _synthesizer{_llvm_context, input_count, output_count}
    {
        //  audio I/O
        //_start_audio(RtAudio::UNIX_JACK, 0u, 48000);

        // midi multiplex
        _initialize_midi_multiplex();

        // gui
        auto additional_toolbox = std::make_unique<View::header>(_make_midi_device_widget());
        _window = make_synthesizer_gui(_synthesizer, std::move(additional_toolbox));

        //  display
        _display = std::make_unique<View::native_application_display>(*_window, 12);
    }

    desktop_application::~desktop_application()
    {
        _stop_audio();
    }

    void desktop_application::run()
    {
        _display->open("Gammou");
        _display->wait();
    }

    void desktop_application::_initialize_midi_multiplex()
    {
        RtMidiIn rt_midi;
        const auto midi_input_count = rt_midi.getPortCount();

        //  We need one RtMidi instance per input port
        _midi_inputs.resize(midi_input_count);

        auto midi_callback =
            [](double timestamp, std::vector<unsigned char> *message, void *user_data)
            {
                auto& synth = *(synthesizer*)(user_data);
                execute_midi_msg(synth, message->data(), message->size());
            };

        for (auto& input : _midi_inputs)
            input.setCallback(midi_callback, &_synthesizer);
    }

    void desktop_application::_enable_midi_input(unsigned int idx, bool enable)
    {
        if (idx >= _midi_input_count())
            return;

        if (enable)
            _midi_inputs[idx].openPort(idx);
        else
            _midi_inputs[idx].closePort();
    }

    unsigned int desktop_application::_midi_input_count() const noexcept
    {
        return _midi_inputs.size();
    }

    void desktop_application::_start_audio(
            RtAudio::Api api,
            unsigned int device_index,
            unsigned int sample_rate)
    {
        _stop_audio();

        RtAudio::StreamParameters output_params;
        unsigned int buffer_size = 512;

        output_params.deviceId = device_index;
        output_params.firstChannel = 0u;
        output_params.nChannels = _synthesizer.get_output_count();

        auto audio_callback =
            [](void *output_buffer, void *input_buffer, unsigned int sample_count,
                double stream_time, RtAudioStreamStatus status, void *user_data)
            {
                auto& synth = *(synthesizer*)(user_data);
                synth.process_buffer(sample_count, nullptr, static_cast<float*>(output_buffer));
                return 0;
            };

        try {
            _audio_device = std::make_unique<RtAudio>(api);
            _audio_device->openStream(
                &output_params, nullptr, RTAUDIO_FLOAT32, sample_rate, &buffer_size, audio_callback, &_synthesizer);
            _audio_device->startStream();
        }
        catch(...)
        {
            _stop_audio();
        }
    }

    void desktop_application::_stop_audio()
    {
        if (_audio_device) {
            try {
                _audio_device->stopStream();
                _audio_device.reset();
            }
            catch(...)
            {}
        }
    }

    std::unique_ptr<View::widget> desktop_application::_make_midi_device_widget()
    {
        auto midi_settings_widget = std::make_unique<View::panel<>>(0.f, 0.f /* dummy sizes*/);

        const auto midi_input_count = _midi_input_count();
        constexpr auto y_start_offset = 0.2f;
        constexpr auto line_height = 1.2f;

        for (auto i = 0u; i < midi_input_count; ++i) {
            const auto y_offset = y_start_offset + i * line_height;
            auto device_checkbox = std::make_unique<View::checkbox>();
            auto device_label = std::make_unique<View::label>(_midi_inputs[i].getPortName(i));

            device_checkbox->set_callback(
                [this, idx = i](bool checked)
                {
                    _enable_midi_input(idx, checked);
                });

            midi_settings_widget->insert_widget(1.0, y_offset + 0.1, std::move(device_checkbox));
            midi_settings_widget->insert_widget(2.2f, y_offset, std::move(device_label));
        }

        midi_settings_widget->resize(30, line_height * midi_input_count + y_start_offset * 2);

        return midi_settings_widget;
    }

}