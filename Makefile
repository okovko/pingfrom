##
# Project Title
#
# @file
# @version 0.1

all: server client
server: server.cpp shared.h shared.cpp
	g++ -o server server.cpp shared.cpp -g
client: client.cpp shared.h shared.cpp
	g++ -o client client.cpp shared.cpp -g

clean:
	\rm server client

# end
