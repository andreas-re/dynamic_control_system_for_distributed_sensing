server_main: main.o mqtt_handling.o ikvl.o list.o config.o metadata.o compare_complex_metadata.o
	gcc -o server_main main.o mqtt_handling.o ikvl.o list.o config.o metadata.o compare_complex_metadata.o -lpthread -lmsgpackc -lmosquitto

main.o: main.c
	gcc -c main.c

mqtt_handling.o: mqtt_handling.c
	gcc -c mqtt_handling.c

list.o: list.c
	gcc -c list.c

ikvl.o: ikvl.c
	gcc -c ikvl.c

config.o: config.c
	gcc -c config.c

metadata.o: metadata.c
	gcc -c metadata.c

compare_complex_metadata.o: compare_complex_metadata.c
	gcc -c compare_complex_metadata.c

clean:
	rm -f *.o
	rm -f server_main
