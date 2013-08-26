#ifndef LPF_H
#define LPF_H

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <boost/shared_ptr.hpp>
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

class log_dsd;

typedef boost::shared_ptr<log_dsd> log_dsd_sptr;

log_dsd_sptr make_log_dsd(float f, float c);

class log_dsd : public gr_hier_block2
{
  friend log_dsd_sptr make_log_dsd(float f, float c);
protected:
    log_dsd(float f, float c);

public:
    ~log_dsd();
	void tune_offset(float f);
	void mute();
	void unmute();	

private:
	float center, freq;

    /* GR blocks */
    	gr_fir_filter_ccf_sptr lpf;
	gr_fir_filter_fff_sptr sym_filter;
	gr_freq_xlating_fir_filter_ccf_sptr prefilter;
	gr_sig_source_c_sptr offset_sig; 

	gr_multiply_cc_sptr mixer;
	gr_file_sink_sptr fs;

	gr_rational_resampler_base_ccf_sptr downsample_sig;
	gr_rational_resampler_base_fff_sptr upsample_audio;
	gr::analog::quadrature_demod_cf::sptr demod;
	//gr_quadrature_demod_cf_sptr demod;
	dsd_block_ff_sptr dsd;


};


#endif

