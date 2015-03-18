#/bin/bash

build/specrec --args=master_clock_rate=20e6 --rate=25e6 --time=30 --file=./ch6_wifi.sc16 --freq=90.1e6 --gain=45 --ant=RX2 --progress --stats --metadata=true
