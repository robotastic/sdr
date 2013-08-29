
#include "logging_receiver_dsd.h"

log_dsd_sptr make_log_dsd(float freq, float center)
{
    return gnuradio::get_initial_sptr(new log_dsd(freq, center));
}
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

	std::cout << "Inter: " << interpolation << " mid: " << mid_transition_band/interpolation <<  "trans: " << trans_width/interpolation << std::endl;  
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
log_dsd::log_dsd(float f, float c)
    : gr_hier_block2 ("log_dsd",
          gr_make_io_signature  (1, 1, sizeof(gr_complex)),
          gr_make_io_signature  (1, 1, sizeof(float)))
{
	freq = f;
	center = c;
	float offset = center - 856987500.0;
	int samp_per_sym = 10;
	double samp_rate = 8000000;
	int decim = 160;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = double(samp_rate/decim);
	double vocoder_rate = 8000;
	double audio_rate = 44100;

	std::cout << pre_channel_rate << " " << channel_rate << std::endl;




    
	//offset_sig = gr_make_sig_source_c(samp_rate, GR_SIN_WAVE, offset , 1.0, 0.0);
	//mixer = gr_make_multiply_cc();
	prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						       gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000),
						       offset, 
						       samp_rate);
	//lpf = gr_make_fir_filter_ccf(decim, gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 2000));
	



	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, design_filter(channel_rate, pre_channel_rate)); 


	

	d = GCD(audio_rate, vocoder_rate);
    	audio_rate = floor(audio_rate  / d);
    	vocoder_rate = floor(vocoder_rate / d);
	upsample_audio = gr_make_rational_resampler_base_fff(audio_rate, vocoder_rate, design_filter(audio_rate, vocoder_rate));

	//upsample_audio = gr_make_rational_resampler_base_fff(44100, 8000, std::vector<float>(1,0));
	
	demod = gr_make_quadrature_demod_cf(1.6);
	//demod = gr::analog::quadrature_demod_cf::make(1.6);
	const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};

   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data); //std::vector<float>(10, 0.1 ));
	fs = gr_make_file_sink(sizeof(float),"demod.dat");
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1);
	
	//connect(self(), 0, mixer, 0);
	//connect(offset_sig, 0, mixer, 1);
	//connect(mixer, 0, lpf, 0);
	//connect(lpf, 0, downsample_sig, 0);
	connect(self(), 0, prefilter, 0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, upsample_audio,0);
	connect(upsample_audio, 0, self(),0);
	//connect(sym_filter,0,fs,0);

}

log_dsd::~log_dsd() {

}

void log_dsd::tune_offset(float f) {
	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "Offset set to: " << f*1000000-center << std::endl;
}
	

	
