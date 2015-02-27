import sys


import pmt
from analysis import parse_file_metadata_local
from gnuradio import eng_notation


def rfm(filename, detached=False):
    handle = open(filename, "rb")
    times = 0
    nheaders = 0
    nread = 0
    totaltime = 0
    while(True):
        # read out next header bytes
        hdr_start = handle.tell()
        header_str = handle.read(parse_file_metadata_local.HEADER_LENGTH)
        if(len(header_str) == 0):
            break

        # Convert from string to PMT (should be a dictionary)
        try:
            header = pmt.deserialize_str(header_str)
        except RuntimeError:
            sys.stderr.write("Could not deserialize header: invalid or corrupt data file.\n")
            sys.exit(1)

        print "HEADER {0}".format(nheaders)
        info = parse_file_metadata_local.parse_header(header, True)
        deltat = info["nitems"]/info["rx_rate"]
        totaltime = totaltime + deltat
        #print "Time in this chunk: {0:.6f}".format(deltat)
        print "Chunk Time: " + eng_notation.num_to_str(deltat) + "S"
        print "Total Time elapsed: " + eng_notation.num_to_str(totaltime) + "S"

        if(info["extra_len"] > 0):
            extra_str = handle.read(info["extra_len"])
            if(len(extra_str) == 0):
                break

            try:
                extra = pmt.deserialize_str(extra_str)
            except RuntimeError:
                sys.stderr.write("Could not deserialize extras: invalid or corrupt data file.\n")
                sys.exit(1)

            print "\nExtra Header:"
            extra_info = parse_file_metadata_local.parse_extra_dict(extra, info, True)

        nheaders += 1
        nread += parse_file_metadata_local.HEADER_LENGTH + info["extra_len"]
        if(not detached):
            nread += info['nbytes']
        handle.seek(nread, 0)
        print "\n\n"
    
    