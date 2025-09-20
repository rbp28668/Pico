#include "clock_webapp.hpp"
#include "display.hpp"
#include "tick.hpp"

extern BcdDisplay display;
extern Tick ticker;

bool ClockWebapp::matches(const char *verb, const char *path)
{
    bool accept = false;
    if (strcmp(verb, "GET") == 0)
    {
        accept =
            strncmp(path, "/digits", 7) == 0 || // inc digitshsv
            strncmp(path, "/colons", 7) == 0 || // inc colonshsv
            strncmp(path, "/notify1", 8) == 0 ||
            strncmp(path, "/notify2", 8) == 0 ||
            strncmp(path, "/autobright", 11) == 0 ||
            strncmp(path, "/tick", 5) == 0 ||
            false;
        ;
    }
    return accept;
}

void ClockWebapp::process(HttpRequest &request, HttpResponse &response)
{

    uint32_t rgb = 0;
    float h = 0;
    float s = 0;
    float v = 0;
    bool on = false;
    BlockListIter<Parameter> iter = request.Parameters().iter();
    Parameter *p;
    while (p = iter.next())
    {
        if (strcmp(p->name(), "rgb") == 0)
        {
            rgb = p->asRgb();
        }
        else if (strcmp(p->name(), "h") == 0)
        {
            h = p->asFloat();
        }
        else if (strcmp(p->name(), "s") == 0)
        {
            s = p->asFloat();
        }
        else if (strcmp(p->name(), "v") == 0)
        {
            v = p->asFloat();
        }
        else if (strcmp(p->name(), "on") == 0)
        {
            on = strcmp(p->value(), "true") == 0;
        }
    }

    if (strncmp(request.path(), "/digitshsv", 10) == 0)
    {
        display.setDigitColour(h, s, v);
        display.redraw();
    }
    else if (strncmp(request.path(), "/colonshsv", 10) == 0)
    {
        display.setColonColour(h, s, v);
        display.redraw();
    }
    else if (strncmp(request.path(), "/digits", 7) == 0)
    {
        display.setDigitColour(rgb);
        display.redraw();
    }
    else if (strncmp(request.path(), "/colons", 7) == 0)
    {
        display.setColonColour(rgb);
        display.redraw();
    }
    else if (strncmp(request.path(), "/notify1", 8) == 0)
    {
        display.setNotify1(rgb);
    }
    else if (strncmp(request.path(), "/notify2", 8) == 0)
    {
        display.setNotify2(rgb);
    }
    else if (strncmp(request.path(), "/autobright", 11) == 0)
    {
        display.setAutoBrightness(on);
    }
    else if (strncmp(request.path(), "/tick", 5) == 0)
    {
        ticker.enable(on);
    }

    response.setStatus(200, "OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Access-Control-Allow-Origin", "*");
}