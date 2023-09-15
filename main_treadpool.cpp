#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <pthread.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

// 最大线程数
#define MAX_THREADS 2
#define PORT 56765

// 任务队列
std::queue<int> clientQueue;
std::mutex queueMutex;
std::condition_variable queueCondition;

// 处理客户端连接的函数
void handleClient(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            std::cerr << "Error: Failed to receive data from client" << std::endl;
            break;
        } else if (bytesRead == 0) {
            std::cerr << "Client disconnected" << std::endl;
            break;
        }
        std::cout << "from client:" << clientSocket << ", receive data :" << buffer << std::endl;
        buffer[0] = 's';
        buffer[1] = ':';
        // 向客户端发送接收到的数据
        send(clientSocket, buffer, bytesRead, 0);
    }

    // 关闭客户端套接字
    close(clientSocket);
}

// 线程池函数
void threadPool() {
    while (true) {
        int clientSocket;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
//            std::cout << "std::unique_lock<std::mutex> lock(queueMutex); start\n";
            while (clientQueue.empty()) {
                queueCondition.wait(lock); // lock 
            }
            clientSocket = clientQueue.front();
            clientQueue.pop();
//            std::cout << "std::unique_lock<std::mutex> lock(queueMutex); end\n";
        }

        handleClient(clientSocket);
    }
}

int main() {
    // 创建服务器套接字
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error: Could not create server socket" << std::endl;
        return 1;
    }

    // 设置服务器地址和端口
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);  // 服务器监听端口
    serverAddress.sin_addr.s_addr = INADDR_ANY; // 任意可用的本地地址

    // 绑定服务器套接字到地址和端口
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error: Could not bind server socket to address" << std::endl;
        perror("bind");
        close(serverSocket);
        return 1;
    }

    // 监听客户端连接
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error: Could not listen for incoming connections" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port 56765..." << std::endl;

    // 创建线程池
    std::vector<std::thread> threadPoolVector;
    for (int i = 0; i < MAX_THREADS; ++i) {
        threadPoolVector.push_back(std::thread(threadPool));
    }

    while (true) {
        // 接受客户端连接
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == -1) {
            std::cerr << "Error: Could not accept client connection" << std::endl;
            continue;
        }

        std::cout << "Client connected from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << std::endl;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            clientQueue.push(clientSocket);
        }

        // 通知线程池有任务
        queueCondition.notify_one();
    }

    // 关闭服务器套接字和线程池（通常不会执行到这一步）
    close(serverSocket);

    for (std::thread& t : threadPoolVector) {
        t.join();
    }

    return 0;
}
