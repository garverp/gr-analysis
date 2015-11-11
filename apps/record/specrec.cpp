// Modified from rx_samples_to_file
// Copyright 2010-2011,2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/types/tune_request.hpp>
#include <uhd/types/time_spec.hpp>
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
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <ctime>
#include <boost/atomic.hpp>
#include <uhd/utils/atomic.hpp>
// need PMT for metadata headers
#include <pmt/pmt.h>
//pthread lib
#include <pthread.h>
//for utc time converting
#include <time.h>
//ptime lib
#include <boost/date_time/posix_time/posix_time.hpp>

//int create_metadata_header(char * header, double samp_rate, double freq, double gain);
std::string create_metadata_header( double samp_rate, double freq, double gain, 
	uhd::time_spec_t timestamp, unsigned long long segment_samps_size, 
	unsigned long long item_num);
#define METADATA_HEADER_SIZE 149
#define CB_ELEMENT_SIZE 4096

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct circbuff_element{
	char a[CB_ELEMENT_SIZE];
} circbuff_element_t;

namespace po = boost::program_options;
boost::atomic<bool> done(false);
uhd::atomic_uint32_t num_elements;
//static bool stop_signal_called = false;
//void sig_int_handler(int){stop_signal_called = true;}
void sig_int_handler(int){ done = true; }

void usrp_write_samples_to_file(int fd, 
		uhd::transport::bounded_buffer<circbuff_element_t>* cb, bool detachhdr){
	int bytes_written = 0;
	circbuff_element_t write_ele;
	while (!done){
		// Block until we have some data to write
		cb->pop_with_wait(write_ele); 
		bytes_written = write(fd,(void*)&write_ele,CB_ELEMENT_SIZE);
		// Using lock to avoid collisions with in file metadata writing.
		if(!detachhdr){pthread_mutex_lock(&mtx);}
		sync_file_range(fd,0,0,SYNC_FILE_RANGE_WRITE);
		if(!detachhdr){pthread_mutex_unlock(&mtx);} 
		if( bytes_written != CB_ELEMENT_SIZE ){
			if( bytes_written < 0 ){
				printf("Problem writing: %s\n",strerror(errno));
			}else{
				printf("Wrote %d/%d bytes\n",bytes_written,CB_ELEMENT_SIZE);
			}
		}
		num_elements.dec();
	}
}

std::string create_metadata_header( double samp_rate, double freq, double gain, 
	uhd::time_spec_t timestamp,unsigned long long segment_samps_size, 
	unsigned long long item_num) {
	// use GNU Radio's PMT methods to construct the header	
	const char METADATA_VERSION = 0x0;
	int headerSize;  
	pmt::pmt_t extras;
	extras = pmt::make_dict();
	extras = pmt::dict_add(extras, pmt::mp("rx_freq"), pmt::mp(freq));
	//item_num specify the order of each segment metadata
        extras = pmt::dict_add(extras, pmt::mp("item_num"), pmt::from_uint64(item_num));
	std::string ext_str = pmt::serialize_str(extras);

	pmt::pmt_t header;
	
	header = pmt::make_dict();
	header = pmt::dict_add(header, pmt::mp("version"), pmt::mp(METADATA_VERSION));
	header = pmt::dict_add(header, pmt::mp("size"), pmt::from_long(4));
	header = pmt::dict_add(header, pmt::mp("type"), pmt::from_long(1));
	header = pmt::dict_add(header, pmt::mp("cplx"), pmt::PMT_T);
	header = pmt::dict_add(header, pmt::mp("rx_time"), 
	pmt::make_tuple(pmt::from_uint64(timestamp.get_full_secs()), 
	pmt::from_double(timestamp.get_frac_secs()) ));
	header = pmt::dict_add(header, pmt::mp("rx_rate"), pmt::mp(samp_rate));
	header = pmt::dict_add(header, pmt::mp("bytes"), pmt::from_uint64(4*segment_samps_size));
	header = pmt::dict_add(header, pmt::mp("strt"), pmt::from_uint64(METADATA_HEADER_SIZE+ext_str.size()));
	std::string hdr_str = pmt::serialize_str(header);
	return hdr_str + ext_str;
}

void write_metadata(int fd, uhd::usrp::multi_usrp::sptr usrp, 
	uhd::time_spec_t timestamp, unsigned long long segment_samps_size, 
	bool detachhdr, unsigned long long item_num) {

	// METADATA - write one header at beginning of file
	char header[METADATA_HEADER_SIZE];
	int headerSize;
	std::string header_str;
	//uhd::time_spec_t timestamp = uhd::time_spec_t::get_system_time(); 
	// can record metadata straight from USRP
	header_str = create_metadata_header( usrp->get_rx_rate(), usrp->get_rx_freq(), 
			usrp->get_rx_gain(), timestamp, segment_samps_size, item_num);
	//std::cout << std::endl << header_str.size() <<std::endl;
	// Using lock to avoid collisions with sample writing.
        if(!detachhdr){	pthread_mutex_lock(&mtx);}
	ssize_t nwritten = write(fd,header_str.c_str(),header_str.size());
        if(!detachhdr){pthread_mutex_unlock(&mtx);}
}

void write_seg_metadata(int fd, uhd::usrp::multi_usrp::sptr usrp, uhd::time_spec_t *g_timestamp,
		unsigned long long * num_total_samps, unsigned long long segment_samps_size, bool detachhdr) {

	int counter = 1;
	unsigned long long item_num = 0;;
	while(!done) {
		if(*num_total_samps >= counter*segment_samps_size){
			//The first sample number in each segment.
			item_num = (counter-1)*segment_samps_size;
            		write_metadata(fd,usrp, *g_timestamp, segment_samps_size, detachhdr, item_num);
			counter++;
			//std::cout << timestamp; 
		} else {
			pthread_cond_wait(&cond, &mtx);
		}
	}

	//write leftover samples metadata which has size less than segment divition size
	if(*num_total_samps > 0) {
		//The first sample number in each segment.
		item_num = (counter-1)*segment_samps_size;
		write_metadata(fd,usrp, *g_timestamp, *num_total_samps%segment_samps_size, detachhdr, item_num);
	}
	
}

void metadata_handle(int fd, int fd_hdr, bool metadata, bool detachhdr, 
		uhd::usrp::multi_usrp::sptr usrp, unsigned long long * num_total_samps, 
		unsigned long long segment_samps_size, uhd::time_spec_t *g_timestamp) {
	if(metadata) {
		if(detachhdr){
			write_seg_metadata(fd_hdr, usrp, g_timestamp, num_total_samps, segment_samps_size, detachhdr);
		} else {
			write_seg_metadata(fd, usrp, g_timestamp, num_total_samps, segment_samps_size, detachhdr);
		}	
	}
}


std::time_t UTC_to_spec_t(const char *buffer){
	struct tm newtime = {0};
	strptime(buffer, "%F %H:%M:%S %z", &newtime);
	//adjust daylight time saving
	newtime.tm_isdst = -1;  
	//utc to time_t	
	std::time_t std_time = mktime(&newtime);
	//Print out starting time in future
	std::cout << "\nRequested Start time: "<< ctime(&std_time) << std::endl;
	
	return std_time;
}

template<typename samp_type> void recv_to_file(
		uhd::usrp::multi_usrp::sptr usrp,
		const std::string &cpu_format,
		const std::string &wire_format,
		const std::string &file,
	        const std::string &start_time,
		size_t cbcapacity,
		unsigned long long num_requested_samples,
		unsigned long long segsize, 
		double time_requested = 0.0,
		bool bw_summary = false,
		bool stats = false,
		bool null = false,
		bool enable_size_map = false,
		bool continue_on_bad_packet = false,
		bool metadata = false,
		bool detachhdr = true
		){
	unsigned long long num_total_samps = 0;
	//create a receive streamer
	uhd::stream_args_t stream_args(cpu_format,wire_format);
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	uhd::rx_metadata_t md;
	uhd::transport::bounded_buffer<circbuff_element_t> buff(cbcapacity); 
	bool circbuff_notfull = true;
	int bytes_written = 0;
	int fd = 0;
	int fd_hdr = 0;
	std::string file_hdr = std::string(file);
	unsigned long long num_samps_to_get = 0;
	// Max number of ticks to give an execution time guarantee
	// = (expected number of seconds for record + guard)*pc_ticks_per_second
	unsigned long long max_ticks = 0;
	// Guard time in case PC tick right is slightly off
	const double guard = 5.0;

	if( (fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 
					S_IRWXU|S_IRWXG|S_IRWXO)) < 0 ){
		std::cerr << "File open failed: " << file.c_str() << std::endl;
	}
	
	// Open detached metadata file
	if(detachhdr && metadata) {
		file_hdr += ".hdr";
		if( (fd_hdr = open(file_hdr.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 
						S_IRWXU|S_IRWXG|S_IRWXO)) < 0 ){
			std::cerr << "File open failed: " << file_hdr.c_str() << std::endl;
		}
	}

	bool overflow_message = true;
	// Set and check circular buffer element and size
	const size_t samps_per_element = CB_ELEMENT_SIZE / sizeof(samp_type);
	if( CB_ELEMENT_SIZE % sizeof(samp_type) != 0 ){
		std::cerr << "Element size " << CB_ELEMENT_SIZE << " not an integer # of samples" 
			<< std::endl;
	}
	printf("Elements are %d bytes, %zu samples/element, %zu elements in circular buffer\n",
		CB_ELEMENT_SIZE,samps_per_element,cbcapacity);
	circbuff_element_t read_ele;
	// Setup streaming
        // STREAM_MODE_NUM_SAMPS_AND_DONE has a limit of 10.7s @ 25MSPS 
	// so num_requested_samples must be < 268435455 to use this mode
	//
	// For --time: calculate num_samps_to_get
	// For --nsamps: num_samps_to_get = num_samples_requested
	//
	// However, we also want a guaranteed execution time in case the USRP drops samples
	// and we don't get num_samps_to_get so we bound the execution time
	// based on the CPU time as a sanity check. 
	// CPU time should still not be required to be correct, just tick at a reasonably
	// close rate to true time for accurate record length
	uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)?
			uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
			uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
			);
	if( num_requested_samples == 0 ){
           if( time_requested != 0){
              num_samps_to_get = time_requested*usrp->get_rx_rate();
	      max_ticks = (long)((time_requested+guard)*(double)boost::posix_time::time_duration::ticks_per_second());
	   }
	   // If nsamps or time arg not given, use ctrl+c to exit
	   // num_samps_to_get = 0
	// non-zero num_requested_samples
        }else{
	   stream_cmd.num_samps = num_requested_samples;
	   num_samps_to_get = num_requested_samples;
	   max_ticks = (long)((guard+num_requested_samples*usrp->get_rx_rate())*
			     (double)boost::posix_time::time_duration::ticks_per_second());
	}
	stream_cmd.stream_now = false;

	std::time_t std_start_time = std::time(NULL);
        // If no future start time is specified on the command line
	if(start_time.compare("0") ==0) {
		//Default starting time is now
		stream_cmd.stream_now = true;
		stream_cmd.time_spec = uhd::time_spec_t();
	} else {
		//Set up future streaming time
		std_start_time = UTC_to_spec_t(start_time.data());
		
		//check if the future time is valid
		if(std_start_time < std::time(NULL)) {
			std::cout << boost::format("Invalid future streaming-time setup") << std::endl;
			done = true;
		} else {
			//time_t to time_spec_t
			uhd::time_spec_t usrp_time = uhd::time_spec_t(std_start_time,0);
			stream_cmd.time_spec = usrp_time;
		}

	}
        // Configure the USRP
	rx_stream->issue_stream_cmd(stream_cmd);
        boost::system_time start =  boost::posix_time::from_time_t(std_start_time);


	boost::posix_time::time_duration ticks_diff;
	boost::system_time last_update = start;
	unsigned long long last_update_samps = 0;
	
	
	typedef std::map<size_t,size_t> SizeMap;
	SizeMap mapSizes;
	boost::thread write_thread(usrp_write_samples_to_file,fd,&buff,detachhdr);

	//When reach future stream time, it will be set to be true
	bool start_stream = false;

	//The counter is used for multiple segment metadata checking.   
	unsigned long long seg_counter = 1;

	// Track when recording started in PC time
	boost::system_time pc_start_time;

	//Gloable variable timestamp for metadata update
	uhd::time_spec_t g_timestamp = uhd::time_spec_t();

	//handle metadata writting
	boost::thread metadata_handle_thread(metadata_handle,fd, fd_hdr, metadata, detachhdr, usrp, &num_total_samps, segsize, &g_timestamp); 
        // Main Loop
	while(not done and (num_total_samps < num_samps_to_get or num_samps_to_get == 0)){

		boost::system_time now = boost::get_system_time();
		size_t num_rx_samps = rx_stream->recv((samp_type*)&read_ele,
				samps_per_element, md,3.0, enable_size_map);

                if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
			std::cout << "." << std::flush;
			continue;
		}else{
		  circbuff_notfull = buff.push_with_haste(read_ele);
		  num_elements.inc();
                  if( start_stream == false ){
                     start_stream = true;
		     pc_start_time = now;
                     time_t sstream_time = md.time_spec.get_full_secs();
                     std::cout << "Start streaming at USRP Time: " << ctime(&sstream_time) << std::endl;
                  }
                }

		if( !circbuff_notfull ){
			std::cerr << "Circular buffer is FULL!" << std::endl;
			done = true;
		}
		
		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
			if (overflow_message){
				overflow_message = false;
				std::cerr << boost::format(
						"Got an overflow indication. Please consider the following:\n"
						"  Your write medium must sustain a rate of %fMB/s.\n"
						"  Dropped samples will not be written to the file.\n"
						"  Please modify this example for your purposes.\n"
						"  This message will not appear again.\n"
						) % (usrp->get_rx_rate()*sizeof(samp_type)/1e6);
			}
			continue;
		}
		if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
			std::string error = str(boost::format("Receiver error: %s") % md.strerror());
			if (continue_on_bad_packet){
				std::cerr << error << std::endl;
				continue;
			}
			else
				throw std::runtime_error(error);
		}

		if (enable_size_map){
			SizeMap::iterator it = mapSizes.find(num_rx_samps);
			if (it == mapSizes.end())
				mapSizes[num_rx_samps] = 0;
			mapSizes[num_rx_samps] += 1;
		}

		num_total_samps += num_rx_samps;
		/**
		if( num_total_samps >= num_samps_to_get ){
			done = true;
		}**/
		if( num_rx_samps != samps_per_element ){
			if( num_total_samps < num_samps_to_get ){
				printf("Only got %zu/%zu samples\n",num_rx_samps,samps_per_element);
			}
		}
		
		if(detachhdr && metadata) {
			//If segment metadata specified, check segment sample size
			if(num_total_samps >= segsize*seg_counter) {
 				g_timestamp = md.time_spec;
				pthread_cond_signal(&cond);
				seg_counter++;
			}
		}
		
		if (bw_summary){
			last_update_samps += num_rx_samps;
			boost::posix_time::time_duration update_diff = now - last_update;
			if (update_diff.ticks() > 
					boost::posix_time::time_duration::ticks_per_second()) {
				double t = (double)update_diff.ticks() / 
					(double)boost::posix_time::time_duration::ticks_per_second();
				double r = (double)last_update_samps / t;
				boost::uint32_t cur_cb_size= num_elements.read();
				printf("%f Msps | %d / %zu elements\n",r/1e6,cur_cb_size,cbcapacity);
				last_update_samps = 0;
				last_update = now;
			}
		}
                ticks_diff = now - pc_start_time;
		if( (unsigned long long)ticks_diff.ticks() > max_ticks && num_samps_to_get != 0){
			std::cerr << "Max recording time exceeded" << std::endl;
			break;
		}
	}
	done = true;
	//wrap up the leftover metadata writing
        pthread_cond_signal(&cond);
	// Wait for threads to exit
	write_thread.join();
	metadata_handle_thread.join(); 
	close(fd);  
	if(detachhdr && metadata){
		close(fd_hdr);
	} 

	if (stats){
		std::cout << std::endl;

		double t = (double)ticks_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
		std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps % t << std::endl;
		double r = (double)num_total_samps / t;
		std::cout << boost::format("%f Msps") % (r/1e6) << std::endl;

		if (enable_size_map) {
			std::cout << std::endl;
			std::cout << "Packet size map (bytes: count)" << std::endl;
			for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
				std::cout << it->first << ":\t" << it->second << std::endl;
		}
	}
}

typedef boost::function<uhd::sensor_value_t (const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names, const char* sensor_name, 
		get_sensor_fn_t get_sensor_fn, double setup_time){
	if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name) == sensor_names.end())
		return false;

	boost::system_time start = boost::get_system_time();
	boost::system_time first_lock_time;

	std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
	std::cout.flush();

	while (true){
		if ((not first_lock_time.is_not_a_date_time()) and
				(boost::get_system_time() > (first_lock_time + boost::posix_time::seconds(setup_time))))
		{
			std::cout << " locked." << std::endl;
			break;
		}

		if (get_sensor_fn(sensor_name).to_bool()){
			if (first_lock_time.is_not_a_date_time())
				first_lock_time = boost::get_system_time();
			std::cout << "+";
			std::cout.flush();
		}else{
			first_lock_time = boost::system_time();	//reset to 'not a date time'

			if (boost::get_system_time() > (start + boost::posix_time::seconds(setup_time))){
				std::cout << std::endl;
				throw std::runtime_error(
						str(boost::format("timed out waiting for consecutive locks on sensor \"%s\"") % sensor_name));
			}

			std::cout << "_";
			std::cout.flush();
		}

		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}

	std::cout << std::endl;

	return true;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
	uhd::set_thread_priority_safe();

	//variables to be set by po
	std::string args, file, type, ant, subdev, ref, wirefmt, start_time;
	size_t total_num_samps, cbcapacity, segsize;
	double rate, freq, gain, bw, total_time, setup_time;
	bool metadata;
	bool detachhdr;
	double gps_pc_deltat_s = 0.0;
	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
		("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
		("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
		("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive (< 268435455)")
		("time", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
		("cbcapacity", po::value<size_t>(&cbcapacity)->default_value(4096), "Elements per Circular Buffer")
		("rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")
		("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz")
		("gain", po::value<double>(&gain), "gain for the RF chain")
		("ant", po::value<std::string>(&ant), "daughterboard antenna selection")
		("subdev", po::value<std::string>(&subdev), "daughterboard subdevice specification")
		("bw", po::value<double>(&bw), "daughterboard IF filter bandwidth in Hz")
		("ref", po::value<std::string>(&ref)->default_value("internal"), "reference source (internal, external, mimo)")
		("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")
		("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
		("progress", "periodically display short-term bandwidth and circbuff capacity")
		("stats", "show average bandwidth on exit")
		("sizemap", "track packet size and display breakdown on exit")
		("null", "run without writing to file")
		("continue", "don't abort on a bad packet")
		("skip-lo", "skip checking LO lock status")
		("int-n", "tune USRP with integer-N tuning")
		("metadata", po::value<bool>(&metadata)->default_value(true),"enable metadata, should write =true")
		("detachhdr", po::value<bool>(&detachhdr)->default_value(true),
		"enable detachhdr, true by default, set false should write = false")
		("segsize", po::value<size_t>(&segsize)->default_value(24999936), 
		"segment size for metadata segmentation. To get accurate timestamp, segment size needs to be multiple of element size")
		("starttime", po::value<std::string>(&start_time)->default_value("0"), "set up start streaming time (YYYY-MM-DD H:M:S)")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
		std::cout << boost::format("specrec %s") % desc << std::endl;
		return ~0;
	}

	bool bw_summary = vm.count("progress") > 0;
	bool stats = vm.count("stats") > 0;
	bool null = vm.count("null") > 0;
	bool enable_size_map = vm.count("sizemap") > 0;
	bool continue_on_bad_packet = vm.count("continue") > 0;

	if (enable_size_map)
		std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;

	// Check number of samples
	if( total_num_samps >= 268435455 ){
		std::cerr << "Error: --nsamps < 268435455" << std::endl;
		return ~0;
	}

	//create a usrp device
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

	//Lock mboard clocks
	usrp->set_clock_source(ref);

	//always select the subdevice first, the channel mapping affects the other settings
	if (vm.count("subdev")) usrp->set_rx_subdev_spec(subdev);

	std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

	//set the sample rate
	if (rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return ~0;
	}
	std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
	usrp->set_rx_rate(rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) 
		<< std::endl << std::endl;

	//set the center frequency
	if (vm.count("freq")){	//with default of 0.0 this will always be true
		std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq/1e6) << std::endl;
		uhd::tune_request_t tune_request(freq);
		if(vm.count("int-n")) tune_request.args = uhd::device_addr_t("mode_n=integer");
		usrp->set_rx_freq(tune_request);
		std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq()/1e6)  
			<< std::endl << std::endl;
	}

	//set the rf gain
	if (vm.count("gain")){
		std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
		usrp->set_rx_gain(gain);
		std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() 
			<< std::endl << std::endl;
	}

	//set the IF filter bandwidth
	if (vm.count("bw")){
		std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % bw << std::endl;
		usrp->set_rx_bandwidth(bw);
		std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % usrp->get_rx_bandwidth() 
			<< std::endl << std::endl;
	}

	//set the antenna
	if (vm.count("ant")) usrp->set_rx_antenna(ant);

	boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time

	//check Ref and LO Lock detect
	if (not vm.count("skip-lo")){
		check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked", 
				boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, 0), setup_time);
		if (ref == "mimo")
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "mimo_locked", 
					boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
		if (ref == "external")
			check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked",
					boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, 0), setup_time);
	}
	
	//Get sensor names from the usrp	
	std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);

	//Set the USRP initial time	
	if(std::find(sensor_names.begin(), sensor_names.end(), "gps_locked") != sensor_names.end()) {              
			uhd::sensor_value_t gps_locked = usrp->get_mboard_sensor("gps_locked",0);
		if( gps_locked.to_bool() ){
			uhd::sensor_value_t gps_time = usrp->get_mboard_sensor("gps_time");
			uhd::time_spec_t usrp_time(gps_time.to_real());	
			usrp->set_time_now(usrp_time);
			time_t stdtime = gps_time.to_real();
			std::cout << "Set USRP with GPS time: "<< ctime(&stdtime) <<std::endl;
		}else{
			std::cout << "Found GPSDO but no GPS lock, setting usrp time to system time" << std::endl;
			time_t pc_time = time(0);       		
			usrp->set_time_now(pc_time);
			std::cout << "Set USRP time with PC system time: "<< ctime(&pc_time) <<std::endl;
		}
	} else {
		//uhd::time_spec_t timestamp = uhd::time_spec_t::get_system_time(); 
		time_t pc_time = time(0);       		
		usrp->set_time_now(pc_time);
		std::cout << "Set USRP time with PC system time: "<< ctime(&pc_time) <<std::endl;
	}

	if (total_num_samps == 0){
		std::signal(SIGINT, &sig_int_handler);
		std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
	}

#define recv_to_file_args(format) \
	(usrp, format, wirefmt, file, start_time, cbcapacity, total_num_samps,segsize, total_time, bw_summary, \
	 stats, null, enable_size_map, continue_on_bad_packet,metadata,detachhdr)
	//recv to file
	if (type == "double") recv_to_file<std::complex<double> >recv_to_file_args("fc64");
	else if (type == "float") recv_to_file<std::complex<float> >recv_to_file_args("fc32");
	else if (type == "short") recv_to_file<std::complex<short> >recv_to_file_args("sc16");
	else throw std::runtime_error("Unknown type " + type);

	//finished
	std::cout << std::endl << "Done!" << std::endl << std::endl;

	return EXIT_SUCCESS;
}
