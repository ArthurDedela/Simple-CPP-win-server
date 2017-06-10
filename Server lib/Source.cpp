#include "Server.h"
#include <iostream>

using namespace std;

int main()
{
  tcp_server::Server server;

  cout << server.start() << endl;

  cout << server.status() << endl;
  

  while (true)
  {
    tcp_server::CLIENT_MESSAGE msg = server.next_message();

    cout << "Client ID: " << msg.client_id << " Message: " << msg.buffer << endl;

    cout << "Sended to " << server.send_all(msg.buffer, msg.buffer_size) << " clients" << endl;
    
    if (msg.buffer) delete[] msg.buffer;
  }




  return 0;
}