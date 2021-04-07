main: main.o Client.o Server.o Checksum.o Panel.o ErrorCreate.o
	g++ -o main main.o Panel.o ErrorCreate.o Client.o Server.o Checksum.o -pthread
	
main.o: main.cpp Client.hpp Server.hpp ErrorCreate.hpp
	g++ -c main.cpp

Client.o: Client.cpp Client.hpp Checksum.hpp ErrorCreate.hpp
	g++ -c Client.cpp

Server.o: Server.cpp Server.hpp ErrorCreate.hpp
	g++ -c Server.cpp
	
Checksum.o: Checksum.cpp Checksum.hpp
	g++ -c Checksum.cpp

Panel.o: Panel.cpp Panel.hpp
	g++ -c Panel.cpp
	
ErrorCreate.o: ErrorCreate.cpp ErrorCreate.hpp
	g++ -c ErrorCreate.cpp
	
clean: 
	rm *.o



	

	
