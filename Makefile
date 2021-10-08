all: screen-worms-server screen-worms-client

screen-worms-server: generator.o game-util.o events.o receiver.o game-server.o main-server.o 
	g++ -o screen-worms-server generator.o game-util.o events.o receiver.o game-server.o main-server.o

screen-worms-client: game-util.o client-receiver.o game-client.o main-client.o 
	g++ -o screen-worms-client game-util.o client-receiver.o game-client.o main-client.o	

game-client.o: types.h game-util.h client-receiver.h game-client.h game-client.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c game-client.cpp

main-server.o: receiver.h game-server.h main-server.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c main-server.cpp

main-client.o: main-client.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c main-client.cpp

game-server.o: types.h game-util.h events.h receiver.h game-server.h generator.h game-server.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c game-server.cpp

receiver.o: types.h receiver.h receiver.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c receiver.cpp

client-receiver.o: client-receiver.h client-receiver.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c client-receiver.cpp	

events.o: game-util.h game-server.h events.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c events.cpp

generator.o: generator.h generator.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c generator.cpp

game-util.o: game-util.cpp
	g++ -Wall -Wextra -std=c++17 -O2 -c game-util.cpp

clean:
	rm main-server.o
	rm main-client.o
	rm receiver.o
	rm client-receiver.o
	rm game-server.o
	rm game-client.o
	rm generator.o
	rm events.o
	rm game-util.o
	rm screen-worms-server
	rm screen-worms-client