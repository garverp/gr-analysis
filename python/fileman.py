#!/usr/bin/python
# Selects a subset of samples out of pre-recorded data
# Support for headers

from gnuradio import gr
from gnuradio import blocks
from gnuradio.blocks import parse_file_metadata
from gnuradio.eng_option import eng_option
import pmt
import sys
import os

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

# Read a single header, return metadata dictionary
def read_single_header(handle):
    nread = handle.tell()
    header_str = handle.read(parse_file_metadata.HEADER_LENGTH)
    if len(header_str) == 0:
        sys.stderr.write("Empty Header, quitting.\n")
        sys.exit(1)
    # Convert from string to PMT (should be a dictionary)
    try:
        header = pmt.deserialize_str(header_str)
    except RuntimeError:
        sys.stderr.write("Could not deserialize header: invalid or \
                corrupt data file.\n")
        sys.exit(1)
    info = parse_file_metadata.parse_header(header, False)
    extras_str = handle.read(info["extra_len"])
    try:
        extras_hdr = pmt.deserialize_str(extras_str)
    except RuntimeError:
        sys.stderr.write("Could not deserialize extras\n")
        sys.exit(1)
    nread += parse_file_metadata.HEADER_LENGTH + info["extra_len"]
    handle.seek(nread, 0)
    return header, extras_hdr, handle

# Returns (infile,intype,outfile,outtype,sample_offset,sample_len)
def propagate_headers(options,args):
    infile = args[0]
    outfile = args[1]
    infile_hdr = infile + '.hdr'
    outfile_hdr = outfile + '.hdr'
    sample_cnt_end = 0
    sample_offset = long(options.start)
        # Open input header
    try:
        handle_in = open(infile_hdr, "rb")
    except IOError:
        sys.stderr.write("Unable to open input file header\n")
        sys.exit(1)
    # Open output header
    try:
        handle_out = open(outfile_hdr, "wb")
    except IOError:
        sys.stderr.write("Unable to open output file header\n")
        sys.exit(1)

    # Read first header separately to get file type
    hdr_in, hdr_extra_in, handle_in = read_single_header(handle_in)
    info_in = parse_file_metadata.parse_header(hdr_in,False)
    sample_cnt_end += info_in["nitems"]
    # Parse file type - ensure support for it
    shortname_intype = find_shortname(info_in['cplx'], info_in['type'],
                info_in['size'])
    if shortname_intype == SNAME_TO_ENUM["unknown"]:
        sys.stderr.write("Unsupported data type\n")
        sys.exit(1)
    if options.output_type == 'unknown':
        shortname_outtype = shortname_intype
    else:
        shortname_outtype = SNAME_TO_ENUM[options.output_type]

    # Calc sample_len from file size if not specified
    if options.nsamples is not None:
        sample_len = long(options.nsamples)
    else:
        sample_len = os.path.getsize(infile)/SNAME_DEFS[shortname_intype][0]
    final_index = sample_offset + sample_len 

    # Search input headers until we find the correct one
    while sample_cnt_end <= sample_offset:
        hdr_in, hdr_extra_in, handle_in = read_single_header(handle_in)
        info_in = parse_file_metadata.parse_header(hdr_in,False)
        sample_cnt_end += info_in["nitems"]
    time_in = info_in["rx_time"]
    # Starting sample of current segment
    sample_cnt_start = sample_cnt_end - info_in["nitems"]
    # Interpolate new timestamp
    delta = sample_offset - sample_cnt_start
    new_ts = time_in + delta/info_in["rx_rate"]
    # Calc new segment size (samples)
    if sample_cnt_end > final_index:
        first_seg_len = final_index - sample_offset
    else:
        first_seg_len = sample_cnt_end - sample_offset
    
    # Write the first output header
    hdr_out = hdr_in
    new_secs = long(new_ts)
    new_fracs = new_ts - new_secs
    time_val = pmt.make_tuple(pmt.from_uint64(new_secs),
            pmt.from_double(new_fracs))
    size_val = pmt.from_long(SNAME_DEFS[shortname_outtype][0])
    bytes_val = pmt.from_uint64(first_seg_len*SNAME_DEFS[shortname_outtype][0])
    type_val = pmt.from_long(SNAME_DEFS[shortname_outtype][2])
    hdr_out = pmt.dict_add(hdr_out, pmt.intern("rx_time"), time_val)
    hdr_out = pmt.dict_add(hdr_out, pmt.intern("bytes"), bytes_val)
    hdr_out = pmt.dict_add(hdr_out, pmt.intern("type"), type_val)
    hdr_out = pmt.dict_add(hdr_out, pmt.intern("size"), size_val)
    hdr_out_str = pmt.serialize_str(hdr_out) + pmt.serialize_str(hdr_extra_in)    
    handle_out.write(hdr_out_str)

    # Continue reading headers, modifying, and writing 
    last_seg_len = info_in['nitems']
    print "sample_cnt_end=%d,final_index=%d" % (sample_cnt_end,final_index)
    # Iterate through remaining headers
    while sample_cnt_end < final_index:
        hdr_in, hdr_extra_in, handle_in = read_single_header(handle_in)
        info_in = parse_file_metadata.parse_header(hdr_in,False)
        nitems = info_in["nitems"]
        sample_cnt_start = sample_cnt_end
        sample_cnt_end += nitems
        hdr_out = hdr_in
        # For last header, adjust segment length accordingly
        if sample_cnt_end > final_index:
            last_seg_len = final_index - sample_cnt_start
        else:
            last_seg_len = nitems
        size_val = pmt.from_long(SNAME_DEFS[shortname_outtype][0])
        bytes_val = pmt.from_uint64(last_seg_len*SNAME_DEFS[shortname_outtype][0])
        type_val = pmt.from_long(SNAME_DEFS[shortname_outtype][2])
        hdr_out = pmt.dict_add(hdr_out, pmt.intern("bytes"), bytes_val)
        hdr_out = pmt.dict_add(hdr_out, pmt.intern("type"), type_val)
        hdr_out = pmt.dict_add(hdr_out, pmt.intern("size"), size_val)
        hdr_out_str = pmt.serialize_str(hdr_out) + pmt.serialize_str(hdr_extra_in)
        handle_out.write(hdr_out_str)
        
    if options.verbose:
        print 'Input File:' + infile
        print 'Input Header:' + infile_hdr
        print 'Input Type:' + ENUM_TO_SNAME[shortname_intype]
        print 'Output File:' + outfile
        print 'Output File Length (Samples):%d' % (final_index-sample_offset)
        print 'Output Header:' + outfile_hdr
        print 'File subsection: [%d,%d]' % (sample_offset,final_index)
        print 'Output Type:' + ENUM_TO_SNAME[shortname_outtype]
        print 'First Segment Length: %e samples' % first_seg_len
        print 'Last Segment Length: %e samples' % last_seg_len
        print 'delta=%f,new ts=%f' % (delta,new_ts)

    # Clean up
    handle_in.close()
    handle_out.close()

    # Return header info
    return {'infile':infile,'intype':shortname_intype,'outfile':outfile,
            'outtype':shortname_outtype,'sample_offset':sample_offset,
            'sample_len':sample_len}

class buildblock(gr.top_block):
    def __init__(self,config):
        gr.top_block.__init__(self)
        ## Vars ##
        ## Interleaved Short input (sc16)
        if config['intype'] == SNAME_TO_ENUM['sc16']:
            fsrc = blocks.file_source(gr.sizeof_short*1,config['infile'], False)
            # SEEK_SET is offset from beginning, sample_offset is in samples
            # Factor of 2 because vector of length 2 (since interleaved)
            fsrc.seek(config['sample_offset']*2, os.SEEK_SET)
            # sc16->fc32
            if config['outtype'] == SNAME_TO_ENUM['fc32']:
                ishort_to_cpxfloat = blocks.interleaved_short_to_complex(False, False)
                # Copies sample_len then exits
                head = blocks.head(gr.sizeof_gr_complex*1, config['sample_len'])
                fsink = blocks.file_sink(gr.sizeof_gr_complex*1, config['outfile'])
                self.connect(fsrc, ishort_to_cpxfloat, head, fsink)
            # sc16->sc16
            elif config['outtype'] == SNAME_TO_ENUM['sc16']:
                head = blocks.head(gr.sizeof_short, config['sample_len'])
                fsink = blocks.file_sink(gr.sizeof_short, config['outfile'])
                self.connect(fsrc, head, fsink)
        ## Complex float input (fc32)
        elif config['intype'] == SNAME_TO_ENUM['fc32']:
            fsrc = blocks.file_source(gr.sizeof_gr_complex*1, config['infile'], False)
            # SEEK_SET is offset from beginning, sample_offset is in samples
            fsrc.seek(config['sample_offset'], os.SEEK_SET)
            # fc32->fc32
            if config['outtype'] == SNAME_TO_ENUM['fc32']:
                head = blocks.head(gr.sizeof_gr_complex*1, config['sample_len'])
                fsink = blocks.file_sink(gr.sizeof_gr_complex*1, config['outfile'])
                self.connect(fsrc, head, fsink)
            # fc32->sc16
            elif config['outtype'] == SNAME_TO_ENUM['sc16']:
                cpxfloat_to_ishort = blocks.complex_to_interleaved_short(False)
                head = blocks.head(gr.sizeof_gr_complex*1, config['sample_len'])
                fsink = blocks.file_sink(gr.sizeof_short, config['outfile'])
                self.connect(fsrc, head,cpxfloat_to_ishort,fsink)


#Convert time input to sample number
def time_to_sample(options,args):
    #Open header file
    infile = args[0]
    infile_hdr = infile + '.hdr'
    handle_in = open(infile_hdr, "rb")
    hdr_in, hdr_extra_in, handle_in = read_single_header(handle_in)
    info_in = parse_file_metadata.parse_header(hdr_in,False)
    time_in = info_in["rx_time"]
    sample_start = 0
    #Find data chunk where time is in
    while options.start > time_in:
	hdr_in, hdr_extra_in, handle_in = read_single_header(handle_in)
        info_in = parse_file_metadata.parse_header(hdr_in,False)
        time_in2 = info_in["rx_time"]
	if options.start < time_in2:
	   sample_start = sample_start + (options.start - time_in)*info_in["rx_rate"]
           break
        time_in = time_in2
        sample_start = sample_start + info_in["nitems"]
    #Set start point and sample step to samples
    options.start = sample_start
    options.nsamples = options.nsamples * info_in["rx_rate"]

def truncate_file(options,args):
    #Check if input is in time format
    if options.timeMode:
        time_to_sample(options,args)
    #Check if need to chunk whole file
    if options.repeat_end:
        stop_point = options.start + options.nsamples
        count = 0
	fileName = args[1].split('.')
	while stop_point < options.length:
	    args[1] = fileName[0] + '_' + str(count) + '.' + fileName[1]
	    the_config = propagate_headers(options,args)
	    tb = buildblock(the_config)
	    tb.run()
	    count = count + 1
	    options.start = stop_point
	    stop_point = stop_point + options.nsamples
	options.nsamples = options.length - options.start
	args[1] = fileName[0] + '_' + str(count) + '.' + fileName[1]
    #Propagate and update header. Return flowgraph config.
    the_config = propagate_headers(options,args)
    #Build the flowgraph
    tb = buildblock(the_config)
    #Execute
    tb.run()

