#include <iostream>
#include <string>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

const int MAX_EVENTS = 100;
const int BUFFER_SIZE = 1024;
const std::string RESPONSE_OK = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: 48\r\n"
                                "\r\n"
                                "<html><body><h1>Hello, Web Server With Epoll!!!</h1></body></html>";
const std::string RESPONSE_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: 37\r\n"
                                       "\r\n"
                                       "<html><body><h1>404 Not Found</h1></body></html>";

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool parseRequest(const std::string& request, std::string& path)
{
// Find the first occurrence of space character
    size_t spacePos = request.find(' ');
    if (spacePos == std::string::npos)
        return false;

// Extract the substring between the second and third space characters
    size_t secondSpacePos = request.find(' ', spacePos + 1);
    if (secondSpacePos == std::string::npos)
        return false;

    path = request.substr(spacePos + 1, secondSpacePos - spacePos - 1);
    return true;
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Failed to create server socket" << std::endl;
        return 1;
    }


    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Failed to bind server socket" << std::endl;
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == -1)
    {
        std::cerr << "Failed to listen on server socket" << std::endl;
        return 1;
    }

    int epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cerr << "Failed to create epoll" << std::endl;
        return 1;
    }

    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1)
    {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        return 1;
    }

    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    std::cout << "Web server started. Listening on port 8080..." << std::endl;

    while (true)
    {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1)
        {
            std::cerr << "epoll_wait error" << std::endl;
            break;
        }

        for (int i = 0; i < numEvents; i++)
        {
            if (events[i].data.fd == serverSocket)
            {
                sockaddr_in clientAddress{};
                socklen_t clientAddressSize = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
                if (clientSocket == -1)
                {
                    std::cerr << "Failed to accept client connection" << std::endl;
                    continue;
                }

                setNonBlocking(clientSocket);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clientSocket;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
                {
                    std::cerr << "Failed to add client socket to epoll" << std::endl;
                    return 1;
                }

                std::cout << "New client connected: " << inet_ntoa(clientAddress.sin_addr) << std::endl;
            }
            else
            {
                int clientSocket = events[i].data.fd;

                ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead == -1)
                {
                    std::cerr << "Failed to read data from client" << std::endl;
                    continue;
                }
                else if (bytesRead == 0)
                {
                    // Client closed connection
                    close(clientSocket);
                    std::cout << "Client disconnected" << std::endl;
                    continue;
                }

                // Process the received HTTP request
                std::string request(buffer, bytesRead);

                // Parse the request and extract the requested path
                std::string requestedPath;
                if (parseRequest(request, requestedPath))
                {
                    if (requestedPath == "/")
                    {
                        // Respond with a simple HTML page
                        ssize_t bytesSent = send(clientSocket, RESPONSE_OK.c_str(), RESPONSE_OK.length(), 0);
                        if (bytesSent == -1)
                        {
                            std::cerr << "Failed to send response to client" << std::endl;
                        }
                    }
                    else
                    {
                        // Respond with a 404 page
                        ssize_t bytesSent = send(clientSocket, RESPONSE_NOT_FOUND.c_str(), RESPONSE_NOT_FOUND.length(), 0);
                        if (bytesSent == -1)
                        {
                            std::cerr << "Failed to send response to client" << std::endl;
                        }
                    }
                }
                else
                {
                    // Invalid request, respond with a 404 page
                    ssize_t bytesSent = send(clientSocket, RESPONSE_NOT_FOUND.c_str(), RESPONSE_NOT_FOUND.length(), 0);
                    if (bytesSent == -1)
                    {
                        std::cerr << "Failed to send response to client" << std::endl;
                    }
                }

                // Close the connection
                close(clientSocket);
            }
        }
    }

    close(epollFd);
    close(serverSocket);

    return 0;
}