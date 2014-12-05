#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Wifi Rx Cli
# Generated: Thu Dec  4 14:14:59 2014
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import fft
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from optparse import OptionParser
import foo
import ieee802_11
import numpy

class wifi_rx_cli(gr.top_block):

    def __init__(self,args):
        gr.top_block.__init__(self, "Wifi Rx Cli")

        ##################################################
        # Variables
        ##################################################
        self.window_size = window_size = 48
        self.sync_length = sync_length = 320
        self.samp_rate = samp_rate = 25e6
        self.infile = args.input_file
        print 'Processing: %s' % self.infile

        ##################################################
        # Blocks
        ##################################################
        self.rational_resampler_xxx_0 = filter.rational_resampler_ccc(
                interpolation=4,
                decimation=5,
                taps=None,
                fractional_bw=None,
        )
        self.ieee802_11_ofdm_sync_short_0 = ieee802_11.ofdm_sync_short(0.85 * 0.8, 80 * 160, 2, False, False)
        self.ieee802_11_ofdm_sync_long_0 = ieee802_11.ofdm_sync_long(sync_length, False, False)
        self.ieee802_11_ofdm_parse_mac_0 = ieee802_11.ofdm_parse_mac(False, False)
        self.ieee802_11_ofdm_equalize_symbols_0 = ieee802_11.ofdm_equalize_symbols(False)
        self.ieee802_11_ofdm_decode_signal_0 = ieee802_11.ofdm_decode_signal(True, False)
        self.ieee802_11_ofdm_decode_mac_0 = ieee802_11.ofdm_decode_mac(True, False)
        self.ieee802_11_moving_average_xx_1 = ieee802_11.moving_average_ff(window_size + 16)
        self.ieee802_11_moving_average_xx_0 = ieee802_11.moving_average_cc(window_size)
        self.foo_wireshark_connector_0 = foo.wireshark_connector(127, False)
        self.fft_vxx_0 = fft.fft_vcc(64, True, (window.rectangular(64)), True, 1)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_gr_complex*48, "ofdm_decode_mac", ""); self.blocks_tag_debug_0.set_display(False)
        self.blocks_stream_to_vector_0 = blocks.stream_to_vector(gr.sizeof_gr_complex*1, 64)
        self.blocks_multiply_xx_0 = blocks.multiply_vcc(1)
        self.blocks_interleaved_short_to_complex_0 = blocks.interleaved_short_to_complex(False, False)
        self.blocks_file_source_0 = blocks.file_source(gr.sizeof_short*1,self.infile, False)
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_char*1, "/dev/null", False)
        self.blocks_file_sink_0.set_unbuffered(True)
        self.blocks_divide_xx_0 = blocks.divide_ff(1)
        self.blocks_delay_0_0 = blocks.delay(gr.sizeof_gr_complex*1, 16)
        self.blocks_delay_0 = blocks.delay(gr.sizeof_gr_complex*1, sync_length)
        self.blocks_conjugate_cc_0 = blocks.conjugate_cc()
        self.blocks_complex_to_mag_squared_0 = blocks.complex_to_mag_squared(1)
        self.blocks_complex_to_mag_0 = blocks.complex_to_mag(1)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_stream_to_vector_0, 0), (self.fft_vxx_0, 0))
        self.connect((self.blocks_delay_0, 0), (self.ieee802_11_ofdm_sync_long_0, 1))
        self.connect((self.ieee802_11_ofdm_sync_short_0, 0), (self.blocks_delay_0, 0))
        self.connect((self.ieee802_11_ofdm_sync_long_0, 0), (self.blocks_stream_to_vector_0, 0))
        self.connect((self.ieee802_11_ofdm_equalize_symbols_0, 0), (self.ieee802_11_ofdm_decode_signal_0, 0))
        self.connect((self.ieee802_11_ofdm_decode_signal_0, 0), (self.ieee802_11_ofdm_decode_mac_0, 0))
        self.connect((self.fft_vxx_0, 0), (self.ieee802_11_ofdm_equalize_symbols_0, 0))
        self.connect((self.blocks_complex_to_mag_0, 0), (self.blocks_divide_xx_0, 0))
        self.connect((self.ieee802_11_moving_average_xx_1, 0), (self.blocks_divide_xx_0, 1))
        self.connect((self.blocks_multiply_xx_0, 0), (self.ieee802_11_moving_average_xx_0, 0))
        self.connect((self.ieee802_11_moving_average_xx_0, 0), (self.blocks_complex_to_mag_0, 0))
        self.connect((self.blocks_complex_to_mag_squared_0, 0), (self.ieee802_11_moving_average_xx_1, 0))
        self.connect((self.blocks_delay_0_0, 0), (self.ieee802_11_ofdm_sync_short_0, 0))
        self.connect((self.blocks_conjugate_cc_0, 0), (self.blocks_multiply_xx_0, 1))
        self.connect((self.blocks_delay_0_0, 0), (self.blocks_conjugate_cc_0, 0))
        self.connect((self.foo_wireshark_connector_0, 0), (self.blocks_file_sink_0, 0))
        self.connect((self.blocks_file_source_0, 0), (self.blocks_interleaved_short_to_complex_0, 0))
        self.connect((self.rational_resampler_xxx_0, 0), (self.blocks_delay_0_0, 0))
        self.connect((self.rational_resampler_xxx_0, 0), (self.blocks_complex_to_mag_squared_0, 0))
        self.connect((self.rational_resampler_xxx_0, 0), (self.blocks_multiply_xx_0, 0))
        self.connect((self.blocks_interleaved_short_to_complex_0, 0), (self.rational_resampler_xxx_0, 0))
        self.connect((self.blocks_divide_xx_0, 0), (self.ieee802_11_ofdm_sync_short_0, 1))
        self.connect((self.ieee802_11_ofdm_sync_short_0, 0), (self.ieee802_11_ofdm_sync_long_0, 0))
        self.connect((self.ieee802_11_ofdm_decode_signal_0, 0), (self.blocks_tag_debug_0, 0))

        ##################################################
        # Asynch Message Connections
        ##################################################
        self.msg_connect(self.ieee802_11_ofdm_decode_mac_0, "out", self.ieee802_11_ofdm_parse_mac_0, "in")
        self.msg_connect(self.ieee802_11_ofdm_decode_mac_0, "out", self.foo_wireshark_connector_0, "in")


    def get_window_size(self):
        return self.window_size

    def set_window_size(self, window_size):
        self.window_size = window_size
        self.ieee802_11_moving_average_xx_0.set_length(self.window_size)
        self.ieee802_11_moving_average_xx_1.set_length(self.window_size + 16)

    def get_sync_length(self):
        return self.sync_length

    def set_sync_length(self, sync_length):
        self.sync_length = sync_length
        self.blocks_delay_0.set_dly(self.sync_length)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    if len(args) < 1:
      print 'No Input file given'
      exit(1)
    tb = wifi_rx_cli(args)
    tb.start()
    tb.wait()
