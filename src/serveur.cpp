#include "../include/Serveur.hpp"


std::string getNthWord(const std::string& input, int place);
void mergeQueues(std::queue<std::string>& q1, std::queue<std::string>& q2);

std::queue<std::string> splitToQueue(const std::string& input, char delimiter);
void fillCommandMapAuth(std::map<std::string, FunctionPtr>& commands);

Serveur::Serveur(int port, char *password) : _port(port), _password(password) 
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
    {
        std::cerr << "Error binding" << std::endl;
        close(_serverSocket);
        exit(0); // a enlever maybe
        return;
    }

    if (listen(_serverSocket, MAX_CLIENTS) == -1) {
        std::cerr << "Error listening" << std::endl;
        close(_serverSocket);
        return;
    }

    fds[0].fd = _serverSocket;
    fds[0].events = POLLIN;
    _numClients = 1;
    fillCommandMap(_commands);
    fillCommandMapAuth(_auth);
}

void Serveur::printInfoServeur()
{
    std::cout << "_port          = *" << _port << "*" << std::endl;
    std::cout << "_serveurSocket = *" << _serverSocket << "*" << std::endl;
    std::cout << "_numClients    = *" <<  _numClients << "*" << std::endl;
    std::cout << "_password      = *" << _password << "*" << std::endl;
}

void Serveur::run() 
{
    while (true) 
    {
        int numEvents = poll(fds, _numClients, 0); // verifier le retour de numEvents
        if (fds[0].revents & POLLIN) 
        {
            acceptClient();
        }    
        else 
        {
            for (int i = 1; i < _numClients; i++) {
                if (fds[i].revents & POLLIN) 
                {
                    int bytesReceived = processReceivedData(i);
                    if (_clients[fds[i].fd]->isAuthenticated() == false) //gestion commande authentification
                        processAuthenticationManagement(i);
                    else // gestion commande non lier a l'authentification
                    {
                        processCommandManagement(i);
                    }
                    if (bytesReceived <= 0)
                         handleClientDisconnect(i);
                    //displayHistory();
                }
            }
       }
    }
}

int Serveur::acceptClient()
{
    int clientSocket = accept(_serverSocket , NULL, NULL);
    int numClients = _numClients;
    if (numClients> MAX_CLIENTS)
    {
        std::cout << "Error : trop de client arrete " << std::endl;
    }
    else
    {
        if (clientSocket != -1)
        {
            setFds(clientSocket, numClients);
            //clients.push_back(Client(clientSocket, numClients));
            _clients[clientSocket] = new Client(clientSocket, numClients);
            setNumCLients(numClients + 1);
            std::cout << "New client connected. Total clients: " << numClients - 1 << std::endl;
        }
    }
    //_clients[clientSocket]->printInfoClient();
    return (0);
}


int Serveur::processReceivedData(int i) 
{
    char buffer[1024];
    int fd = getFdsSocket(i);

    memset(buffer, 0, sizeof(buffer)); //set a 0
    int bytesReceived = recv(getFdsSocket(i), buffer, sizeof(buffer) - 1, 0); //recu
    buffer[bytesReceived] = '\0'; // Ajouter un caractère nul à la fin des données reçues
    Client * client = getClients()[fd];
    std::string pendingCommand =   client->getPendingCommand();
    if (pendingCommand.find('\n') == std::string::npos && pendingCommand.find('\r') == std::string::npos)
    {
        size_t originalLength = strlen(buffer);
        memmove(buffer + pendingCommand.size(), buffer, originalLength + 1);
        memcpy(buffer, pendingCommand.c_str(), pendingCommand.size());
        originalLength += pendingCommand.size();
        client->setPendingCommand("");
    }
    std::queue<std::string> splitBuff = splitToQueue(buffer, '\n');
    if (strlen(buffer) != 0)
        addHistoryChat(create_message(*client, ":   <--", buffer));
    mergeQueues(getmessageQueue() , splitBuff);
    return bytesReceived;
}

void Serveur::handleClientDisconnect(int index)
{
    int fd = getFdsSocket(index);
    Client * client = getClients()[fd];

    client->setDisconnected();
    client->setAuthState(Client::NotAuthenticated);
    std::cout << "Client " << client->getId() << " disconnected." << std::endl;
    // std::string message = client.getNick() + "("+  convertIntToString(client.getSocket()) + ")has been disconnected. (leaving)\n";
    // _historyChat.push_back(create_message(client, ":SERVER", message));
    close(fds[index].fd);
    fds[index].fd = -1;
    _numClients--;
    getClients().erase(fd); // Supprimer le dernier élément après le décalage
}

int Serveur::processAuthenticationManagement(int index)
{
    int fd = getFdsSocket(index);
    Client * client = getClients()[fd];
    std::queue<std::string>& messageQueue = getmessageQueue();
    while (!(messageQueue.empty()))
    {
        const std::string& frontMessage = messageQueue.front();
        if ((frontMessage.find('\n') == std::string::npos) && (frontMessage.find('\r') == std::string::npos))
        {
            client->setPendingCommand(frontMessage);
            messageQueue.pop();
            break;
        }
        std::map<std::string, FunctionPtr>::iterator it = getAuth().find(getNthWord(frontMessage, 0));
        if (it != getAuth().end())
        {
            if (it->second(frontMessage.c_str() , *client, *this) == 1)
            {
                addHistoryChat(create_message(*client, ":TESTIN", "Error : echec authentification\n"));
                std::cout << "Error : echec authentification" << std::endl;
            }
        }
        else
        {
            addHistoryChat(create_message(*client, ":TESTIN", "Error : echec commande introuvable authentification non faite\n"));
            std::cout << "Errror commande introuvable authentification non faite" << std::endl;
        }
        messageQueue.pop();
    }
    return (0);
}

int Serveur::processCommandManagement(int index)
{
    int fd = getFdsSocket(index);
    Client * client = getClients()[fd];
    std::queue<std::string>& messageQueue = getmessageQueue();
    while (!(messageQueue.empty()))
    {
        const std::string& frontMessage = messageQueue.front(); 
                            
        std::map<std::string, FunctionPtr>::iterator it = getCommands().find(getNthWord(frontMessage, 0));
        if (it != getCommands().end())
        {
            if (it->second(frontMessage.c_str() , *client, *this) == 1)
            {
                addHistoryChat(create_message(*client, ":TESTIN", "Error : echec authentification\n"));
                std::cout << "Error : echec authentification" << std::endl;
            }
        }
        else
        {
            addHistoryChat(create_message(*client, ":TESTIN", "Error : echec commande introuvable authentification non faite\n"));
            //std::cout << "Errror commande introuvable" << std::endl;
        }
        if (!messageQueue.empty())
            messageQueue.pop();
    }
    return (0);
}

void Serveur::displayHistory() 
{
    std::system("clear");
    for (size_t i = 0; i < _historyChat.size(); ++i) {
        std::cout << _historyChat[i];
    }
}