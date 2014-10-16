#!/usr/bin/python

# Create a new header file
# PWG 10.15.2014

from gnuradio import gr
from gnuradio import blocks
from gnuradio.blocks import parse_file_metadata
import pmt

# Open the file and make the header
hdr_filename = "filter.hdr"
hdr_file = open(hdr_filename,'wb')
header = pmt.make_dict()

# Fill in header vals
# TODO - Read this from blocks.METADATA_VERSION
ver_val = pmt.from_long(long(0))
rate_val = pmt.from_double(25e6)
time_val = pmt.make_tuple(pmt.from_uint64(0),pmt.from_double(0))
size_val = pmt.from_long(parse_file_metadata.ftype_to_size[blocks.GR_FILE_FLOAT])
type_val = pmt.from_long(blocks.GR_FILE_FLOAT)
cplx_val = pmt.PMT_T
start_val = pmt.from_uint64(blocks.METADATA_HEADER_SIZE + 0)
bytes_val = pmt.from_uint64(25000000*2*parse_file_metadata.ftype_to_size[blocks.GR_FILE_FLOAT])

# Set header vals
header = pmt.dict_add(header,pmt.intern("version"),ver_val)
header = pmt.dict_add(header,pmt.intern("size"),size_val)
header = pmt.dict_add(header,pmt.intern("type"),type_val)
header = pmt.dict_add(header,pmt.intern("cplx"),cplx_val)
header = pmt.dict_add(header,pmt.intern("strt"),start_val)
header = pmt.dict_add(header,pmt.intern("bytes"),bytes_val)
header = pmt.dict_add(header,pmt.intern("rx_rate"),rate_val)
header = pmt.dict_add(header,pmt.intern("rx_time"),time_val)

# Serialize and write out file
header_str = pmt.serialize_str(header)
hdr_file.write(header_str)
