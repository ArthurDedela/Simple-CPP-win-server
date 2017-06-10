#pragma once

#include <iostream>
#include <WinSock2.h>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#define NON_BLOCKING true
#define BLOCKING false

namespace tcp_server
{
  enum STATUS
  {
    DISABLED,
    WORKING,
    PAUSE,
    SHUTTING_DOWN,
  };

  struct CLIENT
  {
    SOCKET soc;
    bool connected = false;
    char name[50];
  };

  struct CLIENT_MESSAGE
  {
    int client_id;
    char *buffer = nullptr;
    unsigned buffer_size;
  };


  class Server
  {
  private:
    WSADATA WsaData;
    SOCKET soc;
    sockaddr_in local;
    unsigned long max_iteration; //Responsible for the maximum iteration of the clients vector.

    STATUS server_status;

    std::mutex _mutex;

    std::vector<CLIENT> clients;
    std::thread thr_listening;

    std::queue<CLIENT_MESSAGE> clients_messages;
    std::thread thr_msg_handler;

    std::thread thr_error_handler;
    

    Server & operator=(Server & obj) = delete;
    Server(const Server & obj) = delete;

    void accept_clients();
    void msg_handler();
    void error_handler();
    std::queue<unsigned> invalid_clients;
    
    std::queue<int> available_ids;
    
  public:
    Server();

    bool start(unsigned port = 7500);

    STATUS status() { return server_status; }

    std::vector<CLIENT> get_client_list() { return clients; }

    bool send_to_client(unsigned client_id, char * buffer, unsigned buffer_size);

    /*Returns the number of successful sent messages*/
    int send_all(char * buffer, unsigned buffer_size);
    
    //Blocks the calling thread if there are no messages in the queue. Pass NON_BLOCKING as first parameter to prevent blocking, in this case if no messages in queue, field "client_id" of the "CLIENT_MESSAGE" structure will hold -1.
    CLIENT_MESSAGE next_message(bool blocking = BLOCKING);


    void stop();

  };

  
}



