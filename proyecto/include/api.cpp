#include "../lib/api.h"
#include "../lib/scanner.h"
#include "../lib/validador.h"
#include "../lib/httplib.h"
#include "../lib/json.hpp"
#include <iostream>
#include <vector>

using json = nlohmann::json;
using namespace std;

Api::Api() {}

string Api::procesarComando(string comando)
{
    scanner sc;
    Validador validador;
    json respuesta;

    string tk = sc.token(comando);
    string resto = comando;
    if (resto.length() > tk.length())
    {
        resto.erase(0, tk.length() + 1);
    }
    else
    {
        resto = "";
    }
    vector<string> tks = sc.split_tokens(resto);

    vector<string> errores;
    string tkUpper = sc.upper(tk);

    if (sc.compare(tk, "MKDISK")) errores = validador.validarMkdisk(tks);
    else if (sc.compare(tk, "RMDISK")) errores = validador.validarRmdisk(tks);
    else if (sc.compare(tk, "FDISK")) errores = validador.validarFdisk(tks);
    else if (sc.compare(tk, "MOUNT")) errores = validador.validarMount(tks);
    else if (sc.compare(tk, "MKFS")) errores = validador.validarMkfs(tks);
    else if (sc.compare(tk, "MKUSR")) errores = validador.validarMkusr(tks);
    else if (sc.compare(tk, "RMUSR")) errores = validador.validarRmusr(tks);
    else if (sc.compare(tk, "MKFILE")) errores = validador.validarMkfile(tks);
    else
    {
        respuesta["comando"] = tk;
        respuesta["valido"] = false;
        respuesta["errores"] = json::array({"COMANDO INVALIDO \"" + tk + "\""});
        return respuesta.dump();
    }

    respuesta["comando"] = tkUpper;
    respuesta["valido"] = errores.empty();
    respuesta["errores"] = errores;
    return respuesta.dump();
}

void Api::iniciar(int puerto)
{
    httplib::Server svr;

    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    svr.Options(".*", [](const httplib::Request &, httplib::Response &res) {
        res.status = 200;
    });

    svr.Post("/analizar", [this](const httplib::Request &req, httplib::Response &res) {
        try
        {
            json body = json::parse(req.body);
            string comando = body.at("comando").get<string>();
            string resultado = procesarComando(comando);
            res.set_content(resultado, "application/json");
        }
        catch (exception &e)
        {
            json error;
            error["error"] = "JSON invalido o falta el campo 'comando'";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
        }
    });

    cout << "Servidor escuchando en el puerto " << puerto << "..." << endl;
    svr.listen("0.0.0.0", puerto);
}