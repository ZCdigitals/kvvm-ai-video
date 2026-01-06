#!/bin/bash

/root/video \
    -w 1920 \
    -h 1080 \
    -i /dev/video0 \
    -o /var/run/capture.sock \
    -b 10240 \
    -g 60
