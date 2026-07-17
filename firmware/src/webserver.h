#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

class HttpServer {
public:
    void begin();
    void handle();
};

extern HttpServer httpServer;
extern WebServer server;
#endif
