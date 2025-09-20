#ifndef CLOCK_WEBAPP_HPP
#define CLOCK_WEBAPP_HPP
#include "../WebServer/webserver.hpp"

class ClockWebapp : public WebApp
{
public:
    virtual bool matches(const char *verb, const char *path);
    virtual void process(HttpRequest &request, HttpResponse &response);
};
#endif