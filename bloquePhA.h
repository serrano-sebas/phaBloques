#pragma once

#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "FreeImage.h"

using namespace std;

constexpr float PI = 3.14159265358979323846f; 

struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;

    void print();
};

struct SizeRect {
    int32_t width;
    int32_t height;

    void print();
};

class BloquePhA;

class DetailPhA {
public:
    BloquePhA* bloque;
    DetailPhA(BloquePhA* _bloque) : bloque(_bloque) {};
    virtual ~DetailPhA() = default;
    virtual void printDetail() {};
    virtual void extract(bool tofile) {};
};

// Tipo 1
class DetailPhAHeader : public DetailPhA {
public:
    int32_t nPags;
    DetailPhAHeader(BloquePhA* _bloque);
    virtual void printDetail();
};

// Tipo 2
class DetailPhAThumb : public DetailPhA {
public:
    int32_t blqPags;
    int32_t blqElems;
    DetailPhAThumb(BloquePhA* _bloque);
    virtual void printDetail();
    virtual void extract(bool tofile);
};

// Tipo 3
class DetailPhAPag : public DetailPhA {
public:
    int nPag; // -1,-2 cubiertas, -3 interior cubierta primera (solo pag dcha), -4 interior contraportada ultima (solo pag izda)
    Rect coord; // coordenadas de la pagina en el conjunto de las dos paginas que se muestran en el editor

    DetailPhAPag(BloquePhA* _bloque);
    virtual void printDetail();
};

// Tipo 4
class DetailPhAImagen : public DetailPhA {
public:
    string nombre;
    Rect coord;
    SizeRect sizeRect;
    SizeRect sizeRotated;
    vector<int32_t> unknow;
    int tipoImage; // 0=RAW 1=TIF
    streamsize sizeImage;
    streampos posImage;
    int32_t rotImage;
    int32_t blqSombra;
    int32_t blqAux1;
    int32_t blqAux2;

    DetailPhAImagen(BloquePhA* _bloque);
    virtual void printDetail();
    virtual void extract(bool tofile);
};

// Tipo 7
class DetailPhAIndexThumb : public DetailPhA {
public:
    vector<int> bloques;

    DetailPhAIndexThumb(BloquePhA* _bloque);
    virtual void printDetail();
};

// Tipo 8
class DetailPhAPagina : public DetailPhA {
public:
    vector<int> bloques;

    DetailPhAPagina(BloquePhA* _bloque);
    virtual void printDetail();
};

// Tipo 9
class DetailPhAIndex : public DetailPhA {
public:
    vector<int> bloques;

    DetailPhAIndex(BloquePhA* _bloque);
    virtual void printDetail();
};

// Tipo 19
class DetailPhASombra : public DetailPhA {
public:
    int32_t opacidad;
    int32_t angulo;
    int32_t distancia;
    int32_t tamanno;
    int32_t color;

    DetailPhASombra(BloquePhA* _bloque);
    virtual void printDetail();
};

class BloquePhA {
public:
    ifstream* file;
    streampos pos;
    int32_t numBloq;
    int32_t data2;
    int tipo;
    int stipo;
    unique_ptr<DetailPhA> detail;

    streamsize size;

    BloquePhA(ifstream* file, streampos _pos);
    virtual ~BloquePhA();
    streamsize readBloque();
    void printBloque();
    const char* printTipo();
    void printResumen();
};

class PhA {
public:
    static char* buffer;
    static streamsize length;
    static char* checkBuffer(streamsize size);
    static void printBloqueHexa(unsigned char* punt, int size);
    static void printBloqueChar(unsigned char* punt, int size);
    static void printBloqueLong(int32_t* punt, int size);
    static void printError(int numBloque, const char* texto);

    std::map<int, unique_ptr<BloquePhA>> pha;
    ifstream* file;
    size_t sizePha;
    char firma[9];
    vector<int32_t> headData;

    PhA(ifstream* _file);
    virtual ~PhA();
    streampos readCabecera();
    void printResumen(int tipo, int extract);
    void generate(string& outDir, bool verbose, bool split, int newh);

private:
    bool generatePag(string& outDir, int nBloqueThumb, bool verbose, bool split, int newh);
    void pasteImageTiff(FIBITMAP* canvas, char* buffer, size_t size, 
        int x, int y, int width, int height);
    void pasteImageRaw(FIBITMAP* canvas, unsigned char* rgbAlpha,
        int width, int height, int posX, int posY);
    FIBITMAP* ExtendVerticalBlur(FIBITMAP* src, int newHeight);
};
