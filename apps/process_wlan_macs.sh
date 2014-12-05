#!/bin/bash

# Process 802.11 WiFi data and dump macs

GR_PROG=~/git_repos/idc/gr-analysis/wifi_rx.py

if [ $# -lt 3 ]
then
  echo "usage: $0 <wideband_pred_file> <pcap_file> <mac_file>"
  exit
fi
pred_file=$1
if [ ! -f $pred_file ]
then
  echo "wideband file $pred_file doens't exist!"
  exit
fi
pcap_file=$2
mac_file=$3
# Use GNURadio to process packets
echo "START GR 802.11 WLAN Processing..." 
$GR_PROG $1 $2
echo "START tshark processing..."
tshark -r $pcap_file -n -T fields -e wlan.addr > $mac_file
echo "Finished"
