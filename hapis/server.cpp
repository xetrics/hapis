#include "server.h"
#include "util.h"

void ListenThread(Proxy::Server* server)
{
	while (server->is_alive)
	{
		while (RustNetAPI::NET_Receive(server->pointer))
		{
			uint32_t size = RustNetAPI::NETRCV_LengthBits(server->pointer) / 8;
			unsigned char* data = (unsigned char*)RustNetAPI::NETRCV_RawData(server->pointer);

			printf("[Server] Packet received from client, ID: %d, size: %d\n", data[0], size);

			switch (data[0])
			{
				case NEW_INCOMING_CONNECTION:
				{
					server->incoming_guid = RustNetAPI::NETRCV_GUID(server->pointer);

					/* connect */
					server->client = new Proxy::Client(server->target_ip, server->target_port, server);

					/* wait for connection success packet */
					while (!RustNetAPI::NET_Receive(server->client->pointer)) Sleep(10); 

					/* store the server we are connecting to's identifier */
					server->client->incoming_guid = RustNetAPI::NETRCV_GUID(server->client->pointer);

					/* check if we successfully connected */
					unsigned char client_id = ((unsigned char*)RustNetAPI::NETRCV_RawData(server->client->pointer))[0];
					if (client_id == CONNECTION_REQUEST_ACCEPTED)
					{
						/* start receiving packets from the server */
						server->client->is_connected = true;
						server->client->Start();

						printf("[Client] Connected to game server: %s:%d\n", server->target_ip.c_str(), server->target_port);

						/* raknet should implicitly send a CONNECTION_REQUEST_ACCEPTED packet to the client, we don't have to do anything ? */
					}
					else
					{
						/* tell the client we couldn't connect to the server */
						printf("[Client] Failed to connect to server, closing connection to client...\n");
						server->Close();
					}

					break;
				}
				case ID_DISCONNECTION_NOTIFICATION: /* client told proxy that they're disconnecting */
					server->client->Close(); /* send notification to game server */
					server->Close();
					break;
				case ID_CONNECTION_LOST: /* client lost connection to proxy */
					server->Close();
					server->client->is_connected = false;
					return;
				default:
					server->client->Send(data, size);
			}
		}

		Sleep(PROXY_TICK_MS);
	}
}

Proxy::Server::Server(std::string target_ip, int target_port)
{
	this->pointer = RustNetAPI::NET_Create();
	this->is_alive = false;
	this->target_ip = target_ip;
	this->target_port = target_port;
	this->incoming_guid = 0;

	if (RustNetAPI::NET_StartServer(this->pointer, "127.0.0.1", SERVER_PORT, SERVER_MAX_CONNECTIONS) != 0)
	{
		printf("[Server] ERROR: Unable to start server on port %d\n", SERVER_PORT);
		return;
	}

	printf("[Server] Listening on port %d\n", SERVER_PORT);

	this->is_alive = true;
}

Proxy::Server::~Server()
{
	Close();
}

void Proxy::Server::Start()
{
	thread = util::athread(ListenThread, this);
}

void Proxy::Server::Send(unsigned char* data, uint32_t size)
{
	if (!this->incoming_guid) return;
	RustNetAPI::NETSND_Start(this->pointer);
	RustNetAPI::NETSND_WriteBytes(this->pointer, data, size);
	RustNetAPI::NETSND_Send(this->pointer, this->incoming_guid, SERVER_PACKET_PRIORITY, SERVER_PACKET_RELIABILITY, SERVER_PACKET_CHANNEL);
}

void Proxy::Server::Close()
{
	RustNetAPI::NET_Close(this->pointer);
	this->pointer = 0;
	this->is_alive = false;
	this->incoming_guid = 0;
}
