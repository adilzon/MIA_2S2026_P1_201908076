#include "../lib/validador.h"
#include <cctype>

using namespace std;

Validador::Validador() {}

// Convierte cada string "nombre=valor" en un struct Parametro,
// quitando las comillas si el valor las trae.
vector<Parametro> Validador::parsearParametros(vector<string> tokens, vector<string> &errores)
{
    vector<Parametro> params;
    for (string tk : tokens)
    {
        size_t pos = tk.find("=");
        if (pos == string::npos)
        {
            errores.push_back("Parametro mal formado (falta '='): \"-" + tk + "\"");
            continue;
        }
        string nombre = tk.substr(0, pos);
        string valor = tk.substr(pos + 1);

        if (!valor.empty() && valor.front() == '"') valor.erase(0, 1);
        if (!valor.empty() && valor.back() == '"') valor.pop_back();

        Parametro p;
        p.nombre = nombre;
        p.valor = valor;
        params.push_back(p);
    }
    return params;
}

string Validador::obtenerValor(vector<Parametro> params, string nombre)
{
    for (Parametro p : params)
    {
        string a = p.nombre, b = nombre;
        for (char &c : a) c = toupper(c);
        for (char &c : b) c = toupper(c);
        if (a == b) return p.valor;
    }
    return "";
}

bool Validador::existeParametro(vector<Parametro> params, string nombre)
{
    for (Parametro p : params)
    {
        string a = p.nombre, b = nombre;
        for (char &c : a) c = toupper(c);
        for (char &c : b) c = toupper(c);
        if (a == b) return true;
    }
    return false;
}

bool Validador::esNumero(string valor)
{
    if (valor.empty()) return false;
    size_t inicio = 0;
    if (valor[0] == '-') inicio = 1; // permitimos detectar el signo para dar mejor error luego
    if (inicio == valor.length()) return false;
    for (size_t i = inicio; i < valor.length(); i++)
    {
        if (!isdigit(valor[i])) return false;
    }
    return true;
}

// Función de apoyo para validar longitud máxima
bool Validador::longitudValida(string valor, int maximo)
{
    return (int)valor.length() <= maximo;
}

void Validador::validarParametrosConocidos(vector<Parametro> params, vector<string> validos, vector<string> &errores)
{
    vector<string> vistos; // Para detectar duplicados

    for (Parametro p : params)
    {
        string nombreUpper = p.nombre;
        for (char &c : nombreUpper) c = toupper(c);

        bool esValido = false;
        for (string v : validos)
        {
            string vUpper = v;
            for (char &c : vUpper) c = toupper(c);
            if (nombreUpper == vUpper) { esValido = true; break; }
        }

        if (!esValido)
        {
            errores.push_back("Parametro desconocido: \"-" + p.nombre + "\"");
        }

        // Revisar si ya lo habíamos visto (duplicado)
        bool yaVisto = false;
        for (string v : vistos)
        {
            if (v == nombreUpper) { yaVisto = true; break; }
        }
        if (yaVisto)
        {
            errores.push_back("Parametro repetido: \"-" + p.nombre + "\"");
        }
        else
        {
            vistos.push_back(nombreUpper);
        }
    }
}

vector<string> Validador::validarMkdisk(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"size", "fit", "unit", "path"}, errores);

    // -size: obligatorio, numero positivo mayor que 0
    if (!existeParametro(params, "size"))
    {
        errores.push_back("Falta el parametro obligatorio -size");
    }
    else
    {
        string valorSize = obtenerValor(params, "size");
        if (!esNumero(valorSize))
        {
            errores.push_back("El valor de -size debe ser numerico: \"" + valorSize + "\"");
        }
        else if (stoi(valorSize) <= 0)
        {
            errores.push_back("El valor de -size debe ser mayor que 0");
        }
    }

    // -path: obligatorio
    if (!existeParametro(params, "path"))
    {
        errores.push_back("Falta el parametro obligatorio -path");
    }

    // -fit: opcional, valores permitidos BF, FF, WF
    if (existeParametro(params, "fit"))
    {
        string valorFit = obtenerValor(params, "fit");
        string up = valorFit;
        for (char &c : up) c = toupper(c);
        if (up != "BF" && up != "FF" && up != "WF")
        {
            errores.push_back("El valor de -fit no es valido: \"" + valorFit + "\" (debe ser BF, FF o WF)");
        }
    }

    // -unit: opcional, valores permitidos K, M
    if (existeParametro(params, "unit"))
    {
        string valorUnit = obtenerValor(params, "unit");
        string up = valorUnit;
        for (char &c : up) c = toupper(c);
        if (up != "K" && up != "M")
        {
            errores.push_back("El valor de -unit no es valido: \"" + valorUnit + "\" (debe ser K o M)");
        }
    }

    return errores;
}

vector<string> Validador::validarRmdisk(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"path"}, errores);

    // -path: obligatorio
    if (!existeParametro(params, "path"))
    {
        errores.push_back("Falta el parametro obligatorio -path");
    }

    return errores;
}

vector<string> Validador::validarRmusr(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"user"}, errores);

    // -user: obligatorio
    if (!existeParametro(params, "user"))
    {
        errores.push_back("Falta el parametro obligatorio -user");
    }

    return errores;
}

// Validación completa de MKUSR
vector<string> Validador::validarMkusr(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"user", "pass", "grp"}, errores);

    // -user: obligatorio, maximo 10 caracteres
    if (!existeParametro(params, "user"))
    {
        errores.push_back("Falta el parametro obligatorio -user");
    }
    else
    {
        string valorUser = obtenerValor(params, "user");
        if (!longitudValida(valorUser, 10))
        {
            errores.push_back("El valor de -user excede el maximo de 10 caracteres: \"" + valorUser + "\"");
        }
    }

    // -pass: obligatorio, maximo 10 caracteres
    if (!existeParametro(params, "pass"))
    {
        errores.push_back("Falta el parametro obligatorio -pass");
    }
    else
    {
        string valorPass = obtenerValor(params, "pass");
        if (!longitudValida(valorPass, 10))
        {
            errores.push_back("El valor de -pass excede el maximo de 10 caracteres: \"" + valorPass + "\"");
        }
    }

    // -grp: obligatorio, maximo 10 caracteres
    if (!existeParametro(params, "grp"))
    {
        errores.push_back("Falta el parametro obligatorio -grp");
    }
    else
    {
        string valorGrp = obtenerValor(params, "grp");
        if (!longitudValida(valorGrp, 10))
        {
            errores.push_back("El valor de -grp excede el maximo de 10 caracteres: \"" + valorGrp + "\"");
        }
    }

    return errores;
}

// Validación de MKFS
vector<string> Validador::validarMkfs(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"id", "type"}, errores);

    // -id: obligatorio
    if (!existeParametro(params, "id"))
    {
        errores.push_back("Falta el parametro obligatorio -id");
    }

    // -type: opcional, unico valor permitido "full"
    if (existeParametro(params, "type"))
    {
        string valorType = obtenerValor(params, "type");
        string up = valorType;
        for (char &c : up) c = toupper(c);
        if (up != "FULL")
        {
            errores.push_back("El valor de -type no es valido: \"" + valorType + "\" (debe ser Full)");
        }
    }

    return errores;
}

// Validación de MOUNT
vector<string> Validador::validarMount(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"path", "name"}, errores);

    // -path: obligatorio
    if (!existeParametro(params, "path"))
    {
        errores.push_back("Falta el parametro obligatorio -path");
    }

    // -name: obligatorio
    if (!existeParametro(params, "name"))
    {
        errores.push_back("Falta el parametro obligatorio -name");
    }

    return errores;
}

// Validación de MKFILE
vector<string> Validador::validarMkfile(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"path", "r", "size", "cont"}, errores);

    // -path: obligatorio
    if (!existeParametro(params, "path"))
    {
        errores.push_back("Falta el parametro obligatorio -path");
    }

    // -size: opcional, debe ser numero; si es negativo es error
    if (existeParametro(params, "size"))
    {
        string valorSize = obtenerValor(params, "size");
        if (!esNumero(valorSize))
        {
            errores.push_back("El valor de -size debe ser numerico: \"" + valorSize + "\"");
        }
        else if (stoi(valorSize) < 0)
        {
            errores.push_back("El valor de -size no puede ser negativo");
        }
    }

    // -r y -cont son opcionales, no requieren un formato de valor especial
    return errores;
}

// Validación de FDISK
vector<string> Validador::validarFdisk(vector<string> tokens)
{
    vector<string> errores;
    vector<Parametro> params = parsearParametros(tokens, errores);

    validarParametrosConocidos(params, {"size", "unit", "path", "type", "fit", "name"}, errores);

    // -size: obligatorio, numero positivo mayor que 0
    if (!existeParametro(params, "size"))
    {
        errores.push_back("Falta el parametro obligatorio -size");
    }
    else
    {
        string valorSize = obtenerValor(params, "size");
        if (!esNumero(valorSize))
        {
            errores.push_back("El valor de -size debe ser numerico: \"" + valorSize + "\"");
        }
        else if (stoi(valorSize) <= 0)
        {
            errores.push_back("El valor de -size debe ser mayor que 0");
        }
    }

    // -path: obligatorio
    if (!existeParametro(params, "path"))
    {
        errores.push_back("Falta el parametro obligatorio -path");
    }

    // -name: obligatorio
    if (!existeParametro(params, "name"))
    {
        errores.push_back("Falta el parametro obligatorio -name");
    }

    // -unit: opcional, valores permitidos B, K, M
    if (existeParametro(params, "unit"))
    {
        string valorUnit = obtenerValor(params, "unit");
        string up = valorUnit;
        for (char &c : up) c = toupper(c);
        if (up != "B" && up != "K" && up != "M")
        {
            errores.push_back("El valor de -unit no es valido: \"" + valorUnit + "\" (debe ser B, K o M)");
        }
    }

    // -type: opcional, valores permitidos P, E, L
    if (existeParametro(params, "type"))
    {
        string valorType = obtenerValor(params, "type");
        string up = valorType;
        for (char &c : up) c = toupper(c);
        if (up != "P" && up != "E" && up != "L")
        {
            errores.push_back("El valor de -type no es valido: \"" + valorType + "\" (debe ser P, E o L)");
        }
    }

    // -fit: opcional, valores permitidos BF, FF, WF
    if (existeParametro(params, "fit"))
    {
        string valorFit = obtenerValor(params, "fit");
        string up = valorFit;
        for (char &c : up) c = toupper(c);
        if (up != "BF" && up != "FF" && up != "WF")
        {
            errores.push_back("El valor de -fit no es valido: \"" + valorFit + "\" (debe ser BF, FF o WF)");
        }
    }

    return errores;
}