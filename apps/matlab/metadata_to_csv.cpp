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
// need PMT for metadata headers
#include "pmt/pmt.h"
#include <boost/algorithm/string.hpp>

#define METADATA_HEADER_SIZE 149
#define CB_ELEMENT_SIZE 4096
#define MAX_SEGMENT_SIZE 1000000
using namespace std;
namespace po = boost::program_options;


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
   // Read and parse program options
   std::string mdfile,csvfile,keys;
   vector<string> key_vect;
   bool verbose_output = false;
   try{
      po::options_description desc("Allowed Options");
      desc.add_options()
         ("help","Produce help message")
         ("mdfile",po::value<std::string>(&mdfile)->required(),"Metadata filename")
         ("csvfile",po::value<std::string>(&csvfile)->required(),"CSV filename")
         ("keys",po::value<std::string>(&keys),
          "Keywords from extras dictionary separated by commas")
         ("verbose","Verbose output")
         ;
      po::positional_options_description p;
      p.add("mdfile",1);
      p.add("csvfile",1);
      po::variables_map vm;
      po::store(po::command_line_parser(argc,argv).options(desc).positional(p).run(),
            vm);
      if( vm.count("help") || argc == 1 ){
         std::cout << "usage: metadata_to_csv <mdfile> <csvfile>" << std::endl;
         std::cout << "Description: Utility to convert GNURadio metadata format" 
            << "to Comma-Separated Values" << std::endl;
         std::cout << desc << "\n";
         return 1;
      }
      po::notify(vm);
      verbose_output = vm.count("verbose") > 0;
      // Parse the extras into a vector
      if( vm.count("keys") > 0 ){
         boost::split(key_vect,keys,boost::is_any_of(","));
      }
   }
   catch(std::exception& e)
   {
      std::cerr << "Error: " << e.what() << "\n";
      return -1;
   }
   std::ifstream file(mdfile.c_str());
   std::ofstream csvOut (csvfile.c_str(), ios::out); 
   string buffer;
   FileRead(file, buffer);
   int numHeaders = 1;
   uint64_t metadata_rowCounter=0;
   int n=0;
   // Main Execution loop over GR headers
   for(int i=0; i<numHeaders; i++){
      std::ostringstream csvline;
      // Pull out data in required header
      pmt::pmt_t hdr = pmt::deserialize_str(buffer.substr(n,n+METADATA_HEADER_SIZE));
      if(pmt::dict_has_key(hdr, pmt::string_to_symbol("strt"))) {
         pmt::pmt_t r = pmt::dict_ref(hdr, pmt::string_to_symbol("strt"), 
               pmt::PMT_NIL);
         uint64_t seg_start = pmt::to_uint64(r);
         uint64_t extra_len = seg_start - METADATA_HEADER_SIZE;
         pmt::pmt_t pversion = pmt::dict_ref(hdr, pmt::string_to_symbol("version"), 
               pmt::PMT_NIL);
         pmt::pmt_t psize = pmt::dict_ref(hdr, pmt::string_to_symbol("size"), pmt::PMT_NIL);
         long size = pmt::to_long(psize);
         pmt::pmt_t ptype = pmt::dict_ref(hdr, pmt::string_to_symbol("type"), pmt::PMT_NIL);
         long type = pmt::to_long(ptype);
         pmt::pmt_t pcplx = pmt::dict_ref(hdr, pmt::string_to_symbol("cplx"), pmt::PMT_NIL);
         bool cplx = pmt::to_bool(pcplx);
         pmt::pmt_t prx_time = pmt::dict_ref(hdr, pmt::string_to_symbol("rx_time"), pmt::PMT_NIL);
         uint64_t tsecs = pmt::to_uint64(pmt::tuple_ref(prx_time, 0));
         double tfrac = pmt::to_double(pmt::tuple_ref(prx_time, 1));
         double rx_time = (double)tsecs + tfrac;
         pmt::pmt_t prx_rate = pmt::dict_ref(hdr, pmt::string_to_symbol("rx_rate"), pmt::PMT_NIL);
         double rx_rate = pmt::to_double(prx_rate);
         pmt::pmt_t pbytes = pmt::dict_ref(hdr, pmt::string_to_symbol("bytes"), pmt::PMT_NIL);
         uint64_t bytes = pmt::to_uint64(pbytes);
         pmt::pmt_t pstrt = pmt::dict_ref(hdr, pmt::string_to_symbol("strt"), pmt::PMT_NIL);
         uint64_t strt = pmt::to_uint64(pstrt);
         uint64_t numElements = bytes/(uint64_t)size;
         // Write the header if this is the first iteration
         if( metadata_rowCounter == 0 ){
               csvline << "rx_rate" << "," <<"rx_tsecs" << "," <<"rx_tfrac" << "," 
                  << "size" << "," << "type" << "," << "cplx" << "," << "strt" 
                  << "," << "bytes"; 
            for(vector<string>::iterator it = key_vect.begin(); it != key_vect.end(); ++it){
               csvline << "," << *it;
            }
            csvline << std::endl;
            // Compute number of headers assuming all contain the same keys
            numHeaders = buffer.length()/(METADATA_HEADER_SIZE + extra_len);
         }
         // Write metadata in csv format
         csvline << rx_rate << "," << tsecs << "," << tfrac << "," << size << "," 
            << type << "," << cplx << "," << strt << "," << bytes;
         pmt::pmt_t hdr_extra = pmt::deserialize_str(buffer.substr(n+METADATA_HEADER_SIZE,
                  n+METADATA_HEADER_SIZE+extra_len));
         // Pull out data in "extras", subject to input key filter
         for(vector<string>::iterator it = key_vect.begin(); it != key_vect.end(); ++it){
            if( pmt::dict_has_key(hdr_extra, pmt::string_to_symbol(*it)) ){
               pmt::pmt_t val = pmt::dict_ref(hdr_extra,pmt::string_to_symbol(*it),
                     pmt::PMT_NIL);
               csvline << "," << val;
            }else{
               std::cout << "Extras Key" << *it << " not found." << std::endl;
               csvline << ",";
            }
         }
         // Finished writing csv 
         csvOut << csvline.str() << std::endl;
         metadata_rowCounter++;
         // Aid the developer
         if( verbose_output ){
            std::cout << "extra_len " << extra_len << " \n";
            std::cout << "version (char) ";
            pmt::print(pversion);
            std::cout << "size (bytes per sample int) " << size << " \n";
            std::cout << "type (data type 1=byte int) " << type << " \n";
            std::cout << "cplx (boolean) " << cplx << " \n";
            std::cout << "rx_tsecs (sec) " << tsecs <<endl;               
            std::cout << "rx_tfrac (sec) " << tfrac <<endl;
            std::cout << "rx_rate (sample rate double) " << rx_rate << " \n";
            std::cout << "bytes (size of data segment in bytes uint64_t) " << bytes << " \n";
            std::cout << "strt (header segment size uint64_t) " << strt << " \n";
         }
         // Calculate buffer index
         n=n+METADATA_HEADER_SIZE + extra_len;
      } // End find header
   } // End main header processing loop
   file.close();
   csvOut.close();
   return 0;
}
