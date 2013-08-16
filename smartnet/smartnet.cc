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
#include <gr_top_block.h>
#include <osmosdr_source_c.h>
#include <osmosdr_sink_c.h>
#include <gr_sig_source_f.h>
#include <gr_audio_sink.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char **argv)
{
std::string device_addr;
    double center_freq, samp_rate;
	int if_gain, bb_gain, rf_gain;
    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("arg", po::value<std::string>(&device_addr)->default_value(""), "the device arguments in string format")
        ("rate", po::value<double>(&samp_rate)->default_value(1e6), "the sample rate in samples per second")
        ("freq", po::value<double>(&center_freq)->default_value(10e6), "the center frequency in Hz")
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




  int rate = 48000;		// Audio card sample rate
  float ampl = 0.1;		// Don't exceed 0.5 or clipping will occur

  // Construct a top block that will contain flowgraph blocks.  Alternatively,
  // one may create a derived class from gr_top_block and hold instantiated blocks
  // as member data for later manipulation.
  gr_top_block_sptr tb = gr_make_top_block("smartnet");

	
	osmosdr_source_c_sptr src = osmosdr_make_source_c();

  // Construct a real-valued signal source for each tone, at given sample rate
  gr_sig_source_f_sptr src0 = gr_make_sig_source_f(rate, GR_SIN_WAVE, 350, ampl);
  gr_sig_source_f_sptr src1 = gr_make_sig_source_f(rate, GR_SIN_WAVE, 440, ampl);

  // Construct an audio sink to accept audio tones
  audio_sink::sptr sink = audio_make_sink(rate);

  // Connect output #0 of src0 to input #0 of sink (left channel)
  tb->connect(src0, 0, sink, 0);

  // Connect output #0 of src1 to input #1 of sink (right channel)
  tb->connect(src1, 0, sink, 1);

  // Tell GNU Radio runtime to start flowgraph threads; the foreground thread
  // will block until either flowgraph exits (this example doesn't) or the
  // application receives SIGINT (e.g., user hits CTRL-C).
  //
  // Real applications may use tb->start() which returns, allowing the foreground
  // thread to proceed, then later use tb->stop(), followed by tb->wait(), to cleanup
  // GNU Radio before exiting.
  tb->run();

  // Exit normally.
  return 0;
}
