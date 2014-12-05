#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: File Meta Sink
# Author: Example
# Generated: Thu Nov 13 16:37:56 2014
##################################################

from PyQt4 import Qt
from PyQt4.QtCore import QObject, pyqtSlot
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import gr, blocks
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import numpy
import sys
import time
import analysis

class file_meta_sink(gr.top_block):

    def __init__(self, param_samp_rate=1e6, param_gain=0, address="", param_freq=2.440e9):
        gr.top_block.__init__(self, "File Meta Sink")
        

        ##################################################
        # Parameters
        ##################################################
        self.param_samp_rate = param_samp_rate
        self.param_gain = param_gain
        self.address = address
        self.param_freq = param_freq

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 25000000
        self.gain = gain = 50
        self.freq = freq = param_freq
        self.ant = ant = "RX2"

        ##################################################
        # Blocks
        ##################################################
        
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join((address, "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_clock_rate(samp_rate, uhd.ALL_MBOARDS)
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_center_freq(freq, 0)
        self.uhd_usrp_source_0.set_gain(gain, 0)
        self.uhd_usrp_source_0.set_antenna(ant, 0)
        self.uhd_usrp_source_0.set_bandwidth(samp_rate, 0)
        self.blocks_file_meta_sink_0 = analysis.file_meta_sink(gr.sizeof_gr_complex*1, "/home/orin/idc/gr-analysis/testing/test1.dat", samp_rate, 1, blocks.GR_FILE_FLOAT, True, 1000000, "", False)
        self.blocks_file_meta_sink_0.set_unbuffered(False)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_file_meta_sink_0, 0))



if __name__ == '__main__':
    import ctypes
    import sys
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("-s", "--param-samp-rate", dest="param_samp_rate", type="eng_float", default=eng_notation.num_to_str(1e6),
        help="Set Sample Rate [default=%default]")
    parser.add_option("-g", "--param-gain", dest="param_gain", type="eng_float", default=eng_notation.num_to_str(0),
        help="Set Default Gain [default=%default]")
    parser.add_option("-a", "--address", dest="address", type="string", default="",
        help="Set IP Address [default=%default]")
    parser.add_option("-f", "--param-freq", dest="param_freq", type="eng_float", default=eng_notation.num_to_str(2.440e9),
        help="Set Default Frequency [default=%default]")
    (options, args) = parser.parse_args()
    if gr.enable_realtime_scheduling() != gr.RT_OK:
        print "Error: failed to enable realtime scheduling."
    tb = file_meta_sink(param_samp_rate=options.param_samp_rate, param_gain=options.param_gain, address=options.address, param_freq=options.param_freq)
    tb.start()
    tb.wait()
    tb = None #to clean up Qt widgets
