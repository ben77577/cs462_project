main: main.o Client.o Server.o Checksum.o
	g++ -o main main.o Panel.o Client.o Server.o Checksum.o
	
main.o: main.cpp Client.o Server.o 
	g++ -c main.cpp

Client.o: Client.cpp Client.hpp Checksum.o
	g++ -c Client.cpp

Server.o: Server.cpp Server.hpp 
	g++ -c Server.cpp
	
Checksum.o: Checksum.cpp Checksum.hpp
	g++ -c Checksum.cpp

Panel.o: Panel.cpp Panel.hpp
	g++ -c Panel.cpp
clean: 
	rm *.o



	

	