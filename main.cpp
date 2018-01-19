#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <zconf.h>

class WifiSocket {
    int sock = -1;
    bool isOpened_;
    sockaddr_in server {};
public:
    WifiSocket(const std::string &address, const int port) {
        if (sock == -1) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                const std::string errMsg("Could not create socket." );
                std::cout << errMsg << std::endl;
                throw std::runtime_error(errMsg);
            }
        }
        if (inet_addr(address.c_str()) == -1) {
            struct hostent *he;
            struct in_addr **addr_list;
            if ((he = gethostbyname(address.c_str())) == nullptr) {
                herror("gethostbyname");
                const std::string errMsg("Failed to resolve hostname.");
                std::cout << errMsg << std::endl;
                throw std::runtime_error(errMsg);
            }
            addr_list = (struct in_addr **)he->h_addr_list;
            server.sin_addr = *addr_list[0];
        } else {
            server.sin_addr.s_addr = inet_addr(address.c_str());
        }
        server.sin_family = AF_INET;
        server.sin_port = htons(static_cast<uint16_t >(port));
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            const std::string errMsg("Connect failed. Error.");
            std::cerr << errMsg << std::endl;
            throw std::runtime_error(errMsg);
        }
        isOpened_ = true;
    }
    ~WifiSocket() {
        this->closeConnection();
    }

    bool closeConnection() {
        if (!isOpened_) {
            close(sock);
            sock = -1;
            isOpened_ = false;
        }
    }

    std::string getAnswer() {
        const int size = 32;
        char buffer[size];
        ssize_t recvCount = size;
        std::stringstream strBuilder;
        while (buffer[size - 1] != '<') {
            recvCount = recv(sock, buffer, size, 0);
            if (recvCount < 0) {
                const std::string errMsg("receive failed!");
                std::cerr << errMsg << std::endl;
                throw std::runtime_error(errMsg);
            }
            strBuilder.write(buffer, recvCount);
        }
        return strBuilder.str();
    }

    bool sendMessage(const std::string &data) {
        data.size();
        if (send(sock, data.c_str(), data.size(), 0) < 0) {
            std::cout << "Send failed: " << data << std::endl;
            return false;
        }
        return true;
    }
};

class Command {
    std::string command, description;
public:
    Command(const std::string &command, const std::string &description) {
        this->command = command;
        this->description = description;
    }

    std::string &getCommand() { return this->command; }
    std::string &getDescription() { return  this->description; }
};

int main() {
    std::ifstream file("test.txt");
    std::vector<Command> cmds;
    do {
        std::string line;
        getline(file, line);
        ulong fS = line.find(' ');
        if (fS == std::string::npos) {
            continue;
        }
        std::string ll = line.substr(0, fS);
        Command cmd(ll, line.substr(fS + 1, line.size()));
        cmds.push_back(cmd);
    } while (!file.eof());
    file.close();

    WifiSocket sock("127.0.0.1", 35000);
    std::ofstream log("log.txt");
    while (true) {
        int i = 0;
        for (auto cmd : cmds) {
            std::cout << '(' << i << ") " << cmd.getCommand() << ": " << cmd.getDescription() << std::endl;
        }

        int var = -1;
        std::cin >> var;

        if (!std::cin.good()) {
            continue;
        }

        if (0 > var && var > cmds.size()) {
            break;
        }

        sock.sendMessage(cmds[var - 1].getCommand() + "\r");
        log << cmds[var - 1].getCommand() + "\n";
        const std::string answer = sock.getAnswer();
        log << answer;
        std::cout << answer <<std::endl;

    }

    return 0;
}