/**
 * This code runs on the Raspberry Pi
 */

var webSocketUrl = "wss://api.artik.cloud/v1.1/websocket?ack=true";
var device_id = "<YOUR DEVICE ID>";
var device_token = "<YOUR DEVICE TOKEN>";

var isWebSocketReady = false;
var ws = null;

const raspi = require('raspi');
const I2C = require('raspi-i2c').I2C;

/*
var serialport = require("serialport")
var SerialPort = serialport.SerialPort;
var sp = new SerialPort("/dev/ttyACM0", {
    baudrate: 9600,
    parser: serialport.parsers.readline("\n")
});
*/

var WebSocket = require('ws');
const i2c = new I2C();

/**
 * Gets the current time in millis
 */
function getTimeMillis(){
    return parseInt(Date.now().toString());
}

/**
 * Create a /websocket device channel connection 
 */
function start() {
    //Create the websocket connection
    isWebSocketReady = false;
    ws = new WebSocket(webSocketUrl);
    ws.on('open', function() {
        console.log("Websocket connection is open ....");
        register();
    });
    ws.on('message', function(data, flags) {
        console.log("Received message: " + data + '\n');
    });
    ws.on('close', function() {
        console.log("Websocket connection is closed ....");
    });
}

/**
 * Sends a register message to the websocket and starts the message flooder
 */
function register(){
    console.log("Registering device on the websocket connection");
    try{
        var registerMessage = '{"type":"register", "sdid":"'+device_id+'", "Authorization":"bearer '+device_token+'", "cid":"'+getTimeMillis()+'"}';
        console.log('Sending register message ' + registerMessage + '\n');
        ws.send(registerMessage, {mask: true});
        isWebSocketReady = true;
    }
    catch (e) {
        console.error('Failed to register messages. Error in registering message: ' + e.toString());
    }   
}

/**
 * Send one message to ARTIK Cloud
 */
function sendData(currentTemp){
    try{
        ts = ', "ts": '+getTimeMillis();
        var data = {
            "currentTemp": currentTemp
        };
        var payload = '{"sdid":"'+device_id+'"'+ts+', "data": '+JSON.stringify(data)+', "cid":"'+getTimeMillis()+'"}';
        console.log('Sending payload ' + payload);
        ws.send(payload, {mask: true});
    } catch (e) {
        console.error('Error in sending a message: ' + e.toString());
    }   
}

/**
 * All start here
 */


start(); // create websocket connection

    i2c.readSync(0x18) //Use i2c address from your Cypress board
    if (!isWebSocketReady){
        console.log("Websocket is not ready. Skip sending data to ARTIK Cloud (data:" + data +")");
        return;
     }
     console.log("Received i2c data:" + data);
     var currentTemp = parseFloat(data);

     sendData(currentTemp);
