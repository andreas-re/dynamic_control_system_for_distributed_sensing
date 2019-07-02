const net = require('net');
const client = net.createConnection(3000,"192.168.1.1", () => {
                                    //'connect' listener
                                    console.log('connected to server!');
                                    client.write("bmp180 = require 'bmp180'; return bmp180.pressure(1);");
                                    });
client.on('data', (data) => {
          console.log(data.toString());
          client.end();
          });
client.on('end', () => {
          console.log('disconnected from server');
          });
