
#include "adc.hpp"
#include "adc_webapp.hpp"

extern Adc adc;

static const char *page =
    "<html>\n"
    "<head>\n"
    "<title>BCD Clock</title>\n"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">\n"
    "<style>legend{font-style:italic;font-weight:700}fieldset{margin-bottom:7px;max-width:1024px}canvas{border:1px solid #000;height:20;width:100%;grid-column:1}input[type=range]{width:100%}input[type=number]{width:100%;max-width:10em}.grid{display:grid;row-gap:10px;grid-template-columns:auto}input{grid-column:1}label{grid-column:1}@media screen and (min-width:550px){.grid{display:grid;column-gap:20px;grid-template-columns:100px auto 10em}input{grid-column:2}label{grid-column:1}canvas{border:1px solid #000;height:20;width:100%;grid-column:2}.c3{grid-column:3}.r1{grid-row:1}.r2{grid-row:2}.r3{grid-row:3}.r4{grid-row:4}}</style>\n"
    "</head>\n"
    "<body>\n"
    "<h1>ADC</h1>\n"
    "</body>\n"
    "</html>";


static char output[1024];


bool AdcWebapp::matches(const char *verb, const char *path)
{
    bool accept = false;
    if (strcmp(verb, "GET") == 0)
    {
        accept =
            strncmp(path, "/adc", 4) == 0 ||
            strncmp(path, "/adcdata", 8) == 0 ||
            false;
        ;
    }
    return accept;
}

void AdcWebapp::process(HttpRequest &request, HttpResponse &response)
{

    const char *body;

    if (strncmp(request.path(), "/adcdata", 8) == 0)
    {
        sprintf(output, "ADC Counts: %u", (int) adc.read());
        body = output;
    }
    else if (strncmp(request.path(), "/adc", 4) == 0)
    {
        body = page;
    }

    response.setStatus(200, "OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Access-Control-Allow-Origin", "*");

    response.setBody(body);
}