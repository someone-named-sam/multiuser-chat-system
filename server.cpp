#include <iostream>
#include <thread>
#include <string>
#include <WinSock2.h>

using namespace std;

#pragma comment (lib, "ws2_32.lib")

#define NEWLINE "\n"

fd_set master;

void connection_handler(SOCKET soc)
{
    char clientName[50];
    ZeroMemory(clientName, 50);
    recv(soc, clientName, sizeof(clientName), 0);

    string welcome_message = "Server: Welcome to chat server ";
    welcome_message = welcome_message + clientName;
    for (auto i = 0; i < master.fd_count; i++)
    {
        send(master.fd_array[i], welcome_message.c_str(), strlen(welcome_message.c_str()), 0);
    }

    cout << '\r';
    cout << clientName << " connected" << NEWLINE;
    cout << "Server: ";

    while (true)
    {
        char recvbuf[512];
        ZeroMemory(recvbuf, 512);

        auto iRecv = recv(soc, recvbuf, sizeof(recvbuf), 0);
        if (iRecv > 0)
        {
            cout << '\r';
            cout << recvbuf << NEWLINE;
            cout << "Server: ";
            for (int i = 0; i < master.fd_count; i++)
            {
                if (master.fd_array[i] != soc)
                    send(master.fd_array[i], recvbuf, strlen(recvbuf), 0);
            }
        }
        else if (iRecv == 0) {
            FD_CLR(soc, &master);
            cout << '\r';
            cout << "A client disconnected" << NEWLINE;
            cout << "Server: ";
            // Annouce to others
            for (int i = 0; i < master.fd_count; i++) {
                char disconnected_message[] = "Server : A client disconnected";
                send(master.fd_array[i], disconnected_message, strlen(disconnected_message), 0);
            }
            return;
        }
        else {
            FD_CLR(soc, &master);
            cout << '\r';
            cout << "a client receive failed: " << WSAGetLastError() << NEWLINE;
            cout << "Server: ";
            // Annouce to others
            for (int i = 0; i < master.fd_count; i++) {
                char disconnected_message[] = "Server : A client disconnected";
                send(master.fd_array[i], disconnected_message, strlen(disconnected_message), 0);
            }
            return;
        }
    }
}

// multiple client using threads for each client
void accept_handler(SOCKET soc)
{
    while (true)
    {
        auto acceptSoc = accept(soc, nullptr, nullptr);
        if (acceptSoc == INVALID_SOCKET) {
            cout << '\r';
            cout << "accept error: " << WSAGetLastError() << NEWLINE;
            cout << "Server: ";
            closesocket(acceptSoc);
        }
        else
        {
            thread newCon = thread(connection_handler, acceptSoc);
            newCon.detach();
            FD_SET(acceptSoc, &master);
        }
    }
}

void send_handler(SOCKET soc)
{
    string data;
    do
    {
        cout << "Server: ";
        getline(std::cin, data);
        data = "Server: " + data;
        for (auto i = 0; i < master.fd_count; i++)
        {
            auto result = send(master.fd_array[i], data.c_str(), strlen(data.c_str()), 0);
            if (result == SOCKET_ERROR) {
                cout << '\r';
                cout << "send to client " << i << " failed: " << WSAGetLastError() << NEWLINE;
                cout << "Server: ";
                return;
            }
        }
    } while (data.size() > 0);
}

int main(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cout<<"WSAStartup failed"<<endl;
        return;
    }
    cout<<"WSAStartup successfull"<<endl;

    auto serverSoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSoc == INVALID_SOCKET)
    {
        cout<<"Socket creation failed"<<endl;
        return;
    }
    cout<<"Socket creation successfull"<<endl;

    // fill server address
    struct sockaddr_in TCPServerAdd;
    TCPServerAdd.sin_family = AF_INET;
    TCPServerAdd.sin_port = htons(8000);
    TCPServerAdd.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    auto result = bind(serverSoc, (sockaddr*)& TCPServerAdd, sizeof(TCPServerAdd));
    if (result == SOCKET_ERROR)
    {
        cout<<"listen failed"<<endl;
        closesocket(serverSoc);
        WSACleanup();
        return;
    }
    cout<<"bind successfull"<<endl;

    result = listen(serverSoc, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        cout<<"listen failed"<<endl;
        closesocket(serverSoc);
        WSACleanup();
        return;
    }
    cout<<"listen successfull"<<endl;;

    thread acceptThread = thread(accept_handler, serverSoc);
    acceptThread.detach();

    thread sendThread = thread(send_handler, serverSoc);
    sendThread.join();

    closesocket(serverSoc);
    WSACleanup();
}