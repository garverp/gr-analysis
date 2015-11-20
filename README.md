# gr-analysis
http://www.trondeau.com/grcon15-presentations#wednesday_Lincoln_Synchronized
## Features
Tools for High Sample Rate Recording and Post-Processing in GNURadio
* metadata_to_csv: Convert metadata to comma-separated value (.csv) format for import into MATLAB, Octave, etc.
* specrec: High sample-rate recording (~30 MSPS) program with metadata. Linux-only as of now.
* gr_fileman: File manipulation program. Can convert between data formats, cut out a subsection of data, interpolates timestamps, etc. Somewhat like SoX but exploits metadata and timestamps
* gr_mkheader: Generate headers for data which previously had none
* gr_fstat: Compute statistics of .sc16 and .fc32 files (mean,var,max,min)
* read_file_metadata: Display metadata headers in human-readable format

## Installation
These tools are primarily Linux-based. All tools but specrec should theoretically work, but untested on OSX/Windows.
<pre>git clone https://github.com/garverp/gr-analysis
cd gr-analysis
mkdir build && cd build
cmake ../
make
sudo make install
sudo ldconfig</pre>
