#include <iostream>
#include "../lib/httplib.h"
#include "../lib/shared.h"
#include "../lib/scanner.h"

using namespace std;

int main()
{
    API_MODE = true;
    scanner scan;
    httplib::Server svr;

    // Preflight CORS handler
    svr.Options(R"(.*)", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    // Health check endpoint
    svr.Get("/api/health", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_content("ok", "text/plain; charset=utf-8");
    });

    // Execute script endpoint
    svr.Post("/api/execute", [&scan](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        bool confirm = false;
        if (req.has_param("confirm"))
        {
            string c = req.get_param_value("confirm");
            if (c == "true" || c == "1")
            {
                confirm = true;
            }
        }
        API_CONFIRM_ANSWER = confirm;

        string script = req.body;
        string output = scan.executeScript(script);

        res.set_content(output, "text/plain; charset=utf-8");
    });

    cout << "API escuchando en http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
