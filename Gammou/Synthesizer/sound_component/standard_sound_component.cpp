
#include "standard_sound_component.h"

#include <cmath>



namespace gammou {


/*
 *		sin_oscilator
 */

sin_oscilator::sin_oscilator(const unsigned int channel_count)
 : sound_component("Sin", 2, 1, channel_count),
   m_freq_integral(this)
{

}

void sin_oscilator::process(const double input[])
{
	m_output[0] = std::sin(m_freq_integral + input[1]);
	m_freq_integral = m_freq_integral + (get_sample_duration() * input[0]);
}

void sin_oscilator::initialize_process()
{
	m_freq_integral = 0.0;
}




}