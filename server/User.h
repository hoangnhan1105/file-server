#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

class User
{
    friend class Program;
public:
    User();
    User(SOCKET const& acceptSocket);
    ~User();

private:
    std::string Username;
    std::string Password;
    SOCKET AcceptSocket;
    // MutexSending is used in 5 cases:
    // - verifyUserLogin (Send new user's info to other clients
    //         for them to print log)
    // - sendAFileToClient
    // - receiveAFileFromClient (Send new file's info to other clients
    //         for them to update shared file list)
    // - sendALogoutReply (Send a reply to the logging out user)
    //     + Also notify other clients about the logging out user.
    // - sendALogoutRequest (Request all Online Clients to logout
    //         so the server can shut down)
    std::mutex MutexSending;
};

