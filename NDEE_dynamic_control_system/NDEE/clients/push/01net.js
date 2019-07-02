const net = require('net');
const client = net.createConnection(3000,"192.168.1.1", () => {
                                    //'connect' listener
                                    console.log('connected to server!');
                                    //client.write("dle = require 'dle'; return dle.net(10, 100, 1, dle.temperature(1, 100, 100));");
                                    client.write("dle = require 'dle'; local task = {}; task['status'] = 2; task['requestId'] = 7; task['measurand'] = ''; task['device'] = ''; task['timestamp'] = 12345678; task['type'] = 'INTEGER'; task['value'] = 0; return dle.net(2, 100, 100, task);");
                                    });
client.on('data', (data) => {
          console.log(data.toString());
          });
client.on('end', () => {
          client.end();
          console.log('disconnected from server');
          });
