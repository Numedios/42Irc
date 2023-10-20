#ifndef SERVEUR_HPP
#define SERVEUR_HPP

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <queue>
#include <typeinfo>
#include <map>
#include <sstream>
#include "Client.hpp"
#include "Channel.hpp"
#include "Responses.hpp"

class Serveur;

typedef int (*FunctionPtr)(const std::string&, Client&, Serveur&);
void fillCommandMapAuth(std::map<std::string, FunctionPtr>& commands);
void fillCommandMap(std::map<std::string, FunctionPtr>& commands);
std::string getNthWord(const std::string& input, int place);
void mergeQueues(std::queue<std::string>& q1, std::queue<std::string>& q2);
std::queue<std::string> splitToQueue(const std::string& input, char delimiter);
std::string convertIntToString(int value) ;
std::string create_message(Client client, std::string capted, std::string message);
void sendResponse(Client& client, Serveur& server, const std::string& response);
std::string convertIntToString(int value);

int handleJoin(const std::string& line, Client& client, Serveur& serveur);

class Client;
class Channel;

const int MAX_CLIENTS = 10;

class Serveur {
public:
    Serveur(int port, char *password);
    void run();
    void printInfoServeur();
    int acceptClient();
    int processReceivedData(int i);
    void handleClientDisconnect(int index);
    int processAuthenticationManagement(int index);
    int processCommandManagement(int i);
    void displayHistory();
    
    int getServeurSocket(){
        return _serverSocket;
    }

    int getNumClients(){
        return _numClients;
    }

    int getFdsSocket(int numClient){
        return fds[numClient].fd;
    }

    std::string getPassword()
    {
        return _password;
    }

    std::map<int , Client *>  getClients(){
        return _clients;
    }

    Client* getClient(int i){
        return _clients[i];
    }

    std::map<std::string, FunctionPtr>&  getCommands()
    {
        return _commands;
    }

    std::map<std::string, FunctionPtr>& getAuth(){
        return _auth;
    }

    std::map<std::string, Channel>& getChannels()
    {
        return _channels;
    }

    void addHistoryChat(std::string message) {
        _historyChat.push_back(message);
    }

    std::queue<std::string>& getmessageQueue(){
        return _messageQueue;
    }

    void setNumCLients(int numClients)
    {
        _numClients = numClients;
    }

    void setFds(int clientSocket, int numClients)
    {
        fds[numClients].fd = clientSocket;
        fds[numClients].events = POLLIN;
        fds[numClients].revents = 0;
    }
    
    void addChannel(std::string name,Channel& channel) {
        _channels[name] = channel;
    }

private:

    int _port;  //rajouter parsing des port
    int _serverSocket; 
    int _numClients; //x
    std::string _password;
    struct pollfd fds[MAX_CLIENTS + 1];

    std::map<int , Client *> _clients;
    std::map<std::string, FunctionPtr>  _commands;
    std::map<std::string, FunctionPtr>  _auth; 
    std::map<std::string, Channel>		_channels;
    std::vector<std::string> _historyChat; // historique des messages emis et recu 
    std::queue<std::string> _messageQueue; // file ordre de traitement des messages
};

#endif