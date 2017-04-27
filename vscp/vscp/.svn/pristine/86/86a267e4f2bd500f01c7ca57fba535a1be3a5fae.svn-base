#!/usr/bin/env python
# www.jbxue.com
import socket
import time
import json

if __name__ == '__main__':
    gps_list = {"sta_mac":"00d0.0000.0001","gps_data":"$GPGLL,2236.91284,N,11403.24705,E,060826.00,A,D*66"}
                
    gps_string = json.dumps(gps_list)
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    conn = '/tmp/app_gps_server'
    sock.connect(conn)
#    time.sleep(1)
    sock.send(gps_string)
    sock.close()