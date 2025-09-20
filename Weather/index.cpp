#include <string.h>
#include <sstream>
#include "index.hpp"
#include "letterbox.hpp"

// Toolchain for generating this data from form.html
//https://www.toptal.com/developers/html-minifier
//https://tomeko.net/online_tools/cpp_text_escape.php?lang=en
static const char* start = 
"<html>\n"
"<style>\n"
".values {\n"
"font-size: 20px;\n"
"}\n"
"</style>\n"
"<head>\n"
"<title>PicoW Weather</title>\n"
"<meta name=viewport content=\"width=device-width,initial-scale=1\">\n"
"<style>legend{font-style:italic;font-weight:700}fieldset{margin-bottom:7px;max-width:1024px}canvas{border:1px solid #000;height:20;width:100%;grid-column:1}input[type=range]{width:100%}input[type=number]{width:100%;max-width:10em}.grid{display:grid;row-gap:10px;grid-template-columns:auto}input{grid-column:1}label{grid-column:1}@media screen and (min-width:550px){.grid{display:grid;column-gap:20px;grid-template-columns:100px auto 10em}input{grid-column:2}label{grid-column:1}canvas{border:1px solid #000;height:20;width:100%;grid-column:2}.c3{grid-column:3}.r1{grid-row:1}.r2{grid-row:2}.r3{grid-row:3}.r4{grid-row:4}}</style>\n"
"</head>\n"
"<body>\n"
"<h1>PicoW Weather</h1>"
;

static const char* end = 
"</body>\n"
"</html>"
;

extern Letterbox letterbox;

bool IndexPage::matches(const char* verb, const char* path){
    bool accept = (strcmp(verb,"GET") == 0) && (
        strncmp(path, "/",1) == 0 ||
        strncmp(path, "/index",6) == 0
        );
     return accept;
}

void IndexPage::process( HttpRequest& request, HttpResponse& response){
    std::ostringstream os;
    os << start;
    os << "<div class=\"values\">" << std::endl;
    os << "<div> Pressure: " << letterbox.pressure << "hPa </div>" << std::endl;
    os << "<div> Temperature: " << letterbox.primaryTemp << "C </div>" << std::endl;
    os << "<div> Humidity: " << letterbox.humidity << "% </div>" << std::endl;
    os << "<div> Light: " << letterbox.lux << "lux </div>" << std::endl;
    // os << "<div> Temp2: " << letterbox.temp2 << "C </div>" << std::endl;
    // os << "<div> Temp3: " << letterbox.temp3 << "C </div>" << std::endl;
    os << "</div>" << std::endl;
    os << end;

    const char* body = os.str().c_str();
    if(!os) body = "Bad output stream\n";

    puts(body);
    response.setStatus(200,"OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Access-Control-Allow-Origin","*");

    response.setBody(body);
}
