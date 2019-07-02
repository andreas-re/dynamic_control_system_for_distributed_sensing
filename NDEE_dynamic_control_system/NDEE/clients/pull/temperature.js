const net = require('net');
const client = net.createConnection(3000,"192.168.1.1", () => {
                                    //'connect' listener
                                    console.log('connected to server!');
                                    client.write("bmp180 = require 'bmp180'; return bmp180.temperature(2);");
                                    });
client.on('data', (data) => {
          console.log(data.toString());
          });
client.on('end', () => {
          console.log('disconnected from server');
          client.end();
          });
