#ifndef HISTORY_WEBAPP_HPP
#define HISTORY_WEBAPP_HPP
#include "../WebServer/webserver.hpp"

class HistoryWebapp : public WebApp
{
    static char* id(char* pos, const char* name);
    static char* value(char* pos, uint64_t value);
public:
    virtual bool matches(const char *verb, const char *path);
    virtual void process(HttpRequest &request, HttpResponse &response);
};
#endif