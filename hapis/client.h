#pragma once
#include <string>

#include "rrapi.h"
#include "server.h"
#include "util.h"

#define CLIENT_MAX_RETRIES 32
#define CLIENT_RETRY_DELAY 100
#define CLIENT_TIMEOUT 100

namespace Proxy {
	class Server; // forward declaration

	class Client
	{
	private:
		util::athread thread;
	public:
		Client(std::string target_ip, int target_port, Proxy::Server* server);
		void Start();
		
		RustNetAPI::RakPeer RakNetClient;
		Proxy::Server* ProxyServer;

		std::string target_ip;
		int target_port;
		bool connected;
		uint64_t incoming_guid; /* the identifier for the server we are connected to (rust server) */
	};
}