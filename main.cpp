#include <vector>
#include <csignal>
#include <sstream>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <zconf.h>
#include <arpa/inet.h>


bool STOPED = false;

void stopLoop(int s) {
    STOPED = true;
    std::cout << "The program is going to stop" << std::endl;
    std::cout << "You are a monster!" << std::endl;
}

class WifiSocket {
protected:
    int sock = -1;
    bool isOpened_;
    sockaddr_in server {};
public:
    WifiSocket(const std::string &address, const int port) {
        if (sock == -1) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                const std::string errMsg("Could not create socket.");
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

class Commands {
public:
    const static std::string &getRpmCode() {
        return Commands::rpm;
    }

    const static std::string &getSpeedCode() {
        return Commands::speed;
    }

    const static std::string &getEngineMomentCode() {
        return Commands::engineMoment;
    }

    const static std::string &getEchoOffCode() {
        return Commands::echoOff;
    }

protected:
    const static std::string rpm, speed, engineMoment, echoOff;

    Commands() {}

    ~Commands() = default;
};

const std::string Commands::rpm = "010c\r";

const std::string Commands::speed = "010d\r";

const std::string Commands::engineMoment = "0162\r";

const std::string Commands::echoOff= "ATE0\r";

//class Commands {
//    std::string command, description;
//public:
//    lol(const std::string &command, const std::string &description) {
//        this->command = command;
//        this->description = description;
//    }
//
//    std::string &getCommand() { return this->command; }
//
//    std::string &getDescription() { return this->description; }
//};

int main() {
//    std::ifstream file("test.txt");
//    std::vector<lol> cmds;
//    while (!file.eof()) {
//        std::string line;
//        getline(file, line);
//        ulong fS = line.find(' ');
//        if (fS == std::string::npos) {
//            continue;
//        }
//        std::string ll = line.substr(0, fS);
//        lol cmd(ll, line.substr(fS + 1, line.size()));
//        cmds.push_back(cmd);
//    }
//    file.close();


    // ctrl + c handler
    signal(SIGINT, stopLoop);

    WifiSocket sock("localhost", 35000);
    sock.sendMessage(Commands::getEchoOffCode());
    sock.getAnswer();
    std::ofstream log("log.txt");
    log << "{ [ ";
    while (!STOPED) {
        log << "{ ";
        sock.sendMessage(Commands::getRpmCode());
        log << '"' << Commands::getRpmCode() << "\" : \"" << std::endl;
        log << sock.getAnswer() << "\", ";

        sock.sendMessage(Commands::getSpeedCode());
        log << '"' << Commands::getSpeedCode() << "\" : \"" << std::endl;
        log << sock.getAnswer() << "\", ";

        sock.sendMessage(Commands::getEngineMomentCode());
        log << '"' << Commands::getEngineMomentCode() << "\" : \"" << std::endl;
        log << sock.getAnswer() << "\" ";
        log.flush();
    }
    log << "] }" << std::endl;
    log.close();
    return 0;
}