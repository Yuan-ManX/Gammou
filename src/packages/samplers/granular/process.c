
#include <math.h>
#include <synthesizer_def.h>

#define GRAIN_COUNT 16u

struct grain
{
    float grain_cursor; //  in sample
    float grain_center; //  in sample
};

typedef struct
{
   struct grain grains[GRAIN_COUNT];
    float time;
   float first_grain_time;
   unsigned int rnd;

} granular_state;

static unsigned int rnd_step(unsigned int last)
{
    return 1664525u * last + 1013904223u;
}

static float rnd_val(unsigned int rnd)
{
    return ((float)(rnd % 10000) - 5000.f) / 10000.f;
}

static float rnd_float(granular_state *state, float center, float width)
{
    const float val = center + rnd_val(state->rnd) * width;
    state->rnd = rnd_step(state->rnd);
    return val;
}

void node_initialize(const wav_channel *sample, granular_state *state)
{
    state->rnd = (unsigned int)state;
    state->time = 0.f;
    state->first_grain_time = -0.5f;

    const float radius = 0.3f;
    for (unsigned int i = 0u; i < GRAIN_COUNT; i++) {
        state->grains[i].grain_center = -1000.f;//rnd_float(state, 5.f, 1.f);
        state->grains[i].grain_cursor = -1.f;// state->grains[i].grain_center + (state->time - state->first_grain_time);
    }
}

static float grain_env(float t, float dev)
{
    return expf(-t * t / (2.f * dev * dev));
}

static float get_sample(const wav_channel *sample, int idx)
{
    if (idx < 0 || idx >= sample->sample_count)
        return 0.f;
    else
        return sample->samples[idx];
}

static float sample_value(const wav_channel *sample, float pos)
{
    const int idx = (int)(pos * _sample_rate);
    const float factor = pos - (float)idx;
    return factor * get_sample(sample, idx) + (1.f - factor) * get_sample(sample, idx);
}

void node_process(const wav_channel *sample, granular_state *state, float pos, float width, float radius, float radius_width, float dev, float speed, float *out)
{
    const float dt = 1.f / _sample_rate;
    const float max_env_cursor = dev * 4.5f;
    float output = 0.f;

    for (unsigned int i = 0u; i < GRAIN_COUNT; i++)
    {
        const float env_cursor = state->grains[i].grain_cursor - state->grains[i].grain_center;

        if (env_cursor > max_env_cursor)
        {
            state->first_grain_time += rnd_float(state, radius, radius_width);
            state->grains[i].grain_center = rnd_float(state, pos, width);
            state->grains[i].grain_cursor = state->grains[i].grain_center + (state->time - state->first_grain_time) * speed;
        }

        const float updated_env_cursor = state->grains[i].grain_cursor - state->grains[i].grain_center;
        output += sample_value(sample, state->grains[i].grain_cursor) * grain_env(env_cursor, dev);
        state->grains[i].grain_cursor += dt * speed;
    }

    state->time += dt;
    *out = output;
}
