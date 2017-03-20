// ChatServer.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <SFML\Network.hpp>

#include "MessageTypes.h"
#include "Client.h"

constexpr int SERVER_TCP_PORT(53000);
constexpr int SERVER_UDP_PORT(53001);

using TcpClient     = sf::TcpSocket;
using TcpClientPtr  = std::unique_ptr<TcpClient>;
using TcpClients    = std::vector<Client>;

// prototypes
bool bindServerPort(sf::TcpListener&);
void clearStaleCli(TcpClients & tcp_clients);
void connect(sf::TcpListener& tcp_listener, sf::SocketSelector& selector, TcpClients& tcp_clients);
void listen(sf::TcpListener&, sf::SocketSelector&, TcpClients&);
void processChatMsg(sf::Packet &packet, Client & sender, TcpClients & tcp_clients);
void ping(TcpClients& tcp_clients);
void receiveMsg(TcpClients& tcp_clients, sf::SocketSelector& selector);
void runServer();

void ping(TcpClients& tcp_clients)
{
	constexpr auto timeout = 10s;
	for (auto& client : tcp_clients)
	{
		const auto& timestamp = client.getPingTime();
		const auto  now = std::chrono::steady_clock::now();
		auto delta = now - timestamp;
		if (delta > timeout)
		{
			client.ping();
		}
	}
}

void runServer()
{
	sf::TcpListener tcp_listener;
	if (!bindServerPort(tcp_listener))
	{
		return;
	}

	sf::SocketSelector selector;
	selector.add(tcp_listener);

	TcpClients tcp_clients;
	return listen(tcp_listener, selector, tcp_clients);
}

void listen(sf::TcpListener& tcp_listener, sf::SocketSelector& selector, TcpClients& tcp_clients)
{
	while (true)
	{
		const sf::Time timeout = sf::Time(sf::milliseconds(250));
		if (selector.wait(timeout))
		{
			if (selector.isReady(tcp_listener))
			{
				connect(tcp_listener, selector, tcp_clients);
			}
			else
			{
				receiveMsg(tcp_clients, selector);
				clearStaleCli(tcp_clients);
			}
		}
		else
		{
			ping(tcp_clients);
		}
	}
}

void connect(sf::TcpListener& tcp_listener, sf::SocketSelector& selector, TcpClients& tcp_clients)
{
	auto  client_ptr = new sf::TcpSocket;
	auto& client_ref = *client_ptr;
	if (tcp_listener.accept(client_ref) == sf::Socket::Done)
	{
		selector.add(client_ref);

		auto client = Client(client_ptr);
		tcp_clients.push_back(std::move(client));
		std::cout << "client connected" << std::endl;

		std::string welcome_msg;
		std::string client_count = std::to_string(tcp_clients.size());
		welcome_msg  = "Welcome \n";
		welcome_msg += "There are " + client_count + " connected clients";

		sf::Packet packet;
		packet << NetMsg::CHAT << welcome_msg;
		client_ref.send(packet);
	}
}

void receiveMsg(TcpClients& tcp_clients, sf::SocketSelector& selector)
{
	for (auto& sender : tcp_clients)
	{
		auto& sender_socket = sender.getSocket();
		if (selector.isReady(sender_socket))
		{
			sf::Packet packet;
			if (sender_socket.receive(packet) == sf::Socket::Disconnected)
			{
				selector.remove(sender_socket);
				sender_socket.disconnect();
				std::cout << "Client (" << sender.getClientID()
					      << ") Disconnected" << std::endl;
				break;
			}

			int header = 0;
			packet >> header;

			NetMsg msg = static_cast<NetMsg>(header);

			if (msg == NetMsg::CHAT)
			{
				processChatMsg(packet, sender, tcp_clients);
			}
			else if (msg == NetMsg::PONG)
			{
				sender.pong();
			}
		}
	}
}

void clearStaleCli(TcpClients & tcp_clients)
{
	tcp_clients.erase(
		std::remove_if(tcp_clients.begin(), tcp_clients.end(), [](const Client& client)
	{
		return(!client.isConnected());
	}), tcp_clients.end());
}

void processChatMsg(sf::Packet &packet, Client & sender, TcpClients & tcp_clients)
{
	std::string string;
	packet >> string;

	std::cout << "Net Msg: (" << sender.getClientID() << ") "
		      << string << std::endl;

	std::cout << "Latency: " << sender.getLatency().count() 
		      << "us" << std::endl;

	// send the packet to other clients
	for (auto& client : tcp_clients)
	{
		if (sender == client)
			continue;

		client.getSocket().send(packet);
	}
}

bool bindServerPort(sf::TcpListener& listener)
{
	if (listener.listen(SERVER_TCP_PORT) != sf::Socket::Done)
	{
		std::cout << "Could not bind server port, is another app using it?";
		std::cout << std::endl << "Port: " << SERVER_TCP_PORT;
		std::cout << std::endl;
		return false;
	}

	std::cout << "Server launched on port: " << SERVER_TCP_PORT << std::endl;;
	std::cout << "Searching for extraterrestrial life..." << std::endl;
	return true;
}

int main()
{
	runServer();
    return 0;
}
