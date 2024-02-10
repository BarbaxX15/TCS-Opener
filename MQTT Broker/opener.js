var admin = require("firebase-admin");
var mqtt=require('mqtt');

var serviceAccount = require("./firebase.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

var mqttbroker = "127.0.0.1:1883";
var username = "";
var password = "";
var tcsSerialNo = "";

var client = mqtt.connect("tcp://"+mqttbroker,{
    clientId:"node_server",
    username:username,
    password:password,
    clean:true});

client.on("connect",function(){	console.log("connected") });

client.on("error",function(error){ console.log("Can't connect"+error) });

client.subscribe("home/bell",{qos:0});

client.on('message',function(topic, message, packet){
    	message = message.toString();
        if(message != null && message.indexOf(tcsSerialNo) >= 0){
            var fcmmessage = {
                data:{ "id": message },
                topic: "doorbell",
                android: {
                    priority: "high",
                }
            };
            admin.messaging().send(fcmmessage).then(res=>{
                console.log("Success",res)
            }).catch(err=>{
                console.log("Error:",err)
            })
	}
    var now = new Date();
    
    console.log(now.toUTCString() +" | message is "+ message);
    console.log("topic is "+ topic);
});
