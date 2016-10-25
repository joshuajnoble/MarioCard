
# NB this might need to be rewritten to use an ID based system rather than IP address
# if the controllers/carts drop off the network too much


import socket
import time
import threading
import SocketServer

from threading import RLock


# TCPSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# # Listen on port 3000 (to all IP addresses on this system)
# listen_addr = ("", 3000)
# TCPSock.settimeout(0.1)
# TCPSock.bind(listen_addr)
# TCPSock.listen(3)

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

masterControllerID = 1
masterCartID = 1

thread_lock = RLock()

lastGameUpdate = 0

##############################################################################
class Cart:
	id = 0
	timestamp = 0

	def __getitem__(self, key):
		# if key is of invalid type or value, the list values will raise the error
		return self.values[key]

	def __setitem__(self, key, value):
		self.values[key] = value

	def __init__(self, id, timestamp):
		self.id = id
		self.timestamp = timestamp
##############################################################################


##############################################################################
class Controller:
	id = 0
	timestamp = 0

	def __getitem__(self, key):
		# if key is of invalid type or value, the list values will raise the error
		return self.values[key]

	def __setitem__(self, key, value):
		self.values[key] = value

	def __init__(self, id, timestamp):
		self.id = id
		self.timestamp = timestamp

##############################################################################


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

##############################################################################


##############################################################################


class MarioCardThreadedServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

##############################################################################


##############################################################################

class MarioCardReqHandler(SocketServer.BaseRequestHandler):

	def handle(self):

		global thread_lock

		thread_lock.acquire()

		# Echo the back to the client
		data = self.request.recv(16)
		#this needs to change
		datastr = str(data.strip())
		if "register_cart" in datastr:
			self.add_cart()
		elif "register_control" in datastr:
			self.add_controller()
		elif "color" in datastr:
			self.get_color(datastr)
		elif "speed" in datastr:
			self.route_control_signal(datastr)
		elif "disconnect_control" in datastr:
			self.remove_controller(datastr)
		elif "color" in datastr:
			print datastr
			self.get_color(datastr)
		elif "keep_alive" in datastr:
			self.keep_alive_cart(datastr)
		elif "keep_alive_control" in datastr:
			self.keep_alive_control(datastr)
		elif "update" in datastr:
			self.update(datastr)
		else:
			print "bad command  " + datastr

		thread_lock.release()

		return

	def add_cart( self ):
		print "add_controller "

		global masterCartID
		
		if( len(carts) < 3):
			self.request.send(str(masterCartID))
			c = Cart(masterCartID, time.time())
			masterCartID += 1
			carts.append(c)
			# are there more controllers and carts than joints?
			if(len(carts) > len(cart_to_controller) and len(controllers) > len(cart_to_controller)):
				joint = {'cart':c, 'controller':controllers[len(controllers)-1], 'speed':[127, 127], 'mod_speed':[127,127]}
				print "adding joint cart " + str(c.id) + " " + str(controllers[len(controllers)-1].id)
				cart_to_controller.append(joint)
		else:
			self.request.send(" too many carts already ")
			#conn.close()
		return


	def add_controller( self ):
		print "add_controller "

		global masterControllerID

		if( len(controllers) < 3):
			self.request.send(str(masterControllerID))
			c = Controller(masterControllerID, time.time())
			masterControllerID += 1
			controllers.append(c)
			# are there more controllers and carts than joints?
			if(len(controllers) > len(cart_to_controller) and len(carts) > len(cart_to_controller)):
				joint = {'cart':carts[len(carts)-1], 'controller':cont, 'speed':[127, 127], 'mod_speed':[127,127]}
				print "adding joint cart " + str(c.id) + " " + str(controllers[len(controllers)-1].id)
				cart_to_controller.append(joint)
		else:
			self.request.send(" too many controllers already ")

	def remove_controller( self, message ):
		print "remove controller "
		id = getId(message)
		[cart_to_controller.remove(j) for j in cart_to_controller if j['controller'].id == id]
		[controllers.remove(c) for c in controllers if c.id == id]

	def remove_cart( self, message ):
		print "remove cart "
		id = getId(message)
		[cart_to_controller.remove(j) for j in cart_to_controller if j['cart'].id == id]
		[carts.remove(c) for c in carts if c.id == id]


	def route_control_signal( self, message):
		id = getId(message) # 1st char
		for joint in cart_to_controller:
			if(joint['controller'].id == id):
				l,r = get_speed(message)
				joint['speed'][0] = l
				joint['speed'][1] = r
				return

	def get_color(self, message):
		# message should be color:2:o e.g. what is it, what id, what color
		id = getId(message) # 1st char
		eventColor = message.split(':')[2][0] # 1st char
		if not eventColor in possible_events:
			print "bad event"
			return

		exists = False

		# do we already have this color?
		for event in events:
			if event.owner == id and event.eventType == eventColor:
				print "event already exists, ignoring"
				exists = True
		
		if exists == False:
			print "making a new game event"
			e = Game_Event(id, time.time(), eventColor)
			events.append(e)

	def update(self, data):
		id = getId(data)
		#print " cart ID is " + str(id) + " and there are " + str(len(cart_to_controller)) + " pairs to look through "
		for joint in cart_to_controller:
			if(joint['cart'].id == id):
				print "sending " + str(stringify(joint['mod_speed'][0], joint['mod_speed'][1]))
				self.request.send(stringify(joint['mod_speed'][0], joint['mod_speed'][1]))





##############################################################################


##############################################################################

def get_speed(message):
	l = int(message.split(':')[1])
	r = int(message.split(':')[2])
	return l,r

#game running updates

def game_update():

	#print "game update"

	global events

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
			skew_right(event)
		if event.eventType == SKEW_LEFT_EVENT:
			skew_left(event)
		if event.eventType == CIRCLE_EVENT:
			circle(event)
		if event.eventType == SLOW_DOWN_EVENT:
			slow_down(event)

	events[:] = [event for event in events if time.time() - event.timestamp < 5.0]

# game run functions

def speed_up(event):
	for joint in cart_to_controller:
		if(joint['cart'].id == event.owner):
			joint['mod_speed'][0] = round(joint['mod_speed'][0]*1.5)
			joint['mod_speed'][1] = round(joint['mod_speed'][1]*1.5)

def slow_down(event):
	for joint in cart_to_controller:
		if(joint['cart'].id != event.owner):
			joint['mod_speed'][0] = round(joint['mod_speed'][0]*0.5)
			joint['mod_speed'][1] = round(joint['mod_speed'][1]*0.5)

def circle(event):
	for joint in cart_to_controller:
		if(joint['cart'].id != event.owner):
			joint['mod_speed'][0] = 255
			joint['mod_speed'][1] = 0

def skew_right(event):
	for joint in cart_to_controller:
		if(joint['cart'].id != event.owner):
			joint['mod_speed'][0] = round(joint['mod_speed'][0]*0.6)
			joint['mod_speed'][1] = joint['mod_speed'][1]

def skew_left(event):
	for joint in cart_to_controller:
		if(joint['cart'].id != event.owner):
			joint['mod_speed'][0] = joint['mod_speed'][0]
			joint['mod_speed'][1] = round(joint['mod_speed'][1]*0.6)


def flip_controls(event):
	for joint in cart_to_controller:
		if(joint['cart'].id != event.owner):
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

	datastring += ';'

	return datastring


def getId( string ):
	return int(string.split(':')[1])

try:

	listen_addr = ("", 3000)
	server = MarioCardThreadedServer(listen_addr, MarioCardReqHandler)

	t = threading.Thread(target=server.serve_forever)
	t.setDaemon(True) # don't hang on exit
	t.start()

	while 1:

		if time.time() - lastGameUpdate > 0.1:
			thread_lock.acquire()
			game_update()
			lastGameUpdate = time.time()
			thread_lock.release()

except KeyboardInterrupt:
	TCPSock.close()

