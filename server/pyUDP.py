
# NB this might need to be rewritten to use an ID based system rather than IP address
# if the controllers/carts drop off the network too much


import socket
import time
import threading
from threading import Lock
from threading import Thread


UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
# Listen on port 3000 (to all IP addresses on this system)
listen_addr = ("", 3000)
UDPSock.bind(listen_addr)

controllers = []
carts = []
cart_to_controller = []
allow_reuse_address = True

currentGameEvent = -1
currentGameEventOwner = ""

##############################################################################
class cart:

	addr = ""
	timestamp = 0

	def __getitem__(self, key):
		# if key is of invalid type or value, the list values will raise the error
		return self.values[key]

	def __setitem__(self, key, value):
		self.values[key] = value

	def __init__(self, address, timestamp):
		self.addr = address
		self.timestamp = timestamp
##############################################################################


##############################################################################
class controller:
	addr = ""
	timestamp = 0

	def __getitem__(self, key):
		# if key is of invalid type or value, the list values will raise the error
		return self.values[key]

	def __setitem__(self, key, value):
		self.values[key] = value

	def __init__(self, address, timestamp):
		self.addr = address
		self.timestamp = timestamp
##############################################################################


#initializations

def add_cart( address):
	print "add_cart " + str(address)
	
	c = cart(address, time.time())
	carts.append(c)
	# are there more controllers and carts than joints?
	if(len(carts) > len(cart_to_controller) and len(controllers) > len(cart_to_controller)):
		joint = {'cart':c, 'controller':controllers[len(controllers)-1], 'speed':[127, 127]}
		print "adding joint cart " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		cart_to_controller.append(joint)

def add_controller( address):
	print "add_controller " + str(address)

	cont = controller(address, time.time())
	controllers.append(cont)
	# are there more controllers and carts than joints?
	if(len(controllers) > len(cart_to_controller) and len(carts) > len(cart_to_controller)):
		joint = {'cart':carts[len(carts)-1], 'controller':cont, 'speed':[127, 127]}
		print "adding joint cart " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		cart_to_controller.append(joint)

def remove_controller( address ):
	print "remove controller "
	[cart_to_controller.remove(j) for j in cart_to_controller if j['controller'].addr == address]
	[controllers.remove(c) for c in controllers if c.addr == address]

def remove_cart( address ):
	print "remove cart "
	[cart_to_controller.remove(j) for j in cart_to_controller if j['cart'].addr == address]
	[carts.remove(c) for c in carts if c.addr == address]

def remove_pair( address):
	try:
		for joint in cart_to_controller:
			if(joint['cart'] == address or joint['controller'] == address):
				cart_to_controller.remove(joint)
				return
	except NameError:
		print "no cart_to_controller"


def route_control_signal( address, message):
	for joint in cart_to_controller:
		if(joint['controller'].addr == address):
			l,r = get_speed(message)
			joint['speed'][0] = l
			joint['speed'][1] = r
			# now doing this in the game update loop
			#UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'])
			return

def get_color( address, message):
	print "color " + str(message)	
	for joint in cart_to_controller:
		if(joint['cart'] == address):
			try:
				currentGameEvent = int(message)
				currentGameEventOwner = address
				t = Timer(3.0, clearGameState)
			except ValueError:
				print "Value error parsing color"
				return

def get_speed(message):
	l = int(message.split(':')[1])
	r = int(message.split(':')[2])
	return l,r

#game running updates

def game_update():

	if currentGameEvent == 1:
		slow_down()
	elif currentGameEvent == 2:
		circle()
	elif currentGameEvent == 3:
		skew_right()
	elif currentGameEvent == 4:
		skew_left()
	else:
		run_unmodified()

# game run functions
def run_unmodified():
	for joint in cart_to_controller:
		UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'].addr)

def slow_down():
	for joint in cart_to_controller:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['speed'][0]*0.5, joint['speed'][1]*0.5), joint['cart'].addr)
		else:
			UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'].addr)

def circle():
	for joint in cart_to_controller:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['speed'][0]*-1, joint['speed'][1]*-1), joint['cart'].addr)
		else:
			UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'].addr)

def skew_right():
	for joint in cart_to_controller:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]*0.6), joint['cart'].addr)
		else:
			UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'].addr)

def skew_left():
	for joint in cart_to_controller:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['speed'][0]*0.6, joint['speed'][1]), joint['cart'].addr)
		else:
			UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'].addr)

# pad strings nicely
def stringify( left, right):
	datastring = ""

	modLeft = left
	modRight = right

	if modLeft < 10:
		datastring += "00" + str(modLeft)
	elif left < 100:
		datastring += "0" + str(modLeft)
	else:
		datastring += str(modLeft)

	datastring+=":"

	if right < 10:
		datastring += "00" + str(modRight)
	elif right < 100:
		datastring += "0" + str(modRight)
	else:
		datastring += str(modRight)

	return datastring

def clearGameState():
	currentGameEvent = -1



def keep_alive_cart(addr):
	for c in carts:
		if(c.addr == addr):
			c.timestamp = time.time()
	#prune the list
	for c in carts:
		if(time.time() - c.timestamp > 5.0):
			remove_cart(c.addr)
			remove_pair(c.addr)

def keep_alive_controller(addr):
	for c in controllers:
		if(c.addr == addr):
			c.timestamp = time.time()
	#prune the list
	for c in controllers:
		if(time.time() - c.timestamp > 5.0):
			remove_controller(c.addr)
			remove_pair(c.addr)




##############################################################################
# make a lock
thread_lock = Lock()

#functions to be called in threads
def run_game():
	while True:
		thread_lock.acquire()
		game_update()
		thread_lock.release()
		time.sleep(0.1)

#now run the UDP thread
def run_udp():
	while True:
		thread_lock.acquire()
		data,addr = UDPSock.recvfrom(64)
		#print data.strip()
		datastr = str(data.strip())
		if "register_cart" in datastr:
			add_cart(addr)
		elif "register_control" in datastr:
			add_controller(addr)
		elif "color" in datastr:
			get_color(addr, data)
		elif "speed" in datastr:
			route_control_signal(addr, data)
		elif "disconnect_control" in datastr:
			remove_controller(addr)
		elif "color" in datastr:
			print datastr
		elif "keep_alive" in datastr:
			keep_alive_cart(addr)
		elif "keep_alive_control" in datastr:
			keep_alive_control(addr)
		else:
			print "bad command  " + datastr
		thread_lock.release()

# now start the threads
game_thread = Thread(target = run_game)
udp_thread = Thread(target = run_udp)

game_thread.start()
udp_thread.start()



