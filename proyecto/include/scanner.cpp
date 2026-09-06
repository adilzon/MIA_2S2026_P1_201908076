#include "../lib/scanner.h"
#include "../lib/validador.h"  // NUEVO INCLUDE
#include <iostream>
#include <stdlib.h>
#include <locale>
#include <cstring>
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;

bool logued = false;
scanner::scanner()
{
}
void Clear()
{
    cout << "\x1B[2J\x1B[H";
}

void scanner::start()
{
    system("clear");
    cout << "\033[1;92m";

    cout << R"(

            .--.
           |o_o |
           |:_/ |
          //   \ \
         (|     | )
        /'\_   _/`\
        \___)=(___/

        Linux Practica - ADILZON

)";

        cout << "\033[47m"; 
    cout << "\033[32m"; 

    cout << "====================================" << endl;
    cout << "                                    " << endl;
    cout << "          [1] Ingrese Comando       " << endl;
    cout << "          [2] exit                  " << endl;
    cout << "                                    " << endl;
    cout << "====================================" << endl;

    cout << "\033[0m";
    cout << "\033[32m[(execute)]>: \033[0m ";
    
    while (true)
    {
        string texto;
        getline(cin, texto);
        Clear();
        if (compare(texto, "exit"))
        {
            break;
        }
        string tk = token(texto);
        texto.erase(0, tk.length() + 1); // borrar los comentarios de  los archivos de entrada
        vector<string> tks = split_tokens(texto);
        functions(tk, tks);
        cout << "\033[30;43mPRESIONAR ENTER <--|\033[0m" << endl;
        getline(cin, texto);
        Clear();
                cout << "\033[47m"; // Fondo gris claro
    cout << "\033[32m"; // Texto verde

    cout << "====================================" << endl;
    cout << "                                    " << endl;
    cout << "          [1] Ingrese Comando       " << endl;
    cout << "          [2] exit                  " << endl;
    cout << "                                    " << endl;
    cout << "====================================" << endl;

    // Restaurar colores normales
    cout << "\033[0m";
    cout << "\033[32m[(execute)]>: \033[0m ";
    
    }
}

void scanner::functions(string token, vector<string> tks)
{
    if (compare(token, "MKDISK"))
    {
        Validador validador;
        vector<string> errores_mkdisk = validador.validarMkdisk(tks);
        if (errores_mkdisk.empty())
        {
            respuesta("MKDISK", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_mkdisk)
            {
                errores("MKDISK", e);
            }
        }
    }
    else if (compare(token, "RMDISK"))
    {
        Validador validador;
        vector<string> errores_rmdisk = validador.validarRmdisk(tks);
        if (errores_rmdisk.empty())
        {
            respuesta("RMDISK", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_rmdisk)
            {
                errores("RMDISK", e);
            }
        }
    }
    else if (compare(token, "FDISK"))
    {
        Validador validador;
        vector<string> errores_fdisk = validador.validarFdisk(tks);
        if (errores_fdisk.empty())
        {
            respuesta("FDISK", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_fdisk)
            {
                errores("FDISK", e);
            }
        }
    }
    else if (compare(token, "MOUNT"))
    {
        Validador validador;
        vector<string> errores_mount = validador.validarMount(tks);
        if (errores_mount.empty())
        {
            respuesta("MOUNT", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_mount)
            {
                errores("MOUNT", e);
            }
        }
    }
    else if (compare(token, "MKFS"))
    {
        Validador validador;
        vector<string> errores_mkfs = validador.validarMkfs(tks);
        if (errores_mkfs.empty())
        {
            respuesta("MKFS", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_mkfs)
            {
                errores("MKFS", e);
            }
        }
    }
    else if (compare(token, "MKUSR"))
    {
        Validador validador;
        vector<string> errores_mkusr = validador.validarMkusr(tks);
        if (errores_mkusr.empty())
        {
            respuesta("MKUSR", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_mkusr)
            {
                errores("MKUSR", e);
            }
        }
    }
    else if (compare(token, "RMUSR"))
    {
        Validador validador;
        vector<string> errores_rmusr = validador.validarRmusr(tks);
        if (errores_rmusr.empty())
        {
            respuesta("RMUSR", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_rmusr)
            {
                errores("RMUSR", e);
            }
        }
    }
    else if (compare(token, "MKFILE"))
    {
        Validador validador;
        vector<string> errores_mkfile = validador.validarMkfile(tks);
        if (errores_mkfile.empty())
        {
            respuesta("MKFILE", "Comando valido. Parametros correctos.");
        }
        else
        {
            for (string e : errores_mkfile)
            {
                errores("MKFILE", e);
            }
        }
    }
    else
    {
        errores("SYSTEM", "COMANDO INVALIDO \"" + token + "\"");
    }
}

string scanner::token(string text)
{
    string tkn = "";
    bool terminar = false;
    for (char &c : text)
    {
        if (terminar)
        {
            if (c == ' ' || c == '-')
            {
                break;
            }
            tkn += c;
        }
        else if ((c != ' ' && !terminar))
        {
            if (c == '#')
            {
                tkn = text;
                break;
            }
            else
            {
                tkn += c;
                terminar = true;
            }
        }
    }
    return tkn;
}

vector<string> scanner::split(string text, string text_split)
{
    vector<string> cadena;
    if (text.empty())
    {
        return cadena;
    }

    int n = text.length();
    char char_array[n + 1];
    strcpy(char_array, text.c_str());
    char *point = strtok(char_array, text_split.c_str());
    while (point != NULL)
    {
        cadena.push_back(string(point));
        point = strtok(NULL, text_split.c_str());
    }
    return cadena;
}

vector<string> scanner::split_tokens(string text)
{
    vector<string> tokens;
    if (text.empty())
    {
        return tokens;
    }
    text.push_back(' '); // centinela para simplificar el final del texto

    size_t i = 0;
    size_t n = text.length();

    while (i < n)
    {
        // Saltar espacios en blanco
        while (i < n && text[i] == ' ') i++;
        if (i >= n) break;

        // Si encontramos un comentario, ignoramos el resto de la línea
        if (text[i] == '#') break;

        // Todo parámetro debe empezar con '-'
        if (text[i] != '-')
        {
            // Carácter inesperado (formato incorrecto): saltamos hasta el siguiente espacio
            while (i < n && text[i] != ' ') i++;
            continue;
        }
        i++; // saltar el '-'

        // Leer el nombre del parámetro
        string nombre = "";
        while (i < n && text[i] != '=' && text[i] != ' ')
        {
            nombre += text[i];
            i++;
        }

        // CASO MODIFICADO: parámetro sin '=' (puede ser una bandera como "-r", o un error)
        // Ya no le agregamos el '=' automáticamente, dejamos que el Validador decida si es una bandera válida o un error
        if (i < n && text[i] == ' ')
        {
            tokens.push_back(nombre); // sin '=' — se decide en parsearParametros
            continue;
        }

        // Caso: llegó fin de texto sin '=' (ej. "-r" al final de la línea)
        if (i >= n)
        {
            tokens.push_back(nombre);
            break;
        }

        // Aquí text[i] == '='
        i++; // saltar el '='
        string valor = "";

        // BLOQUE MODIFICADO: detección de comillas sin cerrar
        if (i < n && text[i] == '"')
        {
            // Valor entre comillas: puede contener espacios
            i++; // saltar comilla de apertura
            while (i < n && text[i] != '"')
            {
                valor += text[i];
                i++;
            }
            if (i < n && text[i] == '"')
            {
                i++; // saltar comilla de cierre
            }
            else
            {
                // No se encontró la comilla de cierre: parámetro mal formado
                tokens.push_back("__ERROR_COMILLA__=" + nombre);
                break; // el resto de la línea ya no es confiable, dejamos de leer
            }
        }
        else
        {
            // Valor sin comillas: permitimos un '-' inicial (numeros negativos, ej. -size=-5)
            if (i < n && text[i] == '-')
            {
                valor += text[i];
                i++;
            }
            // El valor termina en espacio o cuando empieza el siguiente parámetro ('-')
            while (i < n && text[i] != ' ' && text[i] != '-')
            {
                valor += text[i];
                i++;
            }
        }

        tokens.push_back(nombre + "=" + valor);
    }

    return tokens;
}

string scanner::upper(string a)
{
    string up = "";
    for (char &a : a)
    {
        up += toupper(a);
    }
    return up;
}

bool scanner::compare(string a, string b)
{
    if (upper(a) == upper(b))
    {
        return true;
    }
    return false;
}

void scanner::errores(string operacion, string mensaje)
{

    cout << "\033[1;41m Error\033" << "\033[0;31m(" + operacion + ")~~> \033[0m" << mensaje << endl;
}

void scanner::respuesta(string operacion, string mensaje)
{

    cout << "\033[0;42m(" + operacion + ")~~> \033[0m" << mensaje << endl;
}

bool scanner::confirmar(string mensaje)
{
    cout << mensaje << "[S/N]" << endl;
    string respuesta;
    getline(cin, respuesta);
    if (compare(respuesta, "s"))
    {
        return true;
    }
    return false;
}

void scanner::funcion_excec(vector<string> tokens)
{
    string path = "";
    for (string token : tokens)
    {
        string tk = token.substr(0, token.find("="));
        token.erase(0, tk.length() + 1);
        if (compare(tk, "path"))
        {
            path = token;
        }
    }
    if (path.empty())
    {
        errores("EXEC", " path no ingresado ");
        return;
    }
    excec(path);
}

void scanner::excec(string path)
{
    string filename(path);
    vector<string> lines;
    string line;
    ifstream input_file(filename);
    if (!input_file.is_open())
    {
        cerr << "ERROR al abrir archivo" << filename << endl;
        return;
    }
    while (getline(input_file, line))
    {
        lines.push_back(line);
    }
    for (const auto &i : lines)
    {
        string texto = i;
        string tk = token(texto);
        if (texto != "")
        {
            if (compare(texto, "PAUSE"))
            {
                string pause;
                respuesta("PAUSE", "Presionar enter....");
                getline(cin, pause);
                continue;
            }
            texto.erase(0, tk.length() + 1);
            vector<string> tks = split_tokens(texto);
            functions(tk, tks);
        }
    }
    input_file.close();
    return;
}