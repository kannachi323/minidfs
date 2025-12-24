#pragma once

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string>

std::string GetLocalIP() {
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    std::string ip = "127.0.0.1";

    if (getifaddrs(&ifAddrStruct) == -1) {
        return ip;
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        // Check for IPv4
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char addressBuffer[INET_ADDRSTRLEN];
            void* tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            std::string found_ip(addressBuffer);
            // Ignore loopback so we get the actual network IP (WiFi/Ethernet)
            if (found_ip != "127.0.0.1") {
                ip = found_ip;
                break; 
            }
        }
    }

    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return ip;
}