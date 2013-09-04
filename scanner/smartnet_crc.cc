//smartnet_crc.cc
/* -*- c++ -*- */
/*
 * Copyright 2012 Nick Foster
 * 
 * This file is part of gr_smartnet
 * 
 * gr_smartnet is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * gr_smartnet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smartnet_crc.h"
#include <gr_io_signature.h>
#include <stdio.h>
#include <gr_tags.h>
#include <sstream>
#include <smartnet_types.h>

#define VERBOSE 0

/*
 * Create a new instance of smartnet_crc and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
smartnet_crc_sptr smartnet_make_crc(gr_msg_queue_sptr queue)
{
  return smartnet_crc_sptr (new smartnet_crc (queue));
}

smartnet_crc::smartnet_crc (gr_msg_queue_sptr queue)
  : gr_sync_block ("crc",
                   gr_make_io_signature (1, 1, sizeof (char)),
                   gr_make_io_signature (0, 0, 0))
{
    set_output_multiple(38);
    d_queue = queue;
}

/*
 * Our virtual destructor.
 */
smartnet_crc::~smartnet_crc ()
{
    //nothing else required in this example
}

static void smartnet_ecc(char *out, const char *in) {
    char expected[76];
    char syndrome[76];

    //first we calculate the EXPECTED parity bits from the RECEIVED bitstream
    //parity is I[i] ^ I[i-1]
    //since the bitstream is still interleaved with the P bits, we can do this while running
    expected[0] = in[0] & 0x01; //info bit
    expected[1] = in[0] & 0x01; //this is a parity bit, prev bits were 0 so we call x ^ 0 = x
    for(int k = 2; k < 76*2; k+=2) {
	expected[k] = in[k] & 0x01; //info bit
	expected[k+1] = (in[k] & 0x01) ^ (in[k-2] & 0x01); //parity bit
    }

    for(int k = 0; k < 76; k++) {
	syndrome[k] = expected[k] ^ (in[k] & 0x01); //calculate the syndrome
	if(VERBOSE) if(syndrome[k]) std::cout << "Bit error at bit " << k << std::endl;
    }

    for(int k = 0; k < 38-1; k++) {
	//now we correct the data using the syndrome: if two consecutive
	//parity bits are flipped, you've got a bad previous bit
	if(syndrome[2*k+1] && syndrome[2*k+3]) {
	    out[k] = (in[2*k] & 0x01) ? 0 : 1; //byte-safe bit flip
	    if(VERBOSE) std::cout << "I just flipped a bit!" << std::endl;
	}
	else out[k] = in[2*k];
    }
}

static bool crc(const char *in) {
    unsigned int crcaccum = 0x0393;
    unsigned int crcop = 0x036E;
    unsigned int crcgiven;

    //calc expected crc
    for(int j=0; j<27; j++) {
	if(crcop & 0x01) crcop = (crcop >> 1)^0x0225;
	else crcop >>= 1;
	if (in[j] & 0x01) crcaccum = crcaccum ^ crcop;
    }

    //load given crc
    crcgiven = 0x0000;
    for(int j=0; j<10; j++) {
	crcgiven <<= 1;
	crcgiven += !bool(in[j+27] & 0x01);
    }

    return (crcgiven == crcaccum);
}

static smartnet_packet parse(const char *in) {
    smartnet_packet pkt;
    
    pkt.address = 0;
    pkt.groupflag = false;
    pkt.command = 0;
    pkt.crc = 0;

    int i=0;
    
    for(int k = 15; k >=0 ; k--) pkt.address += (!bool(in[i++] & 0x01)) << k; //first 16 bits are ID, MSB first
    pkt.groupflag = !bool(in[i++]);
    for(int k = 9; k >=0 ; k--) pkt.command += (!bool(in[i++] & 0x01)) << k; //next 10 bits are command, MSB first
    for(int k = 9; k >=0 ; k--) pkt.crc += (!bool(in[i++] & 0x01)) << k; //next 10 bits are CRC
    i++; //skip the guard bit

    //now correct things according to the mottrunk.txt description
    pkt.address ^= 0x33C7;
    pkt.command ^= 0x032A;
    
    return pkt;
}

int 
smartnet_crc::work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items)
{
    const char *in = (const char *) input_items[0];

    int size = noutput_items - 76;
    if(size <= 0) {
	return 0; //better luck next time
    }

    uint64_t abs_sample_cnt = nitems_read(0);
    std::vector<gr_tag_t> frame_tags;

    get_tags_in_range(frame_tags, 0, abs_sample_cnt, abs_sample_cnt + size, pmt::pmt_string_to_symbol("smartnet_frame"));
    if(frame_tags.size() == 0) {
	return 0; //sad trombone
    }

    std::vector<gr_tag_t>::iterator tag_iter;
    for(tag_iter = frame_tags.begin(); tag_iter != frame_tags.end(); tag_iter++) {
	uint64_t mark = tag_iter->offset - abs_sample_cnt;
	if(VERBOSE) std::cout << "found a frame at " << mark << std::endl;

	char databits[38];
	smartnet_ecc(databits, &in[mark]);
	bool crc_ok = crc(databits);

	if(crc_ok) {
	    if(VERBOSE) std::cout << "CRC OK" << std::endl;
	    //parse the message into readable chunks
	    smartnet_packet pkt = parse(databits);

	    //and throw it at the msgq
	    std::ostringstream payload;
	    payload.str("");
	    payload << pkt.address << "," << pkt.groupflag << "," << pkt.command;
	    gr_message_sptr msg = gr_make_message_from_string(std::string(payload.str()));
	    d_queue->handle(msg);
	} else if (VERBOSE) std::cout << "CRC FAILED" << std::endl;
    }
    return size;
}
