// ChatClient.cpp : Defines the entry point for the console application.
//
#include <SFML/Graphics.hpp>

#include "stdafx.h"

#include <future>
#include <iostream>
#include <string>

#include <thread>

#include <SFML/Network.hpp>

#include "MessageTypes.h"

using TcpClient     = sf::TcpSocket;
using TcpClientPtr  = std::unique_ptr<TcpClient>;
using TcpClients    = std::vector<TcpClientPtr>;

const sf::IpAddress SERVER_IP("127.0.0.1");
constexpr int SERVER_TCP_PORT(53000);
constexpr int SERVER_UDP_PORT(53001);

void client();
bool connect(TcpClient& _client);
void input(TcpClient& _client);



int main()
{
	//std::thread clientThread(client);

	//sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
	//sf::CircleShape shape(100.f);
	//shape.setFillColor(sf::Color::Green);

	/*while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear();
		window.draw(shape);
		window.display();
	}*/

	client();
    return 0;
}


void client()
{
	TcpClient socket;
	if (!connect(socket))
	{
		return;
	}

	auto handle = std::async(std::launch::async, [&]
	{
		// keep track of the socket status
		sf::Socket::Status status;
	
		do
		{
			sf::Packet packet;
			status = socket.receive(packet);
			if ( status == sf::Socket::Done )
			{
				int header = 0;
				packet >> header;

				NetMsg msg = static_cast<NetMsg>(header);
				if (msg == NetMsg::CHAT)
				{
					std::string str;
					packet >> str;
					std::cout << "< " << str << std::endl;
				}
				else if (msg == NetMsg::PING)
				{
					sf::Packet pong;
					pong << NetMsg::PONG;
					socket.send(pong);
				}
			}
		} while (status != sf::Socket::Disconnected);

	});

	return input(socket);
}

void input(TcpClient &socket)
{
	while (true)
	{
		std::string input;
		std::getline(std::cin, input);

		sf::Packet packet;
		packet << NetMsg::CHAT << input;
		socket.send(packet);
	}
}

bool connect(TcpClient& socket)
{
	auto status = socket.connect(SERVER_IP, SERVER_TCP_PORT);
	if (status != sf::Socket::Done)
	{
		return false;
	}

	std::cout << "Connected to server: " << SERVER_IP << std::endl;
	socket.setBlocking(false);
	return true;
}


