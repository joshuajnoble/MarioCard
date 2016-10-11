
# NB this might need to be rewritten to use an ID based system rather than IP address
# if the controllers/carts drop off the network too much


import socket


UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
# Listen on port 3000 (to all IP addresses on this system)
listen_addr = ("", 3000)
UDPSock.bind(listen_addr)

controllers = []
carts = []
cartToController = []
allow_reuse_address = True

currentGameEvent = -1
currentGameEventOwner = ""


def addCart( address):
	print "addCart " + str(address)
	carts.append(address)
	if(len(carts) > len(controllers) and len(controllers) > len(cartToController)):
		joint = {'cart':address, 'controller':controllers[len(controllers)-1], 'cachedSpeed':[127, 127]}
		print "adding joint cart " + str(joint['cart']) + " " + str(joint['controller'])
		cartToController.append(joint)

def addController( address):
	print "addController " + str(address)
	controllers.append(address)
	if(len(controllers) > len(cartToController) and len(carts) > len(cartToController)):
		joint = {'cart':carts[len(carts)-1], 'controller':address, 'cachedSpeed':[127, 127]}
		print "adding joint cart " + str(joint['cart']) + " " + str(joint['controller'])
		cartToController.append(joint)

def removeController( address ):
	print "remove controller "
	controllers.remove(address)

def removePair( address):
	try:
		for joint in cartToController:
			if(joint['cart'] == address or joint['cart'] == address):
				cartToController.remove(joint)
				return
	except NameError:
		print "no cartToController"


def routeControlSignal( address, message):
	for joint in cartToController:
		if(joint['controller'] == address):
			l,r = getSpeed(message)
			joint['cachedSpeed'][0] = l
			joint['cachedSpeed'][1] = r
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]), joint['cart'])
			return

def getColor( address, message):
	print "color " + str(message)	
	for joint in cartToController:
		if(joint['cart'] == address):
			try:
				currentGameEvent = int(message)
				currentGameEventOwner = address
				t = Timer(2.0, clearGameState)
			except ValueError:
				print "Value error parsing color"
				return

def getSpeed(message):
	l = int(message.split(':')[1])
	r = int(message.split(':')[2])
	return l,r


def gameUpdate():

	if currentGameEvent == 1:
		slowDown()
	elif currentGameEvent == 2:
		circle()
	elif currentGameEvent == 3:
		skewRight()
	elif currentGameEvent == 4:
		skewLeft()


def slowDown():
	for joint in cartToController:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['cachedSpeed'][0]*0.5, joint['cachedSpeed'][1]*0.5), joint['cart'])
		else:
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]), joint['cart'])

def circle():
	for joint in cartToController:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['cachedSpeed'][0]*-1, joint['cachedSpeed'][1]*-1), joint['cart'])
		else:
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]), joint['cart'])

def skewRight():
	for joint in cartToController:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]*0.6), joint['cart'])
		else:
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]), joint['cart'])

def skewLeft():
	for joint in cartToController:
		if(joint['cart'].client_address != currentGameEventOwner):
			UDPSock.sendto(stringify(joint['cachedSpeed'][0]*0.6, joint['cachedSpeed'][1]), joint['cart'])
		else:
			UDPSock.sendto(stringify(joint['cachedSpeed'][0], joint['cachedSpeed'][1]), joint['cart'])

# don't think I need this for the feather
def stringify( left, right):
	dataString = ""

	modLeft = left
	modRight = right

	if modLeft < 10:
		dataString += "00" + str(modLeft)
	elif left < 100:
		dataString += "0" + str(modLeft)
	else:
		dataString += str(modLeft)

	dataString+=":"

	if right < 10:
		dataString += "00" + str(modRight)
	elif right < 100:
		dataString += "0" + str(modRight)
	else:
		dataString += str(modRight)

	return dataString

def clearGameState():
	currentGameEvent = -1



while True:
	data,addr = UDPSock.recvfrom(64)
	#print data.strip()
	dataStr = str(data.strip())
	if "register_cart" in dataStr:
		addCart(addr)
	elif "register_control" in dataStr:
		addController(addr)
	elif "color" in dataStr:
		getColor(addr, data)
	elif "speed" in dataStr:
		routeControlSignal(addr, data)
	elif "disconnect_control" in dataStr:
		removeController(addr)
	elif "color" in dataStr:
		print dataStr
	elif "keep_alive" in dataStr:
		print "keep alive"
	else:
		print "bad command  " + dataStr
