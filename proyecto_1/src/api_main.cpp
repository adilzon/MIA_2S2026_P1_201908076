#include <iostream>
#include <fstream>
#include <sstream>
#include "../lib/httplib.h"
#include "../lib/shared.h"
#include "../lib/scanner.h"

using namespace std;

string mimeTypeFor(const string &path) {
    size_t dot = path.find_last_of('.');
    string ext = (dot == string::npos) ? "" : path.substr(dot + 1);

    for (auto &c : ext)
        c = tolower(c);

    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";

    if (ext == "png")
        return "image/png";

    if (ext == "svg")
        return "image/svg+xml";

    if (ext == "pdf")
        return "application/pdf";

    if (ext == "txt")
        return "text/plain; charset=utf-8";

    return "application/octet-stream";
}

bool isSafePath(const string &path) {
    if (path.find("..") != string::npos)
        return false;

    if (path.empty() || path[0] != '/')
        return false;

    return true;
}

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

    // File serving endpoint
    svr.Get("/api/file", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        if (!req.has_param("path")) {
            res.status = 400;
            res.set_content("falta el parametro path", "text/plain");
            return;
        }

        string path = req.get_param_value("path");

        if (!isSafePath(path)) {
            res.status = 403;
            res.set_content("ruta no permitida", "text/plain");
            return;
        }

        ifstream file(path, ios::binary);

        if (!file.is_open()) {
            res.status = 404;
            res.set_content("archivo no encontrado", "text/plain");
            return;
        }

        ostringstream buffer;
        buffer << file.rdbuf();

        file.close();

        res.set_content(
            buffer.str(),
            mimeTypeFor(path).c_str()
        );
    });

    cout << "API escuchando en http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
