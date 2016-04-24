#!/bin/bash

name=`date "+%Y%m%d_%H%M%S"`.jpeg

#from webcam
streamer -o /Photos/$name

#from android ip camera
#wget http://192.168.0.133:8080/photo.jpg -O /Photos/$name
