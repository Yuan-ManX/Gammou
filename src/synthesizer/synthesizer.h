#ifndef GAMMOU_SYNTHESIZER_H_
#define GAMMOU_SYNTHESIZER_H_

#include <compile_node_class.h>
#include "voice_manager.h"

namespace Gammou
{
    class synthesizer {
    public:
        enum midi_inputs {
            gate = 0u,
            pitch,

            midi_input_count
        };

        static constexpr auto polyphonic_to_master_channel_count = 2u;
        static constexpr auto voice_disappearance_treshold = 0.0003f;

        synthesizer(
            llvm::LLVMContext &llvm_context,
            unsigned int input_count = 0u,
            unsigned int output_count = 1u,
            unsigned int voice_count = 16);

        synthesizer(const synthesizer&) = delete;
        synthesizer(synthesizer&&) = delete;
        ~synthesizer() = default;

        //  Master circuit internal nodes
        auto& from_polyphonic_node() noexcept { return _from_polyphonic; }
        auto& output_node() noexcept { return _output; }

        //  Polyphonic circuit internal nodes
        auto& midi_input_node() noexcept { return _midi_input; }
        auto& to_master_node() noexcept { return _to_master; }

        /**
         * @brief Compile the master circuit and update the processing code
         **/
        void compile_master_circuit();

        /**
         * @brief Compile the polyphonic circuit and update the processing code
         **/
        void compile_polyphonic_circuit();

        /**
         *  @brief
         **/
        // void bypass_master(bool);

        /**
         **
         **    Process thread part
         **
         **/

        /**
         *  @brief
         *  @param input input values [channel0, channel1, ...]
         *  @param output output values [channel0, channel1, ...]
         **/
        void process(
            const float input[],
            float output[]);

        /*
         *
         */
        void midi_note_on(uint8_t node, float velocity);
        void midi_note_off(uint8_t node, float velocity);
        void midi_control_change(uint8_t control, float value);

    private:
        auto get_voice_midi_input(voice_manager::voice voice) noexcept
        {
            return _midi_input_values.data() + voice * midi_input_count;
        }

        llvm::LLVMContext& _llvm_context;

        //  Midi input values are stored here for every running voices
        std::vector<float> _midi_input_values;

        //  Circuit execution context
        DSPJIT::graph_execution_context _master_circuit_context;
        DSPJIT::graph_execution_context _polyphonic_circuit_context;

        //  Master circuit internal nodes
        DSPJIT::compile_node_class _from_polyphonic;
        DSPJIT::compile_node_class _output;

        //  Polyphonic circuit internal nodes
        DSPJIT::compile_node_class _midi_input;
        DSPJIT::compile_node_class _to_master;

        //  Voice management
        voice_manager _voice_manager;
        unsigned int _voice_disappearance_sample_count{400u};
        std::vector<unsigned int> _voice_lifetime;
    };

} // namespace Gammou


#endif