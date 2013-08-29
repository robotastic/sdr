/*
 * Copyright 2011 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


/*
 * GNU Radio C++ example creating dial tone
 * ("the simplest thing that could possibly work")
 *
 * Send a tone each to the left and right channels of stereo audio
 * output and let the user's brain sum them.
 *
 * GNU Radio makes extensive use of Boost shared pointers.  Signal processing
 * blocks are typically created by calling a "make" factory function, which
 * returns an instance of the block as a typedef'd shared pointer that can
 * be used in any way a regular pointer can.  Shared pointers created this way
 * keep track of their memory and free it at the right time, so the user
 * doesn't need to worry about it (really).
 *
 */

// Include header files for each block used in flowgraph

#include <iostream>
#include <string> 
#include <stdio.h>
#include <stdlib.h>

#include "logging_receiver_dsd.h"
#include <smartnet_crc.h>
#include <smartnet_deinterleave.h>

#include <osmosdr_source_c.h>
#include <osmosdr_sink_c.h>

#include <boost/program_options.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <filter/freq_xlating_fir_filter_ccf.h>
#include <filter/firdes.h>

#include <digital_fll_band_edge_cc.h>
#include <digital_clock_recovery_mm_ff.h>
#include <digital_binary_slicer_fb.h>

#include <gr_firdes.h>
#include <gr_fir_filter_ccf.h>

#include <gr_pll_freqdet_cf.h>
#include <gr_sig_source_f.h>
#include <gr_sig_source_c.h>
#include <gr_audio_sink.h>
#include <gr_correlate_access_code_tag_bb.h>
#include <gr_msg_queue.h>
#include <gr_message.h>
#include <gr_file_sink.h>
#include <gr_complex.h>
#include <gr_fir_filter_ccf.h>
#include <gr_top_block.h>
#include <gr_multiply_cc.h>
#include <gr_io_signature.h>
#include <gr_hier_block2.h>
#include <gr_firdes.h>
#include <gr_fir_filter_ccf.h>
#include <gr_fir_filter_fff.h>
#include <gr_freq_xlating_fir_filter_ccf.h>
#include <filter/firdes.h>
#include <filter/rational_resampler_base_ccc.h>
#include <gr_quadrature_demod_cf.h>
#include <analog/quadrature_demod_cf.h>
#include <dsd_block_ff.h>
#include <gr_sig_source_f.h>
#include <gr_sig_source_c.h>
#include <gr_multiply_cc.h>
#include <gr_file_sink.h>
#include <gr_rational_resampler_base_ccf.h>
#include <gr_rational_resampler_base_fff.h>






namespace po = boost::program_options;

using namespace std;


unsigned GCD(unsigned u, unsigned v) {
    while ( v != 0) {
        unsigned r = u % v;
        u = v;
        v = r;
    }
    return u;
}

std::vector<float> design_filter(double interpolation, double deci) {


    




    float beta = 5.0;
    float trans_width = 0.5 - 0.4;
    float mid_transition_band = 0.5 - trans_width/2;

 std::vector<float> result = gr_firdes::low_pass(
		              interpolation,
				1,	                     
	                      mid_transition_band/interpolation, 
                              trans_width/interpolation,         
                              gr_firdes::WIN_KAISER,
                              beta                               
                              );

	return result;
}

int main(int argc, char **argv)
{
std::string device_addr;
    double center_freq, samp_rate, chan_freq, error;
	int if_gain, bb_gain, rf_gain;
    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("arg", po::value<std::string>(&device_addr)->default_value(""), "the device arguments in string format")
        ("rate", po::value<double>(&samp_rate)->default_value(1e6), "the sample rate in samples per second")
        ("center", po::value<double>(&center_freq)->default_value(10e6), "the center frequency in Hz")
	("error", po::value<double>(&error)->default_value(0), "the Error in frequency in Hz")
	("freq", po::value<double>(&chan_freq)->default_value(10e6), "the frequency in Hz of the trunking channel")
        ("rfgain", po::value<int>(&rf_gain)->default_value(14), "RF Gain")
	("bbgain", po::value<int>(&bb_gain)->default_value(25), "BB Gain")
	("ifgain", po::value<int>(&if_gain)->default_value(25), "IF Gain")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout
            << boost::format("SmartNet Trunking Reciever %s") % desc << std::endl
            << "The tags sink demo block will print USRP source time stamps." << std::endl
            << "The tags source demo block will send bursts to the USRP sink." << std::endl
            << "Look at the USRP output on a scope to see the timed bursts." << std::endl
            << std::endl;
        return ~0;
    }




 
  gr_top_block_sptr tb = gr_make_top_block("dsd_rx");

	
	osmosdr_source_c_sptr src = osmosdr_make_source_c();
	cout << "Setting sample rate to: " << samp_rate << endl;
	src->set_sample_rate(samp_rate);
	cout << "Tunning to " << center_freq - error << "hz" << endl;
	src->set_center_freq(center_freq - error,0);

	cout << "Setting RF gain to " << rf_gain << endl;
	cout << "Setting BB gain to " << bb_gain << endl;
	cout << "Setting IF gain to " << if_gain << endl;

	src->set_gain(rf_gain);
	src->set_if_gain(if_gain);
	src->set_bb_gain(bb_gain);



	
	int samp_per_sym = 10;
	int decim = 20;
	float xlate_bandwidth = 12500; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = double(samp_rate/decim);
	float offset = center_freq - chan_freq;
	double vocoder_rate = 8000;
	double audio_rate = 44100;
	const double pi = boost::math::constants::pi<double>();

	audio_sink::sptr sink;
    	gr_fir_filter_ccf_sptr lpf;
	gr_fir_filter_fff_sptr sym_filter;
	gr_freq_xlating_fir_filter_ccf_sptr prefilter;
	gr_sig_source_c_sptr offset_sig; 

	gr_multiply_cc_sptr mixer;
	gr_file_sink_sptr fs;
	gr_file_sink_sptr fs2;
	gr_rational_resampler_base_ccf_sptr downsample_sig;
	gr_rational_resampler_base_fff_sptr upsample_audio;
	//gr::analog::quadrature_demod_cf::sptr demod;
	gr_quadrature_demod_cf_sptr demod;
	dsd_block_ff_sptr dsd;
	
	cout << "Control channel offset: " << offset << endl;
	cout << "Decim: " << decim << endl;
	cout << "Samples per symbol: " << samp_per_sym << endl;




	prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						       gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000),
						       offset, 
						       samp_rate);
	lpf = gr_make_fir_filter_ccf(decim, gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 2000));
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, design_filter(channel_rate, pre_channel_rate)); 

	d = GCD(audio_rate, vocoder_rate);
    	audio_rate = floor(audio_rate  / d);
    	vocoder_rate = floor(vocoder_rate / d);
	upsample_audio = gr_make_rational_resampler_base_fff(audio_rate, vocoder_rate, design_filter(audio_rate, vocoder_rate));

	demod = gr_make_quadrature_demod_cf(1.6);
	//demod = gr::analog::quadrature_demod_cf::make(1.6);
	const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};

   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data);
	fs = gr_make_file_sink(sizeof(float),"demod.dat");
	fs2 = gr_make_file_sink(sizeof(float),"sym.dat");
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1);
	sink = audio_make_sink(44100);
	
	//connect(self(), 0, mixer, 0);
	//connect(offset_sig, 0, mixer, 1);
	//connect(mixer, 0, lpf, 0);
	//connect(lpf, 0, downsample_sig, 0);
	tb->connect(src, 0, prefilter, 0);
	tb->connect(prefilter, 0, downsample_sig, 0);
	tb->connect(downsample_sig, 0, demod, 0);
	tb->connect(demod, 0, dsd, 0);	
	//tb->connect(demod, 0, sym_filter, 0);
	//tb->connect(sym_filter, 0, dsd, 0);
	tb->connect(dsd, 0, upsample_audio,0);
	tb->connect(upsample_audio, 0, sink,0);
	//tb->connect(demod,0,fs,0);
	//tb->connect(sym_filter,0,fs2,0);

	
	tb->run();

/*	tb->start();

	while (1) {
		if (!queue->empty_p())
		{
			std::string sentence;
			gr_message_sptr msg;
			msg = queue->delete_head();
			sentence = msg->to_string();
			parsefreq(sentence);	
			
		} else {
			
			boost::this_thread::sleep(boost::posix_time::milliseconds(1.0/10));
		}

	}
	
  */

  // Exit normally.
  return 0;
}
