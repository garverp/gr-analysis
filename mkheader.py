#!/usr/bin/python

# Create a new header file (and eventually modify existing ones)
# PWG 10.15.2014

from gnuradio import gr,eng_notation
from gnuradio import blocks
from gnuradio.blocks import parse_file_metadata
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import pmt


n2s = eng_notation.num_to_str

def get_options():
   usage="%prog: [options] header_filename"
   parser = OptionParser(option_class=eng_option, usage=usage)
   parser.add_option("-r","--samp-rate",type="eng_float",default=1e6,
                     help="Set sample rate [default=%default]")
   parser.add_option("-v","--verbose",action="store_true",default=False,
                     help="Verbose output [default=%default]")
   parser.add_option("-t","--time-sec",type="long",default=0,
                     help="Whole Second Timestamp [default=%default]")
   parser.add_option("-s","--time-fsec",type="eng_float",default=0.0,
                     help="Fractional Sec Timestamp [default=%default]")
   parser.add_option("-f","--format",type="choice",default="short",
                     choices=['bytes','short','int','long','long long','float',
                              'double',],help="Item data type [default=%default]")
   parser.add_option("-R","--real",action="store_true",dest="real",default=False,
                     help="Data is real, not complex [default=%default]")
   parser.add_option("-l","--length",type="long",default=1e6,
                     help="Length of Data in samples [default=%default]")
                     
   (options,args) = parser.parse_args()
   if len(args) != 1:
      parser.print_help()
      raise SystemExit, 1

   return (options, args[0])


def make_header(options,filename):
   # Open the file and make the header
   hdr_filename = filename + '.hdr'
   hdr_file = open(hdr_filename,'wb')
   header = pmt.make_dict()
   
   
   # Fill in header vals
   # TODO - Read this from blocks.METADATA_VERSION
   ver_val = pmt.from_long(long(0))
   rate_val = pmt.from_double(options.samp_rate)
   time_val = pmt.make_tuple(pmt.from_uint64(options.time_sec),
                             pmt.from_float(options.time_fsec))
   #samp_num = rate_val*time_val
   # Interpret the size
   ft_to_sz = parse_file_metadata.ftype_to_size
   ft_to_str = parse_file_metadata.ftype_to_string
   fmt = [fmt for fmt,value in ft_to_str.items() if value==options.format][0]
   #print ft_to_sz
   #print ft_to_sz[fmt]
   #print type(ft_to_sz[fmt])
   size_val = pmt.from_long(ft_to_sz[fmt])
   #print size_val
   type_val = pmt.from_long(fmt)
   cplx_val = pmt.from_bool(not options.real)
   print cplx_val 
   print type(cplx_val)
   if not options.real:
	ft_to_sz[fmt]=2*ft_to_sz[fmt]
	size_val = pmt.from_long(ft_to_sz[fmt])
   start_val = pmt.from_uint64(blocks.METADATA_HEADER_SIZE + 0)
   samp_len = long(options.length*ft_to_sz[fmt])
   # If complex, two items per sample
   #if not options.real:
      #samp_len = samp_len*2
   #print samp_len
   #print type(samp_len)
   #size_val = pmt.from_long(samp_len/1000000)
   bytes_val = pmt.from_uint64(samp_len)
   #print bytes_val
   bytes_val_int = pmt.to_python(bytes_val)

		

   # Set header vals
   header = pmt.dict_add(header,pmt.intern("version"),ver_val)
   header = pmt.dict_add(header,pmt.intern("size"),size_val)
   #header = pmt.dict_add(header,pmt.intern("sample number"),samp_num)
   header = pmt.dict_add(header,pmt.intern("type"),type_val)
   header = pmt.dict_add(header,pmt.intern("cplx"),cplx_val)
   header = pmt.dict_add(header,pmt.intern("rx_time"),time_val)
   header = pmt.dict_add(header,pmt.intern("rx_rate"),rate_val)
   header = pmt.dict_add(header,pmt.intern("bytes"),bytes_val)
   header = pmt.dict_add(header,pmt.intern("strt"),start_val)
  
   for x in range(0,bytes_val_int-1):
    	if (x%1000000==0):  
   		print header
   		# Serialize and write out file
   		header_str = pmt.serialize_str(header)
		hdr_file.write(header_str)
   
   # Tell user what was written if verbose
   if options.verbose:
     print "Wrote Version " + repr(ver_val) + " header: " + hdr_filename 

if __name__=='__main__':
   (options, filename) = get_options()
   make_header(options,filename)
  
