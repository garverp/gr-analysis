/* -*- c++ -*- */
/* 
 * Copyright 2015 Paul Garver
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
#include "statistics_impl.h"

namespace gr {
  namespace analysis {

    statistics::sptr
    statistics::make()
    {
      return gnuradio::get_initial_sptr
        (new statistics_impl());
    }

    /*
     * The private constructor
     */
    statistics_impl::statistics_impl()
      : gr::sync_block("statistics",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
       // Init vars
       max_index = 0;
       min_index = 0;
       max_val = 0;
       min_val = 0;
       mean = 0;
       M2 = 0;
    }

    /*
     * Our virtual destructor.
     */
    statistics_impl::~statistics_impl()
    {
    }

    int
    statistics_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];
        // Algorithm from Welford, B.P. "Note on a Method for Calculating Corrected Sums 
        // of Squares and Products" Technometrics Aug. 1962 Vol. 4 No. 3 pp. 419-420
        // Also see "Algorithms for computing the sample variance: analysis and
        // Recommendations" Tony F. Chan et al. American Statistician
        // S_1 = 0, M_1 = x_1 = x[0]
        if( nitems_written(0) == 0 ){
           mean = in[0];
           out[0] = in[0];
           return 1;
        }
        gr_complex n = gr_complex(nitems_written(0)+1);
        gr_complex nm1 = n-gr_complex(1);
        gr_complex delta = 0.0;
        gr_complex deltaovern = 0.0;
        for( int i = 0; i < noutput_items; i++ ){
           delta = in[i]-mean;
           deltaovern=delta/n;
           // or pow(delta,2) faster?
           M2 += nm1*delta*deltaovern;
           mean += deltaovern;
        }
        std::cout << "mean=" << mean << std::endl;
        std::cout << "var=" << M2/(n-gr_complex(1)) << std::endl;

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace analysis */
} /* namespace gr */

