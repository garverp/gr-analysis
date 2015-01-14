/* -*- c++ -*- */
/* 
 * Copyright 2015 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "stream_max_impl.h"

namespace gr {
  namespace analysis {

    stream_max::sptr
    stream_max::make()
    {
      return gnuradio::get_initial_sptr
        (new stream_max_impl());
    }

    /*
     * The private constructor
     */
    stream_max_impl::stream_max_impl()
      : gr::block("stream_max",
              gr::io_signature::make(1,1, sizeof(float)),
              gr::io_signature::make(1,1, sizeof(float))){
        max_value = 0;
        max_location = 0;
    }

    /*
     * Our virtual destructor.
     */
    stream_max_impl::~stream_max_impl()
    {
    }

    void
    stream_max_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        ninput_items_required[0] = noutput_items;
    }

    int
    stream_max_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const float *in = (const float *) input_items[0];
        float *out = (float*) output_items[0];

        // Do <+signal processing+>
        for(int i =0; i < noutput_items; i++){
          if( in[i]  > max_value ){
            max_value = in[i];
            max_location = nitems_read(0)+i;
           std::cout << "max_val=" << max_value << " @ " << max_location 
                     << std::endl;
          }
          out[i] = max_value;
        }

        // Tell runtime system how many input items we consumed on
        // each input stream.
        consume_each (noutput_items);

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace analysis */
} /* namespace gr */

