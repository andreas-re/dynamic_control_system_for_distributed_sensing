const net = require('net');
const client = net.createConnection(3000,"192.168.1.1", () => {
                                    //'connect' listener
                                    console.log('connected to server!');
                                    //client.write("dle = require 'dle'; return dle.net(10, 100, 1, dle.temperature(1, 100, 100));");
                                    client.write("dle = require 'dle'; return dle.stop(1, 1)");
                                    });
client.on('data', (data) => {
          console.log(data.toString());
          });
client.on('end', () => {
          client.end();
          console.log('disconnected from server');
          });
