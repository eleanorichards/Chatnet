#pragma once
#include <chrono>
#include <memory>
#include <SFML/Network.hpp>

using namespace std::chrono_literals;
class Client
{
public:
	Client() = default;
	Client(sf::TcpSocket*);
	Client(Client&&);
	
	sf::TcpSocket& getSocket();
	bool isConnected() const;
	void setLatency(std::chrono::microseconds);
	void ping();
	void pong();

	const auto& getPingTime() const { return timestamp;}
	const auto& getLatency()  const { return latency; }
	int getClientID()         const { return id; }
	
	Client& operator=(Client&&);
	bool operator==(const Client& rhs) { return id == rhs.id; }
	
private:
	static unsigned int next_id;

	std::unique_ptr<sf::TcpSocket> socket = nullptr;
	std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();
	std::chrono::microseconds latency = 100us;
	int id = next_id++;
};