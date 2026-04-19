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
    bool split = false;
    int newh = 0;

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
        else if ((strncmp(argv[2], "--generate", 10) == 0) || (strncmp(argv[2], "generate", 8) == 0)) {
			string generaParam = argv[2];
            generate = true;
            if (generaParam.find("--", 0) == 0)
                generaParam.erase(0, 10);
            else
				generaParam.erase(0, 8);
            if (generaParam.find("2", 0) == 0) {
                split = true;
                generaParam.erase(0, 1);
			}
            if (generaParam.find("h=", 0) == 0) {
                generaParam.erase(0, 2);
                try {
                    newh = stoi(generaParam);
					ok = true;
                }
                catch (const std::invalid_argument& e) {
                    cerr << "Error: h=pixels_nueva_altura no reconocido (debe ser numérico)" << endl;
                    return 1;
                }
            }
            else
                ok = generaParam.empty();
        }
    }
    if (!ok) {
        cerr << "Formato: phaBloques fichero.PhA struct | tipo n | extract n | generate[2][h=pixels]" << endl;
		cerr << "  struct: muestra la estructura de los bloques sin mostrar su contenido" << endl;
		cerr << "  tipo n: muestra el contenido de los bloques del tipo n" << endl;
		cerr << "  extract n: extrae el contenido del bloque con numBloq n (si es posible)" << endl;
		cerr << "  generate: genera un JPG con las páginas (si hay bloques tipo 3) en el mismo directorio con el mismo nombre que el fichero sin extensión" << endl;
		cerr << "  generate2: dividide JPG en dos mitades para cada página excepto portada" << endl;
        cerr << "  h=pixels ajusta la aoltura de las páginas a pixels duplicando las partes superior e inferior de las imagenes en las bandas adicionales" << endl;
        return 1;
    }

    PhA pha(&file);
    pha.printResumen(tipo, extract);

    if (generate) {
        string outDir = getOutputDir(argv[1]);
        createDir(outDir);
        pha.generate(outDir, true, split, newh);
    }

    if (PhA::buffer) delete[] PhA::buffer;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

/*
* Tipos bloque 8 x página 2-9 alguna mas
*              4     x marco
*/
