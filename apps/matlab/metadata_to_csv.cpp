

//Sample usage/command:
// assuming that metadata_to_csv directory is located in gr-analysis/apps run the following commands
// $ cd ~/git-repos/idc/gr-analysis/apps/metadata_to_csv
// $ mkdir build && cd build
// $ cmake ../
// $ make
//command syntax:
// $ ./metadata_to_csv /path_to_input_hdr_file/file.hdr /path_to_input_hdr_file/file.csv
// sample command
// $ ./metadata_to_csv /home/user1/git-repos/idc/gr-analysis/apps/metadata_to_csv/test_file_02273.sc16.hdr /home/user1/git-repos/idc/gr-analysis/apps/metadata_to_csv/test_file_02273_metadata.csv
// 


#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring> // for std::strlen
#include <cstddef> //for std::size_t
#include <string>
#include <csignal>
#include <complex>
#include <vector>
#include <boost/atomic.hpp>
#include <uhd/utils/atomic.hpp>

// need PMT for metadata headers
#include "pmt/pmt.h"

#define METADATA_HEADER_SIZE 149
#define CB_ELEMENT_SIZE 4096
#define EXTRA_LEN 22
#define MAX_SEGMENT_SIZE 1000000
bool checkFirstEntry = true;
using namespace std;


unsigned int FileRead( std::istream & is, std::vector <char> & buff ) {
    is.read( &buff[0], buff.size() );
    return is.gcount();
}

void FileRead( std::ifstream & ifs, string & s ) {
    const unsigned int BUFSIZE = 64 * 1024; // buffer size
    std::vector <char> buffer( BUFSIZE );

    while( unsigned int n = FileRead( ifs, buffer ) ) {
        s.append( &buffer[0], n );
    }
}

int main(int argc, char *argv[])
{
   std::ifstream       file(argv[1]);
   std::ofstream       csvOut (argv[2], ios::app); 
   string buffer;
   FileRead(file, buffer);
   //cout << buffer.substr(0,149) << endl;
   static int numHeaders = buffer.length()/(METADATA_HEADER_SIZE + EXTRA_LEN);
   uint64_t metadata_rowCounter=0;
   int n=0;
   for(int i=0; i<numHeaders; i++){
   pmt::pmt_t hdr = pmt::deserialize_str(buffer.substr(n,n+METADATA_HEADER_SIZE));
         if(pmt::dict_has_key(hdr, pmt::string_to_symbol("strt"))) {
            pmt::pmt_t r = pmt::dict_ref(hdr, pmt::string_to_symbol("strt"), pmt::PMT_NIL);
            uint64_t seg_start = pmt::to_uint64(r);
            uint64_t extra_len = seg_start - METADATA_HEADER_SIZE;
            //std::cout << "extra_len " << extra_len << " \n";
            pmt::pmt_t pversion = pmt::dict_ref(hdr, pmt::string_to_symbol("version"), pmt::PMT_NIL);
               //string version = pmt::symbol_to_string(pversion);
               //std::cout << "version " << version << " \n";
	      std::cout << "version (char) ";
               pmt::print(pversion);
            pmt::pmt_t psize = pmt::dict_ref(hdr, pmt::string_to_symbol("size"), pmt::PMT_NIL);
               long size = pmt::to_long(psize);
               std::cout << "size (bytes per sample int) " << size << " \n";
            pmt::pmt_t ptype = pmt::dict_ref(hdr, pmt::string_to_symbol("type"), pmt::PMT_NIL);
               long type = pmt::to_long(ptype);
               std::cout << "type (data type 1=byte int) " << type << " \n";
            pmt::pmt_t pcplx = pmt::dict_ref(hdr, pmt::string_to_symbol("cplx"), pmt::PMT_NIL);
               bool cplx = pmt::to_bool(pcplx);
               std::cout << "cplx (boolean) " << cplx << " \n";
            pmt::pmt_t prx_time = pmt::dict_ref(hdr, pmt::string_to_symbol("rx_time"), pmt::PMT_NIL);
               uint64_t tsecs = pmt::to_uint64(pmt::tuple_ref(prx_time, 0));
               double tfrac = pmt::to_double(pmt::tuple_ref(prx_time, 1));
               cout << "rx_tsecs (sec) " << tsecs <<endl;               
		cout << "rx_tfrac (sec) " << tfrac <<endl;
               double rx_time = (double)tsecs + tfrac;
               //std::cout << "rx_time " << rx_time << " \n";
            pmt::pmt_t prx_rate = pmt::dict_ref(hdr, pmt::string_to_symbol("rx_rate"), pmt::PMT_NIL);
               double rx_rate = pmt::to_double(prx_rate);
               std::cout << "rx_rate (sample rate double) " << rx_rate << " \n";
            pmt::pmt_t pbytes = pmt::dict_ref(hdr, pmt::string_to_symbol("bytes"), pmt::PMT_NIL);
               uint64_t bytes = pmt::to_uint64(pbytes);
               std::cout << "bytes (size of data segment in bytes uint64_t) " << bytes << " \n";
            pmt::pmt_t pstrt = pmt::dict_ref(hdr, pmt::string_to_symbol("strt"), pmt::PMT_NIL);
               uint64_t strt = pmt::to_uint64(pstrt);
               std::cout << "strt (header segment size uint64_t) " << strt << " \n";
            double rx_freq=0;
            pmt::pmt_t hdr_extra = pmt::deserialize_str(buffer.substr(n+METADATA_HEADER_SIZE,n+METADATA_HEADER_SIZE+extra_len));
               if(pmt::dict_has_key(hdr_extra, pmt::string_to_symbol("rx_freq"))) {
                  pmt::pmt_t prx_freq = pmt::dict_ref(hdr_extra, pmt::string_to_symbol("rx_freq"), pmt::PMT_NIL);
                  rx_freq = pmt::to_double(prx_freq);
                  std::cout << "rx_freq (Hz) " << rx_freq << " \n";
               }
               std::cout << " \n";

            uint64_t numElements = bytes/(uint64_t)size;
            double samplingPeriod = 1/rx_freq;
            if(checkFirstEntry){
               csvOut << tsecs << ","<< tfrac << "," << numElements << "," << rx_freq << "," << rx_rate << "\n";
               checkFirstEntry=false;
            }
            else{
               metadata_rowCounter++;
               rx_time+=samplingPeriod;
               csvOut << tsecs << ","<< tfrac << "," << numElements << "," << rx_freq << "," << rx_rate << "\n";
            }
            n=n+METADATA_HEADER_SIZE + extra_len;
         }
   }
   file.close();
   csvOut.close();
   return 0;
}
