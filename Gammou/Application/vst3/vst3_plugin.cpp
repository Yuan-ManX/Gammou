

#include <iostream>
#include <cmath>

#include "vst3_plugin.h"

#include <Windows.h> // a supprimer

namespace Gammou {

	namespace Vst {

		//	Plugin class implementation

		Plugin::Plugin()
			:
			m_synthesizer_mutex(),
			m_synthesizer(2, 2, GAMMOU_SYNTHESIZER_CHANNEL_COUNT, GAMMOU_VST_AUTOMATION_INPUT_COUNT),
			m_window(&m_synthesizer, &m_synthesizer_mutex, 800, 800)
		{
			DEBUG_PRINT("Gammou CTOR\n");
		}

		Plugin::~Plugin()
		{
			DEBUG_PRINT("Gammou DTOR\n");
		}

		Steinberg::tresult PLUGIN_API Plugin::initialize(FUnknown * context)
		{
			Steinberg::tresult result = SingleComponentEffect::initialize(context);

			if (result != Steinberg::kResultOk)
				return result;

			// Create Audio Input
			addAudioInput(USTRING("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);

			// Create Audio Output
			addAudioOutput(USTRING("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

			//	Create Midi Input
			addEventInput(USTRING("Midi In"), 1); // 1 channel

			// Create Parameter inputs
			const unsigned int parameter_count = m_synthesizer.get_parameter_input_count();

			for (unsigned int i = 0; i < parameter_count; ++i) {
				std::string param_name = ("Parameter " + std::to_string(i));
				parameters.addParameter(new Steinberg::Vst::Parameter(USTRING(param_name.c_str()), i));
			}

			DEBUG_PRINT("Gammou Initialize\n");

			return Steinberg::kResultOk;
		}

		Steinberg::tresult PLUGIN_API Plugin::terminate()
		{
			return SingleComponentEffect::terminate();
		}

		Steinberg::tresult PLUGIN_API Plugin::setActive(Steinberg::TBool state)
		{
			return Steinberg::kResultOk;
		}

		Steinberg::tresult PLUGIN_API Plugin::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
		{
			return (symbolicSampleSize == Steinberg::Vst::kSample64 ||
				symbolicSampleSize == Steinberg::Vst::kSample32) ?
				Steinberg::kResultTrue : Steinberg::kResultFalse;
		}

		Steinberg::tresult PLUGIN_API Plugin::setupProcessing(Steinberg::Vst::ProcessSetup & newSetup)
		{
			DEBUG_PRINT("Gammou Setup processing : sample size = %u bits, sample rate = %lf\n",
				newSetup.symbolicSampleSize == Steinberg::Vst::kSample32 ? 32 : 64,
				newSetup.sampleRate);

			lock_synthesizer();
			m_synthesizer.set_sample_rate(newSetup.sampleRate);
			unlock_synthesizer();

			return SingleComponentEffect::setupProcessing(newSetup);
		}

		Steinberg::tresult PLUGIN_API Plugin::process(Steinberg::Vst::ProcessData & data)
		{
			lock_synthesizer();

			// Input parameter change (Parameter input)
			Steinberg::Vst::IParameterChanges *const param_changes = data.inputParameterChanges;

			if (param_changes != nullptr) {
				const unsigned int changed_param_count = param_changes->getParameterCount();
				
				for (unsigned int i = 0; i < changed_param_count; ++i) {
					Steinberg::Vst::IParamValueQueue *const param_data = param_changes->getParameterData(i);

					if (param_data != nullptr) {
						const unsigned int data_count = param_data->getPointCount();
						const unsigned int param_id = param_data->getParameterId();
						Steinberg::int32 sample_offset;
						double value;
						// For le moment, only the last point

						DEBUG_PRINT("Parameter data : id = %u, %u values\n", param_id, data_count);

						if (Steinberg::kResultTrue == 
							param_data->getPoint(data_count - 1, sample_offset, value))
							m_synthesizer.set_parameter_value(value, param_id);
					}
				}
			}

			// Process Midi Input Events
			Steinberg::Vst::IEventList *event_list = data.inputEvents;

			if (event_list != nullptr) {
				const unsigned int event_count = event_list->getEventCount();

				for (unsigned int i = 0; i < event_count; ++i) {
					Steinberg::Vst::Event event;

					if (Steinberg::kResultOk == event_list->getEvent(i, event)) {
						
						//	TODO
						//	event.sampleOffset

						switch (event.type) {

						case Steinberg::Vst::Event::kNoteOnEvent:
							m_synthesizer.send_note_on(event.noteOn.pitch, event.noteOn.velocity);
							break;

						case Steinberg::Vst::Event::kNoteOffEvent:
							m_synthesizer.send_note_off(event.noteOff.pitch, event.noteOff.velocity);
							break;

						}
					}
				}
			}

			//	Process Audio
			
			//if (data.numOutputs == 0) // 
			//	return Steinberg::kResultOk;


			//	No Silent flag (Todo check input silent flag)
			data.outputs[0].silenceFlags = 0;

			const unsigned int nbSamples = data.numSamples;

			double input[2];
			double output[2];

			if (processSetup.symbolicSampleSize == Steinberg::Vst::kSample32) { // 32 bits
				float *output_buffer_left = data.outputs[0].channelBuffers32[0];
				float *output_buffer_right = data.outputs[0].channelBuffers32[1];

				float *input_buffer_left = data.inputs[0].channelBuffers32[0];
				float *input_buffer_right = data.inputs[0].channelBuffers32[1];

				for (unsigned int i = 0; i < nbSamples; 
					++i, ++output_buffer_left, ++output_buffer_right, ++input_buffer_left, ++input_buffer_right) {

					input[0] = 2000.0;//static_cast<double>(*input_buffer_left);
					input[1] = static_cast<double>(*input_buffer_right);

					m_synthesizer.process(input, output);

					*output_buffer_left = static_cast<float>(output[0]);
					*output_buffer_right = static_cast<float>(output[1]);
				}
			}
			else { // 64 bit
				double *output_buffer_left = data.outputs[0].channelBuffers64[0];
				double *output_buffer_right = data.outputs[0].channelBuffers64[1];

				double *input_buffer_left = data.inputs[0].channelBuffers64[0];
				double *input_buffer_right = data.inputs[0].channelBuffers64[1];

				for (unsigned int i = 0; i < nbSamples; 
					++i, ++output_buffer_left, ++output_buffer_right, ++input_buffer_left, ++input_buffer_right) {

					input[0] = *input_buffer_left;
					input[1] = *input_buffer_right;

					m_synthesizer.process(input, output);

					*output_buffer_left = output[0];
					*output_buffer_right = output[1];
				}
			}

			unlock_synthesizer();

			return Steinberg::kResultOk;
		}

		Steinberg::tresult PLUGIN_API Plugin::setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
			Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
		{
			// Host ask to change
			// TODO check coherence with synthesizer
			if (numIns != 1 ||
				numOuts != 1 ||
				outputs[0] != Steinberg::Vst::SpeakerArr::kStereo ||
				inputs[0] != Steinberg::Vst::SpeakerArr::kStereo)
				return Steinberg::kResultFalse;
			else
				return Steinberg::kResultOk;
		}


		Steinberg::IPlugView *Plugin::createView(const char* name)
		{
			return m_window.create_vst3_view_instance();
		}



	} /*  Vst */


} /* Gammou */

