mqtt_client: mqtt_client.o
	gcc -o mqtt_client mqtt_client.o -lmosquitto -lmsgpackc -lpthread

mqtt_client.o: mqtt_client.c
	gcc -c mqtt_client.c

clean:
	rm -f *.o
	rm -f mqtt_client
