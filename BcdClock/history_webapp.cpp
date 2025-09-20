#include "history_webapp.hpp"
#include "history.hpp"

extern History history;

// Toolchain for generating this data from form.html
// https://www.toptal.com/developers/html-minifier
// https://tomeko.net/online_tools/cpp_text_escape.php?lang=en
static const char *page =
    "<html>\n"
    "<head>\n"
    "<title>BCD Clock</title>\n"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">\n"
    "<style>legend{font-style:italic;font-weight:700}fieldset{margin-bottom:7px;max-width:1024px}canvas{border:1px solid #000;height:20;width:100%;grid-column:1}input[type=range]{width:100%}input[type=number]{width:100%;max-width:10em}.grid{display:grid;row-gap:10px;grid-template-columns:auto}input{grid-column:1}label{grid-column:1}@media screen and (min-width:550px){.grid{display:grid;column-gap:20px;grid-template-columns:100px auto 10em}input{grid-column:2}label{grid-column:1}canvas{border:1px solid #000;height:20;width:100%;grid-column:2}.c3{grid-column:3}.r1{grid-row:1}.r2{grid-row:2}.r3{grid-row:3}.r4{grid-row:4}}</style>\n"
    "<script>var baseURI=document.baseURI;function drawData(t,e){var a=document.getElementById(t),n=a.getContext(\"2d\");console.log(\"Writing \"+e),n.lineWidth=10;const r=a.width/60;var s=0;for(let t=0;t<15;++t){var o=Number.parseInt(e.substr(t,1),16);for(let t=0;t<4;++t)n.fillStyle=0!=(8&o)?\"black\":\"white\",n.fillRect(s,0,r,a.height),n.stroke,o<<=1,s+=r}}function configure(){var t=new XMLHttpRequest;t.onreadystatechange=function(){if(4==this.readyState&&200==this.status){const e=JSON.parse(t.responseText);drawData(\"lastMin\",e.lastMin);for(let t=0;t<24;++t){drawData(\"hour\"+t,e.hour[t])}}},t.open(\"GET\",\"historydata\",!0),t.send()}</script>\n"
    "</head>\n"
    "<body onload=configure()>\n"
    "<h1>Sensor</h1>\n"
    "<h2>Last Minute</h2>\n"
    "<div class=grid>\n"
    "<label for=lastMin>Last Minute</label>\n"
    "<canvas id=lastMin> </canvas>\n"
    "</div>\n"
    "<h2>Previous 24hrs</h2>\n"
    "<div class=grid>\n"
    "<label for=hour0>00:00</label> <canvas id=hour0> </canvas>\n"
    "<label for=hour1>01:00</label><canvas id=hour1> </canvas>\n"
    "<label for=hour2>02:00</label><canvas id=hour2> </canvas>\n"
    "<label for=hour3>03:00</label><canvas id=hour3> </canvas>\n"
    "<label for=hour4>04:00</label><canvas id=hour4> </canvas>\n"
    "<label for=hour5>05:00</label><canvas id=hour5> </canvas>\n"
    "<label for=hour6>06:00</label><canvas id=hour6> </canvas>\n"
    "<label for=hour7>07:00</label> <canvas id=hour7> </canvas>\n"
    "<label for=hour8>08:00</label><canvas id=hour8> </canvas>\n"
    "<label for=hour9>09:00</label> <canvas id=hour9> </canvas>\n"
    "<label for=hour10>10:00</label><canvas id=hour10> </canvas>\n"
    "<label for=hour11>11:00</label> <canvas id=hour11> </canvas>\n"
    "<label for=hour12>12:00</label><canvas id=hour12> </canvas>\n"
    "<label for=hour13>13:00</label><canvas id=hour13> </canvas>\n"
    "<label for=hour14>14:00</label><canvas id=hour14> </canvas>\n"
    "<label for=hour15>15:00</label><canvas id=hour15> </canvas>\n"
    "<label for=hour16>16:00</label><canvas id=hour16> </canvas>\n"
    "<label for=hour17>17:00</label><canvas id=hour17> </canvas>\n"
    "<label for=hour18>18:00</label><canvas id=hour18> </canvas>\n"
    "<label for=hour19>19:00</label><canvas id=hour19> </canvas>\n"
    "<label for=hour20>20:00</label><canvas id=hour20> </canvas>\n"
    "<label for=hour21>21:00</label><canvas id=hour21> </canvas>\n"
    "<label for=hour22>22:00</label><canvas id=hour22> </canvas>\n"
    "<label for=hour23>23:00</label><canvas id=hour23> </canvas>\n"
    "</div>\n"
    "</body>\n"
    "</html>";

// Note JSON was 537 bytes.  Allocate 1k for creating json data response
static char output[1024];

// writes JSON ID
char *HistoryWebapp::id(char *pos, const char *name)
{
    *pos++ = '"';
    while (*name)
    {
        *pos++ = *name++;
    }
    *pos++ = '"';
    return pos;
}

// Writes a 60 bit value as hex into JSON
char *HistoryWebapp::value(char *pos, uint64_t value)
{
    const char *digits = "0123456789ABCDEF";
    *pos++ = '"';
    for (int i = 0; i < 15; ++i)
    {
        int idx = (value & 0xF00000000000000) >> (14 * 4);
        *pos++ = digits[idx];
        value <<= 4; // next hex digit
    }
    *pos++ = '"';
    return pos;
}

bool HistoryWebapp::matches(const char *verb, const char *path)
{
    bool accept = false;
    if (strcmp(verb, "GET") == 0)
    {
        accept =
            strncmp(path, "/history", 8) == 0 ||
            strncmp(path, "/historydata", 12) == 0 ||
            false;
        ;
    }
    return accept;
}

void HistoryWebapp::process(HttpRequest &request, HttpResponse &response)
{

    const char *body;

    if (strncmp(request.path(), "/historydata", 12) == 0)
    {
        // Generate JSON object
  
  /*  // Test object 
          body =
            "{ \"lastMin\":\"A45F035D94573AD\","
            "\"hour\":["
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\","
            "\"A45F035D94573AD\""
            "]"
            "}";
*/
        char *pos = output;
        *pos++ = '{';

        pos = id(pos, "lastMin");
        *pos++ = ':';
        pos = value(pos, history.current());
        
        *pos++ = ',';

        pos = id(pos,"hour");
        *pos++ = ':';
        *pos++ = '[';

        for(int i=0; i<24; ++i){
            if(i != 0) *pos++ = ',';
            pos = value(pos, history.past()[i]);
        }

        *pos++ = ']';

        *pos++ = '}';
        *pos++ = '\0';

        body = output;
    }
    else if (strncmp(request.path(), "/history", 8) == 0)
    {
        body = page;
    }

    response.setStatus(200, "OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Access-Control-Allow-Origin", "*");

    response.setBody(body);
}