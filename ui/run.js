const jsdom = require("jsdom");
const { JSDOM } = jsdom;
//const dom = new JSDOM(`<!doctype html> <html lang="en"> <head> ...  <script> window.foo = "hello world"; </script> </head> </html>`, { runScripts: "dangerously", resources: "usable" });
const window = (new JSDOM(``, { pretendToBeVisual: true })).window;
//const window = dom.window;
//dom.window.onload = () => { console.log(dom.window.foo); };
//var fs = require('fs');
//eval(fs.readFileSync("./paho-mqtt.js")+'');
//eval(fs.readFileSync("mqttws31.js")+'');
//eval(fs.readFileSync("config.js")+'');
//require("./mqttws31.js")();
//require("jquery.min.js");
//require("config.js");
    var mqtt;
    var reconnectTimeout = 2000;

    function MQTTconnect() {
	if (typeof path == "undefined") {
		path = '/mqtt';
	}
	mqtt = new Paho.MQTT.Client(
			host,
			port,
			path,
			"web_" + parseInt(Math.random() * 100, 10)
	);
        var options = {
            timeout: 3,
            useSSL: useTLS,
            cleanSession: cleansession,
            onSuccess: onConnect,
            onFailure: function (message) {
                $('#status').val("Connection failed: " + message.errorMessage + "Retrying");
                setTimeout(MQTTconnect, reconnectTimeout);
            }
        };

        mqtt.onConnectionLost = onConnectionLost;
        mqtt.onMessageArrived = onMessageArrived;

        if (username != null) {
            options.userName = username;
            options.password = password;
        }
        console.log("Host="+ host + ", port=" + port + ", path=" + path + " TLS = " + useTLS + " username=" + username + " password=" + password);
        mqtt.connect(options);
    }

    function onConnect() {
//        $('#status').val('Connected to ' + host + ':' + port + path);
        // Connection succeeded; subscribe to our topic
        mqtt.subscribe(topic, {qos: 0});
//        $('#topic').val(topic);
    }

    function onConnectionLost(response) {
        setTimeout(MQTTconnect, reconnectTimeout);
//       $('#status').val("connection lost: " + responseObject.errorMessage + ". Reconnecting");

    };

    function onMessageArrived(message) {

        var topic = message.destinationName;
        var payload = message.payloadString;

//        $('#ws').prepend('<li>' + topic + ' = ' + payload + '</li>');
    };

MQTTconnect();
