# udp_stress_server.py by Eli Fulkerson
# http://www.elifulkerson.com for updates and documentation
# You will also need udp_stress_client.py for this to do anything for you.

# "Push it to the limit!"

# This is an extremely quick-and-dirty UDP testing utility.
# All it does is shove a bunch of UDP traffic through to the server, which
# records and reports the amount of data successfully recieved and the time
# that the transmission took.  It spits out the ratio to give a rough kbps
# estimate.

# The results are very dependent on how much data you push through.  Low amounts
# of data will give you artificially low results.

# "Safety is not guaranteed."

# June 24 2006

from socket import *
import time

# we want to bind on all possible IP addresses
host = "0.0.0.0"

# if you change the port, change it in the client program as well
port = 3000
buffer = 102400

# Create socket and bind to address
UDPSock = socket(AF_INET, SOCK_DGRAM)
UDPSock.bind((host, port))

time.time()
print "\n"
print "-" * 40


# total bytes recieved since last 'reset'
totalbytes = 0

# -1 is a deliberately invalid timestamp
timestamp = -1

start_timestamp = False

# the total number of bursts that have come in
totalrcvs = 0

address = ""

while 1:
    data, addr = UDPSock.recvfrom(buffer)

    address = addr

    data = "X" * 16

    for x in 2048:
		UDPSock.sendto(data,addr)					
		# a pause via time.sleep()
		# not sure that this is needed.  Put it here to play with maybe not-overloading the
		# windows tcp/ip stack, but not sure if it actually has any noticable effect.
		time.sleep(0.001)

UDPSock.close()
