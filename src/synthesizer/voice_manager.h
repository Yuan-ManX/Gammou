#ifndef GAMMOU_VOICE_MANAGER_H_
#define GAMMOU_VOICE_MANAGER_H_

#include <numeric>
#include <unordered_map>
#include <vector>

namespace Gammou
{
    class voice_manager {

    public:
        using note = uint8_t;
        using voice = uint32_t;
        using voice_store = std::vector<std::pair<note, voice>>;

        voice_manager(const std::size_t voice_count)
        :   _voices{}
        {
            _voices.reserve(voice_count);
            for (auto i = 0u; i < voice_count; ++i)
                _voices.emplace_back(0u, i);

            _on_voice_end = _voices.begin();
            _active_voices_end = _voices.begin();
        }

        /**
         *  @brief  Allocate a new voice
         *  @param n the note for which the voice is allocated
         **/
        bool note_on(note n, voice& v)
        {
            voice_store::iterator it;
            if (allocate_voice(n, it)) {
                v = it->second;
                return true;
            }
            else {
                return false;
            }
        }

        /**
         *  @brief Switch a voice off
         *  @param n the note assigned to the voice
         *
         **/
        bool note_off(note n, voice& v)
        {
            auto it = find_on_voice(n);
            if (it != _on_voice_end) {
                v = it->second;
                voice_off(it);
                return true;
            }
            else {
                return false;
            }
        }

        /**
         *  @brief Call the given function on each active voice, and stop them if needed
         *  @param func Function called on each active voice. return false is channel must be stopped
         **/
        template <typename TFunction>
        void foreach_active_voice(TFunction func)
        {
            for (auto it = _voices.begin(); it != _active_voices_end;) {
                const auto voice = it->second;
                //  If the note should be stopped
                if (!func(voice)) {
                    //  If note is still on
                    if (it < _on_voice_end)
                        voice_off(it);
                    voice_stop(it);
                    continue;
                }
                ++it;
            }
        }

    protected:
        bool allocate_voice(note n, voice_store::iterator& it)
        {
            //  if there is a free voice
            if (_active_voices_end != _voices.end()) {
                it = _active_voices_end++;
                it->first = n;
                return true;
            }
            else {
                return false;
            }
        }

        void voice_off(voice_store::iterator it)
        {
            std::iter_swap(it, --_on_voice_end);
        }

        void voice_stop(voice_store::iterator it)
        {
            std::iter_swap(it, --_active_voices_end);
        }

        voice_store::iterator find_on_voice(note n)
        {
            for (auto it = _voices.begin(); it != _on_voice_end; ++it) {
                if (it->first == n)
                    return it;
            }
            return _on_voice_end;
        }

    private:
        voice_store _voices{};
        voice_store::iterator _on_voice_end;
        voice_store::iterator _active_voices_end;
    };

} // namespace Gammou


#endif