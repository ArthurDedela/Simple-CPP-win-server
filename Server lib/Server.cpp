#include "Server.h"

#define BUFSIZE 4096


namespace tcp_server
{
  
  Server::Server()
  {
    max_iteration = 0;
    clients.resize(10000);
    server_status = DISABLED;
  }

  void Server::accept_clients()
  {
    while (server_status == WORKING)
    {
      CLIENT client;

      client.soc = accept(soc, NULL, NULL);
      if (client.soc == INVALID_SOCKET) continue;

      client.connected = true;
        
      if (!available_ids.empty())
      {
        clients[available_ids.front()] = client;
        available_ids.pop();
      }
      else
      {
        clients[max_iteration++] = client;
        if (max_iteration == clients.size())
        {
          clients.resize(max_iteration + 10000);
        }
      }
    }
  }

  void Server::msg_handler()
  {
    fd_set read_s;

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

    while (server_status == WORKING)
    {
      for (int i = 0; i < max_iteration; i++)
      {
        if (!clients[i].connected) continue;

        FD_ZERO(&read_s);
        FD_SET(clients[i].soc, &read_s);

        int res = select(0, &read_s, NULL, NULL, &timeout);
        
        if (res == SOCKET_ERROR)
        {
          clients[i].connected = false;
          invalid_clients.push(i);
          continue;
        }

        if (res)
        {
          char buffer[BUFSIZE];
          int len = recv(clients[i].soc, buffer, BUFSIZE, 0);

          if (len == SOCKET_ERROR)
          {
            clients[i].connected = false;
            invalid_clients.push(i);
            continue;
          }

          CLIENT_MESSAGE msg;
          msg.client_id = i;
          msg.buffer = new char[len];
          msg.buffer_size = len;
                    
          memcpy(msg.buffer, buffer, len);

          clients_messages.push(msg);
        }        
      }
    }
  }


  void Server::error_handler()
  {
    while (server_status == WORKING)
    {
      if (!invalid_clients.empty())
      {
        unsigned id = invalid_clients.front();
        //if (id == (max_iteration - 1))
        //{
        //  for (int i = max_iteration - 1; i >= 0; i--)
        //  {
        //    if (clients[i].connected == false) max_iteration--;
        //    else break;
        //  }
        //}

        invalid_clients.pop();
        closesocket(clients[id].soc);
        available_ids.push(id);
      }
    }
  }


  bool Server::start(unsigned port)
  {
    WSADATA WsaData;
    WSAStartup(0x0101, &WsaData);

    soc = socket(AF_INET, SOCK_STREAM, 0);

    if (soc == INVALID_SOCKET) return false;

    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(soc, (sockaddr *)&local, sizeof(local)) == SOCKET_ERROR) return false;

    if (listen(soc, 5) == INVALID_SOCKET) return false;

    server_status = WORKING;
    thr_listening = std::thread(&Server::accept_clients, this);
    thr_msg_handler = std::thread(&Server::msg_handler, this);
    thr_error_handler = std::thread(&Server::error_handler, this);
        
    return true;
  }


  bool Server::send_to_client(unsigned id, char * buffer, unsigned buffer_size)
  {
    if (id > max_iteration || clients[id].connected == false) return false;
    if (send(clients[id].soc, buffer, buffer_size, 0) == SOCKET_ERROR)
    {
      clients[id].connected = false;
      invalid_clients.push(id);
      return false;
    }

    return true;
  }

  int Server::send_all(char * buffer, unsigned buffer_size)
  {
    int success_cnt = 0;
    for (int i = 0; i < max_iteration; i++)
    {
      if (!clients[i].connected) continue;
      if (send(clients[i].soc, buffer, buffer_size, 0) == SOCKET_ERROR)
      {
        clients[i].connected = false;
        invalid_clients.push(i);
      }
      else success_cnt++;
    }
    
    return success_cnt;
  }

  CLIENT_MESSAGE Server::next_message(bool blocking)
  {
    CLIENT_MESSAGE msg;
    msg.client_id = -1;

    do
    {
      if (!clients_messages.empty())
      {
        msg = clients_messages.front();
        clients_messages.pop();
      }
    } while ((msg.client_id < 0) && (blocking == BLOCKING));
    
    return msg;
  }

  void Server::stop()
  {
    server_status = SHUTTING_DOWN;
    closesocket(soc);

    for (int i = 0; i < max_iteration; i++)
    {
      closesocket(clients[i].soc);
    }

    while (!clients_messages.empty())
    {
      delete[] clients_messages.front().buffer;
      clients_messages.pop();
    }

    clients.clear();

    WSACleanup();
    server_status = DISABLED;
  }

}