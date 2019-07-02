const net = require('net');
const client = net.createConnection(3000,"192.168.1.1", () => {
                                    //'connect' listener
                                    console.log('connected to server!');
                                    client.write("dle = require 'dle'; return dle.humidity(4, 500, 100);");
                                    });
client.on('data', (data) => {
          console.log(data.toString());
          client.end();
          });
client.on('end', () => {
          console.log('disconnected from server');
          });
