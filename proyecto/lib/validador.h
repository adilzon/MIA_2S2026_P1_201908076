#ifndef VALIDADOR_H
#define VALIDADOR_H
#include <vector>
#include <string>
using namespace std;

struct Parametro
{
    string nombre;
    string valor;
};

class Validador
{
public:
    Validador();
    vector<Parametro> parsearParametros(vector<string> tokens, vector<string> &errores);
    string obtenerValor(vector<Parametro> params, string nombre);
    bool existeParametro(vector<Parametro> params, string nombre);
    bool esNumero(string valor);
    bool longitudValida(string valor, int maximo);
    vector<string> validarMkdisk(vector<string> tokens);
    vector<string> validarRmdisk(vector<string> tokens);
    vector<string> validarRmusr(vector<string> tokens);
    vector<string> validarMkusr(vector<string> tokens);
    vector<string> validarMkfs(vector<string> tokens);
    vector<string> validarMount(vector<string> tokens);
    vector<string> validarMkfile(vector<string> tokens);
    vector<string> validarFdisk(vector<string> tokens);
    void validarParametrosConocidos(vector<Parametro> params, vector<string> validos, vector<string> &errores);
};
#endif