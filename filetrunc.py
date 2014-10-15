#!/usr/bin/python
# Selects a subset of samples out of pre-recorded data
# PWG 10.15.2014

from gnuradio import gr
from gnuradio import blocks
from gnuradio.eng_option import eng_option
from gnuradio import eng_notation

import argparse
import os


class build_block(gr.top_block):
	def __init__(self,args):
		gr.top_block.__init__(self)
                ## Vars ##
		sample_rate=25e6
                num_secs=5;
                #sample_len=long(sample_rate*1)
                #sample_offset=long(sample_rate*num_secs);
                sample_offset=long(args.start);
                sample_len=long(args.nsamps);
                infile=args.input_file
                outfile=args.output_file
                print 'Sample Offset:' + repr(sample_offset)
                print 'Sample Length:' + repr(sample_len)
                print 'Input File:' + infile
                print 'Output File:' + outfile
                ## Blocks ##
		fsrc = blocks.file_source(gr.sizeof_short*1,infile,False)
                # SEEK_SET is offset from beginning, sample_offset is in samples
                fsrc.seek(sample_offset*2,os.SEEK_SET)
                short_to_cpx = blocks.interleaved_short_to_complex(False,False)
                # Copies sample_len then exits
                head = blocks.head(gr.sizeof_gr_complex*1,sample_len)
                #skip_head = blocks.skiphead(gr.sizeof_gr_complex*1,sample_offset)
                fsink = blocks.file_sink(gr.sizeof_gr_complex*1,outfile)
                #throttle = blocks.throttle(gr.sizeof_gr_complex*1,sample_rate,True)
                ## Connect Blocks
                self.connect(fsrc,short_to_cpx,head,fsink)
		#self.connect(src0, (dst,0))
		#self.connect(src1, (dst,1))
def main ():
   parser = argparse.ArgumentParser()
   parser.add_argument("input_file",help="Input filename")
   parser.add_argument("output_file",help="Output filename")
   parser.add_argument("-s","--start",type=long,
                     help="Starting sample index [default=%default]")
   parser.add_argument("-n","--nsamps",type=long,help="# samples")
   args = parser.parse_args()
   # Build the flowgraph
   tb = build_block (args)
   # Execute
   tb.run()
if __name__=='__main__':
   main ()


