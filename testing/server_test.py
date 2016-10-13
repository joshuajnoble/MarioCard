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
buffer = 64

# Create socket and bind to address
UDPSock = socket(AF_INET, SOCK_DGRAM)
UDPSock.bind((host, port))

time.time()
print "\n"
print "-" * 40
print "udp_stress_server.py"
print "Updates and documentation (if any) at http://www.elifulkerson.com"
print "-" * 40
print "\n"
print "Starting UDP receive server...  control-break to exit."
print "\nWaiting for data..."

# total bytes recieved since last 'reset'
totalbytes = 0

# -1 is a deliberately invalid timestamp
timestamp = -1

start_timestamp = False
donestamp = 0
# the total number of bursts that have come in
totalrcvs = 0
totaldatalen = 0

while 1:
	data, addr = UDPSock.recvfrom(buffer)

	datalen = len(data)

	if data == '2':
		print "done"
		print "total time in secs " + str(time.time() - timestamp)
		print "total bytes " + str(totalbytes)
		print "number of receives " + str(totalrcvs)
		print "data rate in kb " + str(rate)
		print "avg recv size " + str(totaldatalen/totalrcvs)
	elif data == '1':
		print "started "
		totalbytes = 0
		totalrcvs = 0
		timestamp = time.time()
		rate = 0
		totaldatalen = 0	
	else:
		totalbytes += datalen
		totalrcvs += 1
		#print totalrcvs
		donestamp = time.time()
		totaldatalen += datalen

		rate = totalbytes/(donestamp - timestamp) * 8 / 1000

UDPSock.close()
