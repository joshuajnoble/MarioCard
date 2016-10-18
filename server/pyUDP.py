
# NB this might need to be rewritten to use an ID based system rather than IP address
# if the controllers/carts drop off the network too much


import socket
import time
import threading
from threading import RLock
from threading import Thread


UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
# Listen on port 3000 (to all IP addresses on this system)
listen_addr = ("", 3000)
UDPSock.settimeout(0.1)
UDPSock.bind(listen_addr)

controllers = []
carts = []
cart_to_controller = []
allow_reuse_address = True

currentGameEvent = -1
currentGameEventOwner = ""

SKEW_RIGHT_EVENT = 'o'
SKEW_LEFT_EVENT = 'g'
CIRCLE_EVENT = 'm'
SLOW_DOWN_EVENT = 'r'
SPEED_UP_EVENT = 'b'
FLIP_CONTROLS_EVENT = 'y'

possible_events = [SKEW_RIGHT_EVENT, SKEW_LEFT_EVENT, CIRCLE_EVENT, SLOW_DOWN_EVENT, SPEED_UP_EVENT, FLIP_CONTROLS_EVENT]


##############################################################################
class Cart:

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
class Controller:
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

class Game_Event:
	eventType = ""
	owner = ""
	timestamp = 0

	def __getitem__(self, key):
		# if key is of invalid type or value, the list values will raise the error
		return self.values[key]

	def __setitem__(self, key, value):
		self.values[key] = value

	def __init__(self, _owner, _timestamp, _eventType):
		self.owner = _owner
		self.timestamp = _timestamp
		self.eventType = _eventType


events = []


#initializations

def add_cart( address):
	print "add_cart " + str(address)
	
	c = Cart(address, time.time())

	exists = False
	for c in carts:
		if c.addr == address:
			exists = True

	if exists == False:
		carts.append(c)
	else:
		print "cart exists, not adding"

	return

	# are there more controllers and carts than joints?
	if(len(carts) > len(cart_to_controller) and len(controllers) > len(cart_to_controller)):
		joint = {'cart':c, 'controller':controllers[len(controllers)-1], 'speed':[127, 127], 'mod_speed':[127,127]}
		print "adding joint cart " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		cart_to_controller.append(joint)

def add_controller( address):
	print "add_controller " + str(address)

	cont = Controller(address, time.time())
	controllers.append(cont)
	# are there more controllers and carts than joints?
	if(len(controllers) > len(cart_to_controller) and len(carts) > len(cart_to_controller)):
		joint = {'cart':carts[len(carts)-1], 'controller':cont, 'speed':[127, 127], 'mod_speed':[127,127]}
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
	eventColor = message.split(':')[1][0]
	if not eventColor in possible_events:
		print "bad event"
		return

	exists = False

	# do we already have this color?
	for event in events:
		if event.owner == address and event.eventType == message:
			print "event already exists, ignoring"
			exists = True
	
	if exists == False:
		print "making a new game event"
		e = Game_Event(address, time.time(), eventColor)
		events.append(e)

def get_speed(message):
	l = int(message.split(':')[1])
	r = int(message.split(':')[2])
	return l,r

#game running updates

def game_update():

	for joint in cart_to_controller:
		joint['mod_speed'][0] = joint['speed'][0]
		joint['mod_speed'][1] = joint['speed'][1]

	for event in events:
		if event.eventType == FLIP_CONTROLS_EVENT:
			flip_controls(event)
		if event.eventType == SPEED_UP_EVENT:
			speed_up(event)
		if event.eventType == SKEW_RIGHT_EVENT:
			print "skewing right"
			skew_right(event)
		if event.eventType == SKEW_LEFT_EVENT:
			skew_left(event)
		if event.eventType == CIRCLE_EVENT:
			circle(event)
		if event.eventType == SLOW_DOWN_EVENT:
			slow_down(event)

	events[:] = [event for event in events if time.time() - event.timestamp < 5.0]

	for joint in cart_to_controller:
		UDPSock.sendto(stringify(joint['mod_speed'][0], joint['mod_speed'][1]), joint['cart'].addr)

# game run functions

def speed_up(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address == event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*1.5
			joint['mod_speed'][1] = joint['mod_speed'][1]*1.5

def slow_down(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*0.5
			joint['mod_speed'][1] = joint['mod_speed'][1]*0.5

def circle(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address != event.owner):
			joint['mod_speed'][0] = 255
			joint['mod_speed'][1] = 0

def skew_right(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*0.6
			joint['mod_speed'][1] = joint['mod_speed'][1]

def skew_left(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]
			joint['mod_speed'][1] = joint['mod_speed'][1]*0.6


def flip_controls(event):
	for joint in cart_to_controller:
		if(joint['cart'].client_address != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][1]
			joint['mod_speed'][1] = joint['mod_speed'][0]

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
thread_lock = RLock()
run_event = threading.Event()

#functions to be called in threads
def run_game():
	while run_event.is_set():
		print "g + " + str( time.time())
		thread_lock.acquire()
		game_update()
		thread_lock.release()
		print "g - " + str(time.time())
		time.sleep(0.1)

#now run the UDP thread
def run_udp():
	while run_event.is_set():
		print "u + " + str(time.time())
		thread_lock.acquire()
		try:
			data,addr = UDPSock.recvfrom(32)
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
		except socket.timeout:
			thread_lock.release()
		
		print "u - " + str(time.time())
		time.sleep(0.01)

run_event.set()

# now start the threads
game_thread = Thread(target = run_game)
udp_thread = Thread(target = run_udp)

game_thread.start()
udp_thread.start()

try:
	while 1:
		time.sleep(0.1)
except KeyboardInterrupt:
	run_event.clear()
	UDPSock.close()
	game_thread.join()
	udp_thread.join()
	print "threads successfully closed"

