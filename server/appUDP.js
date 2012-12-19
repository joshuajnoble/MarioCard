
/**
 * Module dependencies.
 */

var express = require('express')
  , routes = require('./routes')
  , http = require('http')
  , sio = require('socket.io')
  , dgram = require('dgram')
  , WebSocketServer = require('websocket').server;

var app = express();

app.configure(function(){
  app.set('port', process.env.PORT || 3000);
  app.set('views', __dirname + '/views');
  app.set('view engine', 'ejs');
  app.use(express.favicon());
  app.use(express.logger('dev'));
  app.use(express.bodyParser());
  app.use(express.methodOverride());
  app.use(express.cookieParser('your secret here'));
  app.use(express.session());
  app.use(app.router);
  app.use(require('less-middleware')({ src: __dirname + '/public' }));
  app.use(express.static(__dirname + '/public'));

  // just make sure that we're not updating too fast
  setInterval(function() {
    updateGame(); // try to only update the game 10xSec
    if(Math.abs(time - Date.now()) > 100) {
      time = Date.now();
    }
  }, 100);

});

app.configure('development', function(){
  app.use(express.errorHandler());
});

var wiflyCarts = [];
var controllers = [];
var controllerCartJoint = [];
var lastIndex = 0;
var currentGameState = {}; // what modifier it is, and who did it
var time = Date.now();

// udp socket
var udpSocket = dgram.createSocket('udp4');

udpSocket.on("listening", function () {
  var address = udpSocket.address();
  console.log("UDP server listening " + address.address + ":" + address.port);
});

udpSocket.on("message", function(msg, rinfo) {
  //console.log(" Got a message " );
  //console.log('client got: ' + msg + ' from ' + rinfo.address + ':' + rinfo.port);
  //console.log(msg.toString('ascii'));
  var messageStr = msg.toString();
  var idInd = messageStr.indexOf("id=");
  var colorInd = messageStr.indexOf("color=");
  var id, color;
  if(idInd != -1 && colorInd != -1) {
    id = messageStr.slice(idInd, idInd+1);
    color = messageStr.slice(colorInd, idInd+1);
  }

  if(currentGameState) {
    currentGameState['event'] = color;
    currentGameState['source'] = id;
  }

  setTimeout( function() {
      currentGameState = null;
  }, 2000); // 2 seconds of madness?

});

udpSocket.bind(3001, "0.0.0.0");

app.get('/', routes.index);

function updateGame() {

  var buf = Buffer("test");
  wiflyCarts.forEach(function( obj, index, array) {
    console.log(obj.ip);
    udpSocket.send(buf, 0, buf.length, 3002, obj.ip);
  });

  if(!currentGameState)
    return;

  switch(currentGameState['event']) {
  	case 0:
  // nada, normal game play
  	break;
  	case 1:
  // if we're not the one who sent it, then it affects us
  	if(currentGameState['source'] != query.id) {

  	    carts[wiflyCarts[query.id]].left = 10; // slow down, for example
  	    carts[wiflyCarts[query.id]].right = 10; // slow down, for example
  	}
  	break;

  	case 2:
    if(currentGameState['source'] == query.id) {

        // how to do faster
    }
  	break;
  	case 3:
    // make everybody else do a loop
    if(currentGameState['source'] != query.id) {

        carts[wiflyCarts[query.id]].left = 255; // slow down, for example
        carts[wiflyCarts[query.id]].right = -255; // slow down, for example
    }
  	break;
  	case 4:
    // make everybody else skew left
    if(currentGameState['source'] != query.id) {
      //
    }
  	break;
  	case 5:
    // make everybody else skew right?
    if(currentGameState['source'] != query.id) {
      //
    }
  	break;
  	case 6:
    // one more thing?
    if(currentGameState['source'] != query.id) {
      //
    }
  	break;
    default:
    break;
  }
}

function stringify( l, r ){
  var left = "", right = "";
  if(l < 10) {
      left = "00" + l;
    } else if(l < 100) {
      left = "0" + l;
    } else {
      left = l;
    }

    if(r < 10) {
      right = "00" + r;
    } else if(r < 100) {
      right = "0" + r;
    } else {
      right = r;
    }
    return left + ":" + right;
}

function convertToRange(value, srcRange, dstRange){
  // value is outside source range return
  if (value < srcRange[0] || value > srcRange[1]){
    return 122; 
  }

  var srcMax = srcRange[1] - srcRange[0],
      dstMax = dstRange[1] - dstRange[0],
      adjValue = value - srcRange[0];

  return Math.round((adjValue * dstMax / srcMax) + dstRange[0]);

}

function pairControllerToCart () {
  if(controllerCartJoint.length < controllers.length && controllerCartJoint.length < wiflyCarts.length) {
    controllerCartJoint.push( {controller:controllers[controllers.length-1], cart:wiflyCarts[wiflyCarts.length-1]} );
  }
}

app.on('connection', function(sock) {
  console.log('Client connected from ' + sock.remoteAddress);
  // Client address at time of connection ----^
});

app.get('/id', function(req,res) {

  console.log(req.connection.remoteAddress);
  //var endpoint = io.manager.handshaken[socket.id].address;

   if(!wiflyCarts[lastIndex]) {
      var newCart = {id:lastIndex, ip:req.connection.remoteAddress};
      wiflyCarts.push(newCart);
   }

   res.writeHead(200, {"id":lastIndex.toString()}, {"Content-Type":"text/plain"});
   res.write(lastIndex.toString());
   res.end();

   lastIndex++;

});

// create server
var server = http.createServer(app).listen(app.get('port'), function(){
  console.log("Express server listening on port " + app.get('port'));
});

// SOCKET CODE
var io = sio.listen(server);

// now we send messages to everyone
io.sockets.on('connection', function (socket) {
  console.log(socket.id);

  var controller = {id:socket.id, left: 122, right: 122 };
  controllers.push(controller);

  /*if(wiflyCarts.length == 0) {
      var newCart = {id:lastIndex};
      wiflyCarts.push(newCart);
    }*/

  console.log(" controller size "+controllers.length);
  pairControllerToCart();
  console.log(" controller size "+controllers.length);

  socket.on('left', function (data) {

    controller.left = data;
    console.log(controllers[socket.id]);

    var msgStr = "";
    controllerCartJoint.forEach( function( obj, index, arr) {
      msgStr += obj.cart.id + "=" + stringify( convertToRange(obj.controller.left, [-100, 100], [0, 255]), convertToRange(obj.controller.right, [-100, 100], [0, 255])) + ";";
    });
    msgStr += "x";
    var buf = Buffer(msgStr);
    udpSocket.send(buf, 0, buf.length, 3001, "0.0.0.0");
    time = Date.now();

  });

  // now we send messages to everyone
  socket.on('right', function (data) {
    controller.right = data;
    console.log(controllers[socket.id]);

    var msgStr = "";
    controllerCartJoint.forEach( function( obj, index, arr) {
      msgStr += obj.cart.id + "=" + stringify( convertToRange(obj.controller.left, [-100, 100], [0, 255]), convertToRange(obj.controller.right, [-100, 100], [0, 255])) + ";";
    });
    msgStr += "x";
    var buf = Buffer(msgStr);
    udpSocket.send(buf, 0, buf.length, 3001, "0.0.0.0");
    time = Date.now();
  });

  socket.on('disconnect', function () {
    console.log('disconnect = ' + socket.id);
    var controllerId = controllers.indexOf(controller);
    delete controllers[controllerId];

  });

});
