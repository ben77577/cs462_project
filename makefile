main: main.o Client.o Server.o
	g++ -o main main.o Client.o Server.o
	
main.o: main.cpp Client.o Server.o
	g++ -c main.cpp

Client.o: Client.cpp Client.hpp
	g++ -c Client.cpp

Server.o: Server.hpp
	g++ -c Server.cpp

clean: 
	rm *.o



	

	