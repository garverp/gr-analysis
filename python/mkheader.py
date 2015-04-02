#!/usr/bin/python
# Create a new header file (and eventually modify existing ones)
# PWG 10.15.2014

from gnuradio import gr
from gnuradio import eng_notation
from gnuradio import blocks
from gnuradio.blocks import parse_file_metadata
import pmt
import sys

n2s = eng_notation.num_to_str

    
# Defining Tuple for data types: (itemsize,cpx,format)
SC16_DEFS = (gr.sizeof_short*2,True,blocks.GR_FILE_SHORT)
FC32_DEFS = (gr.sizeof_gr_complex,True,blocks.GR_FILE_FLOAT)
B8_DEFS = (gr.sizeof_char,False,blocks.GR_FILE_BYTE)
ENUM_TO_SNAME = {0:'unknown', 1:'sc16', 2:'fc32', 3:'b8'}
SNAME_DEFS = {1:SC16_DEFS, 2:FC32_DEFS, 3:B8_DEFS}
SNAME_TO_ENUM = {v: k for k, v in ENUM_TO_SNAME.items()}
STRING_TO_FTYPE= {v: k for k, v in parse_file_metadata.ftype_to_string.items()}


def find_shortname(cpx, the_type, item_size):
    shortname_type = SNAME_TO_ENUM["unknown"]
    the_type = STRING_TO_FTYPE[the_type]
    # Interleaved Short
    if cmp(SNAME_DEFS[1], (item_size, cpx, the_type)) == 0:
        return SNAME_TO_ENUM["sc16"]
    # Complex Float
    if cmp(SNAME_DEFS[2], (item_size, cpx, the_type)) == 0:
        return SNAME_TO_ENUM["fc32"]
    # Bytes
    if cmp(SNAME_DEFS[3], (item_size, cpx, the_type)) == 0:
        return SNAME_TO_ENUM["b8"]
    return shortname_type

def update_timestamp(hdr,seg_size):
    if pmt.dict_has_key(hdr, pmt.string_to_symbol("rx_time")):
        r = pmt.dict_ref(hdr, pmt.string_to_symbol("rx_time"), pmt.PMT_NIL)
        secs = pmt.tuple_ref(r, 0)
        fracs = pmt.tuple_ref(r, 1)
        secs = float(pmt.to_uint64(secs))
        fracs = pmt.to_double(fracs)
        t = secs + fracs
    else:
        sys.stderr.write("Could not find key 'time': \
                invalid or corrupt data file.\n")
        sys.exit(1)
    new_hdr = pmt.dict_delete(hdr, pmt.intern("rx_time"))
    if pmt.dict_has_key(hdr, pmt.intern("rx_rate")):
        r = pmt.dict_ref(hdr, pmt.intern("rx_rate"), pmt.PMT_NIL)
        rate = pmt.to_double(r)
        new_t = t + float(seg_size)/rate
        new_secs = long(new_t)
        new_fracs = new_t - new_secs
        time_val = pmt.make_tuple(pmt.from_uint64(new_secs),
                             pmt.from_double(new_fracs))
        new_hdr = pmt.dict_add(new_hdr, pmt.intern("rx_time"), time_val)
        return new_hdr


def make_header(options, filename):
    extras_present = False
    if options.freq is not None:
        extras_present = True
    # Open the file and make the header
    hdr_filename = filename + '.hdr'
    hdr_file = open(hdr_filename, 'wb')
    header = pmt.make_dict()
    # Fill in header vals
    # TODO - Read this from blocks.METADATA_VERSION
    ver_val = pmt.from_long(long(0))
    rate_val = pmt.from_double(options.sample_rate)
    time_val = pmt.make_tuple(pmt.from_uint64(options.time_sec),
                             pmt.from_double(options.time_fsec))
    ft_to_sz = parse_file_metadata.ftype_to_size
    # Map shortname to properties
    enum_type = SNAME_TO_ENUM[options.format]
    type_props = SNAME_DEFS[enum_type]
    size_val = pmt.from_long(type_props[0])
    cplx_val = pmt.from_bool(type_props[1])
    type_val = pmt.from_long(type_props[2])
    fmt = type_props[2]
    file_samp_len = long(options.length)
    seg_size = long(options.seg_size)
    bytes_val = pmt.from_uint64(long(seg_size*ft_to_sz[fmt]))
    # Set header vals
    header = pmt.dict_add(header, pmt.intern("version"), ver_val)
    header = pmt.dict_add(header, pmt.intern("size"), size_val)
    header = pmt.dict_add(header, pmt.intern("type"), type_val)
    header = pmt.dict_add(header, pmt.intern("cplx"), cplx_val)
    header = pmt.dict_add(header, pmt.intern("rx_time"), time_val)
    header = pmt.dict_add(header, pmt.intern("rx_rate"), rate_val)
    header = pmt.dict_add(header, pmt.intern("bytes"), bytes_val)

    if extras_present:
        freq_key = pmt.intern("rx_freq")
        freq_val = pmt.from_double(options.freq)
        extras = pmt.make_dict()
        extras = pmt.dict_add(extras, freq_key, freq_val)
        extras_str = pmt.serialize_str(extras)
        start_val = pmt.from_uint64(blocks.METADATA_HEADER_SIZE
                + len(extras_str))
    else:
        start_val = pmt.from_uint64(blocks.METADATA_HEADER_SIZE)
    header = pmt.dict_add(header, pmt.intern("strt"), start_val)
    num_segments = file_samp_len/seg_size
    if options.verbose:
        print "Wrote %d headers to: %s (Version %d)" % (num_segments+1,
                hdr_filename,pmt.to_long(ver_val))
    for x in range(0,num_segments,1):
        # Serialize and write out file
        if extras_present:
            header_str = pmt.serialize_str(header) + extras_str
        else:
            header_str = pmt.serialize_str(header)
        hdr_file.write(header_str)
        # Update header based on sample rate and segment size
        header = update_timestamp(header,seg_size)
    
    # Last header is special b/c file size is probably not mult. of seg_size
    header = pmt.dict_delete(header,pmt.intern("bytes"))
    bytes_remaining = ft_to_sz[fmt]*(file_samp_len - num_segments*long(seg_size))
    bytes_val = pmt.from_uint64(bytes_remaining)
    header = pmt.dict_add(header,pmt.intern("bytes"),bytes_val)
    # Serialize and write out file
    if extras_present:
        header_str = pmt.serialize_str(header) + extras_str
    else:
        header_str = pmt.serialize_str(header)
    hdr_file.write(header_str)
    hdr_file.close()
