#ifndef HTTPLIB_H
#define HTTPLIB_H

/*
 * File: httplib.h
 * Purpose: Single-header zero-dependency local HTTP server with httplib-compatible API.
 * DSA Concept: Event-driven socket I/O, routing table lookup.
 * Time Complexity: Route matching O(R) where R is number of registered endpoints.
 */

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
typedef int (WSAAPI *PFN_WSAStartup)(WORD, LPWSADATA);
typedef int (WSAAPI *PFN_WSACleanup)(void);
typedef SOCKET (WSAAPI *PFN_socket)(int, int, int);
typedef int (WSAAPI *PFN_bind)(SOCKET, const struct sockaddr*, int);
typedef int (WSAAPI *PFN_listen)(SOCKET, int);
typedef SOCKET (WSAAPI *PFN_accept)(SOCKET, struct sockaddr*, int*);
typedef int (WSAAPI *PFN_closesocket)(SOCKET);
typedef int (WSAAPI *PFN_recv)(SOCKET, char*, int, int);
typedef int (WSAAPI *PFN_send)(SOCKET, const char*, int, int);
typedef u_short (WSAAPI *PFN_htons)(u_short);
typedef u_long (WSAAPI *PFN_htonl)(u_long);
typedef int (WSAAPI *PFN_setsockopt)(SOCKET, int, int, const char*, int);
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace httplib {

struct Request {
    std::string method;
    std::string path;
    std::string body;
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "text/plain";

    void set_content(const std::string& s, const std::string& type) {
        body = s;
        content_type = type;
    }
};

class Server {
public:
    using Handler = std::function<void(const Request&, Response&)>;

private:
    struct Route {
        std::string method;
        std::string path;
        Handler handler;
    };
    std::vector<Route> routes;

public:
    // Registers a GET handler for a specific path.
    void Get(const std::string& path, Handler handler) {
        routes.push_back({"GET", path, handler});
    }

    // Registers a POST handler for a specific path.
    void Post(const std::string& path, Handler handler) {
        routes.push_back({"POST", path, handler});
    }

    // Starts listening on the specified port.
    bool listen(const std::string& host, int port) {
#ifdef _WIN32
        HMODULE hWs2 = LoadLibraryA("ws2_32.dll");
        if (!hWs2) return false;
        PFN_WSAStartup pWSAStartup = (PFN_WSAStartup)GetProcAddress(hWs2, "WSAStartup");
        PFN_WSACleanup pWSACleanup = (PFN_WSACleanup)GetProcAddress(hWs2, "WSACleanup");
        PFN_socket p_socket = (PFN_socket)GetProcAddress(hWs2, "socket");
        PFN_bind p_bind = (PFN_bind)GetProcAddress(hWs2, "bind");
        PFN_listen p_listen = (PFN_listen)GetProcAddress(hWs2, "listen");
        PFN_accept p_accept = (PFN_accept)GetProcAddress(hWs2, "accept");
        PFN_closesocket p_closesocket = (PFN_closesocket)GetProcAddress(hWs2, "closesocket");
        PFN_recv p_recv = (PFN_recv)GetProcAddress(hWs2, "recv");
        PFN_send p_send = (PFN_send)GetProcAddress(hWs2, "send");
        PFN_htons p_htons = (PFN_htons)GetProcAddress(hWs2, "htons");
        PFN_htonl p_htonl = (PFN_htonl)GetProcAddress(hWs2, "htonl");
        PFN_setsockopt p_setsockopt = (PFN_setsockopt)GetProcAddress(hWs2, "setsockopt");

        WSADATA wsa;
        if (pWSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#define S_SOCKET p_socket
#define S_BIND p_bind
#define S_LISTEN p_listen
#define S_ACCEPT p_accept
#define S_CLOSESOCKET p_closesocket
#define S_RECV p_recv
#define S_SEND p_send
#define S_HTONS p_htons
#define S_HTONL p_htonl
#define S_SETSOCKOPT p_setsockopt
#else
#define S_SOCKET socket
#define S_BIND bind
#define S_LISTEN listen
#define S_ACCEPT accept
#define S_CLOSESOCKET closesocket
#define S_RECV recv
#define S_SEND send
#define S_HTONS htons
#define S_HTONL htonl
#define S_SETSOCKOPT setsockopt
#endif

        SOCKET server_fd = S_SOCKET(AF_INET, SOCK_STREAM, 0);
        if (server_fd == INVALID_SOCKET) return false;

        int opt = 1;
        S_SETSOCKOPT(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = S_HTONL(INADDR_ANY);
        address.sin_port = S_HTONS(port);

        if (S_BIND(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
            S_CLOSESOCKET(server_fd);
            return false;
        }

        if (S_LISTEN(server_fd, 10) == SOCKET_ERROR) {
            S_CLOSESOCKET(server_fd);
            return false;
        }

        std::cout << "HTTP Server live at http://localhost:" << port << "\n";

        while (true) {
            struct sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            SOCKET client_fd = S_ACCEPT(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == INVALID_SOCKET) continue;

            std::string raw = "";
            char buf[4096];
            size_t body_pos = std::string::npos;
            int content_len = 0;

            while (true) {
                int bytes = S_RECV(client_fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) break;
                buf[bytes] = '\0';
                raw += buf;

                if (body_pos == std::string::npos) {
                    body_pos = raw.find("\r\n\r\n");
                    if (body_pos != std::string::npos) {
                        size_t cl_pos = raw.find("Content-Length:");
                        if (cl_pos == std::string::npos) cl_pos = raw.find("content-length:");
                        if (cl_pos != std::string::npos) {
                            size_t end_cl = raw.find("\r\n", cl_pos);
                            std::string num_str = raw.substr(cl_pos + 15, end_cl - (cl_pos + 15));
                            while (!num_str.empty() && (num_str.front() == ' ' || num_str.front() == '\t')) num_str.erase(0, 1);
                            try { content_len = std::stoi(num_str); } catch (...) { content_len = 0; }
                        }
                    }
                }

                if (body_pos != std::string::npos) {
                    if ((int)raw.size() >= (int)body_pos + 4 + content_len) {
                        break;
                    }
                }
            }

            if (!raw.empty()) {
                Request req;
                size_t first_space = raw.find(' ');
                size_t second_space = (first_space != std::string::npos) ? raw.find(' ', first_space + 1) : std::string::npos;
                if (first_space != std::string::npos && second_space != std::string::npos) {
                    req.method = raw.substr(0, first_space);
                    req.path = raw.substr(first_space + 1, second_space - first_space - 1);
                }
                if (body_pos != std::string::npos) {
                    req.body = raw.substr(body_pos + 4, content_len > 0 ? content_len : std::string::npos);
                }

                Response res;
                if (req.method == "OPTIONS") {
                    res.status = 200;
                    res.body = "";
                } else {
                    bool matched = false;
                    for (const auto& r : routes) {
                        if (r.method == req.method && r.path == req.path) {
                            r.handler(req, res);
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        res.status = 404;
                        res.set_content("{\"error\":\"Endpoint not found\"}", "application/json");
                    }
                }

                std::ostringstream ss;
                ss << "HTTP/1.1 " << res.status << " OK\r\n";
                ss << "Content-Type: " << res.content_type << "\r\n";
                ss << "Content-Length: " << res.body.size() << "\r\n";
                ss << "Access-Control-Allow-Origin: *\r\n";
                ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
                ss << "Access-Control-Allow-Headers: *\r\n";
                ss << "Connection: close\r\n\r\n";
                ss << res.body;

                std::string resp_str = ss.str();
                S_SEND(client_fd, resp_str.c_str(), (int)resp_str.size(), 0);
            }
            S_CLOSESOCKET(client_fd);
        }

        S_CLOSESOCKET(server_fd);
#ifdef _WIN32
        pWSACleanup();
        FreeLibrary(hWs2);
#endif
        return true;
    }
};

} // namespace httplib

#endif
