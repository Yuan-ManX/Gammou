/*
 *		Plugin.h
 */

#pragma once
#include "../../debug.h"

#include "public.sdk/source/vst/vstsinglecomponenteffect.h" // first !!!!

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/ustring.h"

#include "../gui/synthesizer_gui.h"


namespace Gammou {

	namespace Vst {
		class Plugin : public Steinberg::Vst::SingleComponentEffect {

		public:
			Plugin();
			~Plugin();

			//	AudioEffect override
			Steinberg::tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
			Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
			Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

			Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
			Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
			Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

			Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
				Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;

			//	Editor override
			Steinberg::IPlugView *PLUGIN_API createView(const char* name) SMTG_OVERRIDE;

		private:
			synthesizer_gui m_window;

			double m_freq;
			double m_t;
		};
	} /* namespace Vst */

} /* namespace Gammou */