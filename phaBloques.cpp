using namespace std;

#include "bloquePhA.h"
#include <iostream>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

string getOutputDir(const string& path)
{
    // Buscar la última barra (Windows o Linux)
    size_t slash = path.find_last_of("/\\");

    // Buscar la última extensión
    size_t dot = path.find_last_of('.');

    std::string folder;
    std::string name;

    if (slash != string::npos)
        folder = path.substr(0, slash + 1);
    else
        folder = "";

    if (dot != string::npos && ((slash == string::npos) || (dot > slash)))
        name = path.substr(slash + 1, dot - slash - 1);
    else
        name = path.substr(slash + 1);

    return folder + name;
}

void createDir(const string& dir)
{
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0755);
#endif
}

int main(int argc, char* argv[])
{
    cout << "phaBloques decoder" << endl;

    ifstream file;
    bool ok = false;

    if (argc > 1) {
        file.open(argv[1], std::ios::binary);
        if (!file) {
            cerr << "Error: no se pudo abrir el archivo" << endl;
            return 1;
        }
    }

    int tipo = -1; // todos
    int extract = -1;
    bool generate = false;

    if (argc == 4) {
        if ((strcmp(argv[2], "--tipo") == 0) || (strcmp(argv[2], "tipo") == 0)) {
            try {
                tipo = stoi(argv[3]);
                ok = true;
            }
            catch (const std::invalid_argument& e) {
                cerr << "Error: tipo no reconocido (debe ser numérico)" << endl;
                return 1;
            }
        }
        else
        if ((strcmp(argv[2], "--extract") == 0) || (strcmp(argv[2], "extract") == 0)) {
            try {
                extract = stoi(argv[3]);
                ok = true;
            }
            catch (const std::invalid_argument& e) {
                cerr << "Error: bloque no reconocido (debe ser numérico)" << endl;
                return 1;
            }
        }
    }
    else if (argc == 3) {
        if ((strcmp(argv[2], "--struct") == 0) || (strcmp(argv[2], "struct") == 0)) {
            ok = true;
        }
        else if ((strcmp(argv[2], "--generate") == 0) || (strcmp(argv[2], "generate") == 0)) {
            generate = true;
            ok = true;
        }
    }
    if (!ok) {
        cerr << "Formato: phaBloques fichero.PhA struct | tipo n | extract n | generate" << endl;
        return 1;
    }

    PhA pha(&file);
    pha.printResumen(tipo, extract);

    if (generate) {
        string outDir = getOutputDir(argv[1]);
        createDir(outDir);
        pha.generate(outDir, true);
    }

    if (PhA::buffer) delete[] PhA::buffer;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

/*
* Tipos bloque 8 x página 2-9 alguna mas
*              4     x marco
*/
