#!/usr/bin/env python
import socket
import os

if __name__ == '__main__':

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    conn = '/tmp/app_gps_server'
    if os.path.exists(conn):
        os.unlink(conn)
    sock.bind(conn)
    sock.listen(5)
    while True:
        connection,address = sock.accept()
        data = connection.recv(1024)
        print "the client said:\n%s\n" % data
        connection.close()
    sock.close()