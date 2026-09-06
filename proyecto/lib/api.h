#ifndef API_H
#define API_H
#include <string>
using namespace std;

class Api
{
public:
    Api();
    void iniciar(int puerto);
    string procesarComando(string comando);
};
#endif