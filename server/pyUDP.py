
# NB this might need to be rewritten to use an ID based system rather than IP address
# if the controllers/carts drop off the network too much


import socket
import time
import threading
from threading import RLock
from threading import Thread


#### TO TRY: remove globals ala http://codereview.stackexchange.com/questions/55680/adding-a-timeout-to-an-udp-socket-server


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
events = []

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


#initializations

def add_cart( address):

	global carts
	global controllers
	global cart_to_controller

	print "add_cart " + str(address)
	
	c = Cart(address, time.time())

	#exists = False
	#for c in carts:
	#	if c.addr == address:
	#		exists = True

	#if exists == False:
	#	carts.append(c)
	#else:
	#	print "cart exists, not adding"
	#return

	# are there more controllers and carts than joints?
	if(len(controllers) > 0):
		print str(len(controllers))
		joint = {'cart':c, 'controller':controllers.pop(), 'speed':[127, 127], 'mod_speed':[127,127]}
		print "adding joint cart " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		cart_to_controller.append(joint)
		print str(len(controllers))
	else:
		carts.append(c)

def add_controller( address):
	
	print "add_controller " + str(address)
	global cart_to_controller
	global cart
	global controller

	cont = Controller(address, time.time())
	# are there more controllers and carts than joints?
	if(len(carts) > 0):
		joint = {'cart':carts.pop(), 'controller':cont, 'speed':[127, 127], 'mod_speed':[127,127]}
		print "adding joint cart " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		cart_to_controller.append(joint)
	else:
		controllers.append(cont)

def clear_joint( cart_addr, control_addr ):
	print "clearing from cart to controller "
	global cart_to_controller
	if cart_addr is not None:
		[controllers.append(j['controller']) for j in cart_to_controller if j['cart'].addr == cart_addr]
		[cart_to_controller.remove(j) for j in cart_to_controller if j['cart'].addr == cart_addr]
	
	if control_addr is not None:
		[carts.append(j['cart']) for j in cart_to_controller if j['controller'].addr == control_addr]
		[cart_to_controller.remove(j) for j in cart_to_controller if j['controller'].addr == control_addr]

def remove_pair( address):
	try:
		for joint in cart_to_controller:
			if(joint['cart'] == address or joint['controller'] == address):
				cart_to_controller.remove(joint)
				return
	except NameError:
		print "no cart_to_controller"


def update_speed( address, message):
	for joint in cart_to_controller:
		if(joint['controller'].addr == address):
			l,r = get_speed(message)
			joint['speed'][0] = l
			joint['speed'][1] = r
			# now doing this in the game update loop
			#UDPSock.sendto(stringify(joint['speed'][0], joint['speed'][1]), joint['cart'])
			return

def send_do_spin(address, message):
	for joint in cart_to_controller:
		if( joint['controller'].addr == address):
			print "sending spin"
			UDPSock.sendto("do_spin", joint['cart'].addr)

def get_color( address, message):
	global events

	print "color " + str(message)
	eventColor = message.split(':')[1][0]
	if not eventColor in possible_events:
		print "bad event"
		return

	validAddress = False
 	for joint in cart_to_controller:
		if(joint['controller'].cart == address):
 			validAddress = True

	if not validAddress:
		print "invalid address"
		return

	exists = False

	# do we already have this color?
	for event in events:
		if event.owner == address and event.eventType == eventColor:
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

def game_update(events, cart_to_controller):

	#print "game update"
	#global events
	#global cart_to_controller

	for joint in cart_to_controller:
		joint['mod_speed'][0] = joint['speed'][0]
		joint['mod_speed'][1] = joint['speed'][1]

	for event in events:
		print " event " + event.eventType
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
		print "sending " + str(stringify(joint['mod_speed'][0], joint['mod_speed'][1])) + " " + str(joint['cart'].addr) + " " + str(joint['controller'].addr)
		UDPSock.sendto(stringify(joint['mod_speed'][0], joint['mod_speed'][1]), joint['cart'].addr)

# game run functions

def speed_up(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr == event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*1.5
			joint['mod_speed'][1] = joint['mod_speed'][1]*1.5

def slow_down(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*0.5
			joint['mod_speed'][1] = joint['mod_speed'][1]*0.5

def circle(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr != event.owner):
			joint['mod_speed'][0] = 255
			joint['mod_speed'][1] = 0

def skew_right(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]*0.6
			joint['mod_speed'][1] = joint['mod_speed'][1]

def skew_left(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]
			joint['mod_speed'][1] = joint['mod_speed'][1]*0.6


def flip_controls(event):
	for joint in cart_to_controller:
		if(joint['cart'].addr != event.owner):
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
	for c in cart_to_controller:
		if(c['cart'].addr == addr):
			c['cart'].timestamp = time.time()
	# #prune the list
	# for c in carts:
	# 	if(time.time() - c.timestamp > 5.0):
	# 		remove_cart(c.addr)
	# 		remove_pair(c.addr)

def keep_alive_controller(addr):
	for c in cart_to_controller:
		if(c['controller'].addr == addr):
			c['controller'].timestamp = time.time()
	# #prune the list
	# for c in controllers:
	# 	if(time.time() - c.timestamp > 5.0):
	# 		remove_controller(c.addr)
	# 		remove_pair(c.addr)


##############################################################################
# make a lock
thread_lock = RLock()
run_event = threading.Event()

#functions to be called in threads
def run_game():
	global events
	global cart_to_controller
	while run_event.is_set():
		#print "g + " + str( time.time())
		thread_lock.acquire()
		game_update(events, cart_to_controller)
		thread_lock.release()
		#print "g - " + str(time.time())
		time.sleep(0.2)

#now run the UDP thread
def run_udp():
	while run_event.is_set():
		#print "u + " + str(time.time())
		thread_lock.acquire()
		try:
			data,addr = UDPSock.recvfrom(32)
			datastr = str(data.strip())
			#print datastr
			if "register_cart" in datastr:
				add_cart(addr)
			elif "register_control" in datastr:
				add_controller(addr)
			elif "color" in datastr:
				get_color(addr, data)
			elif "speed" in datastr:
				update_speed(addr, data)
			elif "disconnect_cart" in datastr:
				clear_joint(addr, None)
			elif "disconnect_control" in datastr:
				clear_joint(None, addr)
			elif "color" in datastr:
				print datastr
			elif "keep_alive" in datastr:
				keep_alive_cart(addr)
			elif "keep_alive_control" in datastr:
				keep_alive_control(addr)
			elif "do_spin" in datastr:
				send_do_spin(addr, datastr)
			else:
				print "bad command  " + datastr

			thread_lock.release()
		except socket.timeout:
			thread_lock.release()
		
		#print "u - " + str(time.time())
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

