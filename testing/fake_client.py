import socket
import threading
from multiprocessing import Queue
import time

UDP_IP = "192.168.42.1"
UDP_PORT = 3000

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.sendto(bytes("register_control"), (UDP_IP, UDP_PORT))
time.sleep(1)

while 1:
    sock.sendto(bytes("speed:2:2"), (UDP_IP, UDP_PORT))
    time.sleep(1)