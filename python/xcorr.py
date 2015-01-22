#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Xcorr
# Generated: Thu Nov 20 13:20:30 2014
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import qtgui
from gnuradio import filter
from gnuradio import gr
import analysis
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
import argparse,sys,sip
from PyQt4 import QtGui
try:
    import scipy
except ImportError:
    print "Error: Scipy required (www.scipy.org)."
    sys.exit(1)

class xcorr(gr.top_block):

    def __init__(self,args):
        gr.top_block.__init__(self, "Xcorr")
        #self.qapp = QtGui.QApplication(sys.argv)
        ##################################################
        # Variables
        ##################################################
        self.xcorr_threads = xcorr_threads = 8
        self.decimation = 1
        self.samp_rate = samp_rate = 25e6
        self.infile=args.input_file
        self.patfile = args.pattern_file
        self.outfile = args.output_file

        # Read the pattern file into memory
        fhandle = open(self.patfile,'r')
        patdata = scipy.fromfile(fhandle,dtype=scipy.complex64)
        # Time-reverse to make a correlation vs. convolution
        # A faster way to do this is to do a conjugation of 
        # one of the vectors in the frequency domain
        # TODO - Add a conjugate flag to fft_filter primitive
        patdata = scipy.flipud(patdata)
        # Enforce L-2 norm of pat = 1.0
        #patdata = scipy.multiply(patdata,9.046e-4+9.046e-4j)
        print "Pattern file is {0} samples".format(len(patdata))
        print patdata
        taps = patdata.tolist()
        fhandle.close()

        ##################################################
        # Blocks
        ##################################################
        self.fft_filter_xxx_0 = filter.fft_filter_ccc(self.decimation, taps, 
                xcorr_threads)
        self.fft_filter_xxx_0.declare_sample_delay(0)
        self.nsamps = self.fft_filter_xxx_0.set_taps(taps)
        print "self.nsamps is {0}".format(self.nsamps)
        self.blocks_file_source_0 = blocks.file_source(gr.sizeof_gr_complex*1, 
                self.infile, False)
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_float*1, 
                self.outfile, False)
        self.blocks_file_sink_0.set_unbuffered(True)
        self.blocks_cpx_to_mag = blocks.complex_to_mag(1)
        self.blocks_stream_max = analysis.stream_max()
        self.blocks_null_sink = blocks.null_sink(gr.sizeof_float)
        #self.blocks_corr_plot = qtgui.time_sink_f(1000000,self.samp_rate,
        #        "Cross-Correlation")

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_file_source_0, 0), (self.fft_filter_xxx_0, 0))
        #self.connect((self.fft_filter_xxx_0, 0), (self.blocks_file_sink_0, 0))
        self.connect((self.fft_filter_xxx_0,0),self.blocks_cpx_to_mag,
                self.blocks_stream_max,self.blocks_null_sink)
        #self.connect(self.blocks_cpx_to_mag,self.blocks_corr_plot)
        self.connect(self.blocks_cpx_to_mag,self.blocks_file_sink_0)
        # More QT stuff
        #self.pyobj = sip.wrapinstance(self.blocks_corr_plot.pyqwidget(),
        #        QtGui.QWidget)
        #self.pyobj.show()


    def get_xcorr_threads(self):
        return self.xcorr_threads

    def set_xcorr_threads(self, xcorr_threads):
        self.xcorr_threads = xcorr_threads
        self.fft_filter_xxx_0.set_nthreads(self.xcorr_threads)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file",help="Input file")
    parser.add_argument("pattern_file",help="Pattern file")
    parser.add_argument("output_file",help="Output file")
    args = parser.parse_args()
    tb = xcorr(args)
    tb.run()
    #tb.start()
    #tb.qapp.exec_()
if __name__ == '__main__':
    main()
