// BloquePha.cpp : Clase para guardar bloques estructura PhA
//
using namespace std;

#include <iostream>
#include <iomanip>
#include <memory>

#include "bloquePhA.h"

void Rect::print() {
    cout << fixed << setprecision(2) << "[ ("
        << (float)left / 100 << ", " << (float)top / 100 << ") ("
        << (float)right / 100 << ", " << (float)bottom / 100 << ") ]";
};

void SizeRect::print() {
    cout << fixed << setprecision(2)
        << (float)width / 100 << " x " << (float)height / 100;
};

BloquePhA::BloquePhA(ifstream* _file, streampos _pos) : file(_file), pos(_pos) {
    file->seekg(pos);
    file->read(PhA::buffer, 16);
    int32_t* punt = (int32_t*)PhA::buffer;
    numBloq = *(punt++);
    data2 = *(punt++);
    int16_t* ptipo = (int16_t*)(punt++);
    tipo = (int)*(ptipo++);
    stipo = (int)*ptipo;
    size = (streamsize)*punt;

    switch (tipo) {
    case 1:
        detail = make_unique<DetailPhAHeader>(this);
        break;
    case 2:
        detail = make_unique<DetailPhAThumb>(this);
        break;
    case 3:
        detail = make_unique<DetailPhAPag>(this);
        break;
    case 4:
    case 5:
        detail = make_unique<DetailPhAImagen>(this);
        break;
    case 7:
        detail = make_unique <DetailPhAIndexThumb>(this);
        break;
    case 8:
        detail = make_unique <DetailPhAPagina>(this);
        break;
    case 9:
        detail = make_unique <DetailPhAIndex>(this);
        break;
    case 19:
        detail = make_unique <DetailPhASombra>(this);
        break;
    default:
        detail = make_unique <DetailPhA>(this);
    }
}

BloquePhA::~BloquePhA() = default;

streamsize BloquePhA::readBloque() {
    file->seekg(pos + (streamsize)16);
    file->read(PhA::checkBuffer(size), size);
    streamsize fin = size - 1;
    for (; (fin >= 0) && (PhA::buffer[fin] == 0); --fin);
    return fin + 1;
}

void BloquePhA::printBloque() {
    streamsize efSize = readBloque();
    streampos punt = 0;
    while (punt < efSize) {
        cout << endl << setw(48) << setfill(' ') << "";
        PhA::printBloqueHexa((unsigned char*)(PhA::buffer + punt), min(efSize - punt, (streamsize)16));
        PhA::printBloqueChar((unsigned char*)(PhA::buffer + punt), min(efSize - punt, (streamsize)16));
        cout << " ";
        PhA::printBloqueLong((int32_t*)(PhA::buffer + punt), min((efSize - punt + 3) / 4, (streamsize)4));
        punt += 16;
    }
}

const char* BloquePhA::printTipo() {
    switch (tipo) {
    case 1: return "header ";
    case 2: return "thumb  ";
    case 3: return "pag    ";
    case 4: return "imagen ";
    case 5: return "texto  ";
    case 6: return "FotoPrx";
    case 7: return "ix thmb";
    case 8: return "pagina ";
    case 9: return "ix elem";
    case 10:return "img.det";
    case 11:return "FotoPrx";
    case 14:return "msk.det";
    case 19:return "sombra ";
    default: return "       ";
    }
}

void BloquePhA::printResumen() {
    std::cout << hex << setw(8) << setfill('0') << pos << " ";
    if (!numBloq) {
        cout << "Libre";
    }
    else {
        cout << dec << setw(8) << setfill(' ') << numBloq
            << (data2 == 1 ? "   " : " ! ");
        cout << dec << setw(5) << tipo << "/" << setw(5) << stipo << " ";
        cout << printTipo();
        cout << dec << setw(8) << setfill(' ') << size << " ";
        detail->printDetail();
    }
}

void PhA::printError(int numBloque, const char* texto) {
    cout << endl << "ERROR bloque " << numBloque << ": " << texto << endl;
}

// Tipo 1. Cabecera book con tamaño y numero paginas
// Ejemplo 30 paginas: 23 / 210 / 270 / 254 / 254 / 2100 / 2700 / 1 / 0 / 20 / 100 / 10 / 0 / 0 / 130 / 0 / 5 / 0 / 0 / 0 / 0 / 0 / 30 (paginas) / 2
// Ejemplo 20 paginas: 23 / 210 / 270 / 254 / 254 / 2100 / 2700 / 1 / 0 / 20 / 100 / 10 / 0 / 0 / 110 / 0 / 5 / 0 / 0 / 0 / 0 / 0 / 20 (paginas) / 2
// Ejemplo 60 paginas: 23 / 210 / 270 / 254 / 254 / 2100 / 2700 / 1 / 0 / 20 / 100 / 10 / 0 / 0 / 190 / 0 / 5 / 0 / 0 / 0 / 0 / 0 / 60 (paginas) / 2

DetailPhAHeader::DetailPhAHeader(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t ndata;
    bloque->file->read((char*)&ndata, 4); // num datos del bloque (dummy)
    if (ndata != 23) // assert
        PhA::printError(bloque->numBloq, "Num datos bloque (pos.1) no esperado (23)");
    bloque->file->read(PhA::buffer, ndata * 4); // datos pagina
    nPags = *(int32_t*)(PhA::buffer + 84); // pos.22
}

void DetailPhAHeader::printDetail() {
    cout << "Num.pags: " << dec << setw(0) << nPags;
}

// Tipo 2. Miniatura de la página definida en el bloque this - 2
DetailPhAThumb::DetailPhAThumb(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t sizeThumb;
    bloque->file->seekg(bloque->pos + (streamsize)72); // size antes de imagen
    bloque->file->read((char*)&sizeThumb, 4);
    bloque->file->seekg(sizeThumb + 4, ios::cur); // saltamos la imagen + primer valor que es 1 o 7 para pags especiales o normales
    bloque->file->read((char *)&blqPags, 4);
    bloque->file->read((char*)&blqElems, 4);
}

void DetailPhAThumb::printDetail() {
    cout << dec << setw(0) << "Bloque pags:" << blqPags << " elems: " << blqElems;
}

void DetailPhAThumb::extract(bool tofile) {
    int32_t width, height;
    bloque->file->seekg(bloque->pos + (streamsize)96);
    bloque->file->read((char*)&width, 4);
    bloque->file->read((char*)&height, 4);
    streamsize sizeThumb = width * height * 3;
    bloque->file->seekg(bloque->pos + (streamsize)152);
    bloque->file->read(PhA::checkBuffer(sizeThumb), sizeThumb);
    if (tofile) {
        // Guardar como PPM
        string nombre = "miniatura_" + to_string(bloque->numBloq) + ".ppm";
        ofstream out(nombre, ios::binary);

        out << "P6\n" << width << " " << height << "\n255\n";
        // Intercambiar BGR ? RGB
        for (size_t i = 0; i < sizeThumb; i += 3) {
            char b = PhA::buffer[i];
            char g = PhA::buffer[i + 1];
            char r = PhA::buffer[i + 2];
            out.put(g);
            out.put(b);
            out.put(r);
        }
    }
}

// Tipo 3. Páginas individuales apuntados desde bloques tipo 8
// Resto datos no averiguados: 100 / 100 / 100 / 100 / 0 / 0 / 0 / 2
//                             %1
//                             1 / 1 / 0 / 10 / 5
//                             Arial
//                             0 / 0 / 0 / 2 en pp=-1, 1 en pp=-1 3 en pp=-3 4 en pp normales 5 en pp=-4

DetailPhAPag::DetailPhAPag(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t pag;
    bloque->file->read((char*)&pag, 4); // num bloques definidos
    bloque->file->read((char*)&coord, 16); // coordenadas pagina
    nPag = pag;
}

void DetailPhAPag::printDetail() {
    cout << "Num.pag: " << dec << setw(0) << nPag;
    cout << " coord ";
    coord.print();
}

// Tipo 4. Elemento imagen
DetailPhAImagen::DetailPhAImagen(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t sizeName;
    bloque->file->read((char*)&sizeName, 4); // size nombre archivo
    nombre.resize(sizeName, '\0');
    bloque->file->read(&nombre[0], sizeName); // coordenadas pagina
    bloque->file->read((char*)&coord, 16);
    bloque->file->read((char*)&sizeRect, 8);
    bloque->file->read(PhA::buffer, 20);
    rotImage = *(int32_t*)PhA::buffer;
    for (int i = 1; i < 5; i++)
        unknow.push_back(*((int32_t*)(PhA::buffer)+i));
    bloque->file->read((char*)&sizeImage, 4);
    posImage = bloque->file->tellg();
    char startImage[3]; startImage[2] = 0;
    bloque->file->read(startImage, 2);
    if (strcmp(startImage, "TI") == 0) {
        tipoImage = 0;
        bloque->file->seekg(18, ios::cur); // resto firma TIEBITMAPRAW + coord origen (0,0) siempre
        bloque->file->read((char*)&sizeRotated.width, 4);
        bloque->file->read((char*)&sizeRotated.height, 4);
    }
    else if (strcmp(startImage, "II") == 0)
        tipoImage = 1;
    else
        tipoImage = -1;
    bloque->file->seekg(posImage + sizeImage + (streampos)4); // prefijo 0x3C
    bloque->file->read((char*)&blqSombra, 4);
    bloque->file->read(PhA::buffer, 1); // dummy
    bloque->file->read((char*)&blqAux1, 4);
    bloque->file->read((char*)&blqAux2, 4);
}

void DetailPhAImagen::extract(bool tofile) {
    int partialSize, maskSize;
    switch (tipoImage) {
    case 0: // Raw
        bloque->file->seekg(posImage + (streampos) 75);
        bloque->file->read(PhA::checkBuffer(sizeImage), sizeImage);
        if (tofile) {
            // Guardar como PPM
            string nombre = "imagen_" + to_string(bloque->numBloq) + ".ppm";
            string maskNombre = "imagen_" + to_string(bloque->numBloq) + "_mask.ppm";
            ofstream out(nombre, ios::binary);
            ofstream outm(maskNombre, ios::binary);

            out << "P6\n" << sizeRotated.width << " " << sizeRotated.height << "\n255\n";
            outm << "P5\n" << sizeRotated.width << " " << sizeRotated.height << "\n255\n";
            // Intercambiar BGR ? RGB
            size_t i = 0;
            for (int iy = 0; iy < sizeRotated.height; iy++) {
                for (int ix = 0; ix < sizeRotated.width; ix++) {
                    char b = PhA::buffer[i++];
                    char g = PhA::buffer[i++];
                    char r = PhA::buffer[i++];
                    out.put(r);
                    out.put(g);
                    out.put(b);
                }
                i = ((i + 3) / 4) * 4; // frontera de linea en LONG
            }
            for (int iy = 0; iy < sizeRotated.height; iy++) {
                for (int ix = 0; ix < sizeRotated.width; ix++) {
                    outm.put(PhA::buffer[i++]);
                }
                i = ((i + 3) / 4) * 4; // frontera de linea en LONG
            }
        }
        break;
    case 1: // Tiff
        bloque->file->seekg(posImage);
        bloque->file->read(PhA::checkBuffer(sizeImage + 8 ), sizeImage);
        // espacio adicional por si hay que corregir formato

        int32_t pIdf = *(int32_t*)(PhA::buffer + 4); // IFDs
        int16_t nIdf = *(int16_t*)(PhA::buffer + pIdf); pIdf += 2;
        int32_t bitsPerSample = -1;
        for (int i = 0; i < nIdf; i++) {
            if (*(int16_t*)(PhA::buffer + pIdf) == 0x102) { // tag BitsPerSample
                if (*(int32_t*)(PhA::buffer + pIdf + 4) == 3) // count
                    bitsPerSample = pIdf;
                else
                    break;
            }
            else if (*(int16_t*)(PhA::buffer + pIdf) == 0x115) { // tag SamplesPerPixel
                if ((*(int32_t*)(PhA::buffer + pIdf + 8) == 4) && (bitsPerSample >= 0)) { // value
                    // corregir BitsPerSample en 4 valores 8,8,8,8
                    *(int32_t*)(PhA::buffer + bitsPerSample + 4) = 4; // count corregido
                    int32_t bitsOriginal = *(int32_t*)(PhA::buffer + bitsPerSample + 8);
                    *(int32_t*)(PhA::buffer + bitsPerSample + 8) = sizeImage; // offset corregido
                    memcpy(PhA::buffer + sizeImage, PhA::buffer + bitsOriginal, 6); // bitsPerSample originales
                    *(int16_t*)(PhA::buffer + sizeImage + 6) = 8; // Alpha
                    sizeImage += 8;
                }
                else
                    break;

            }
            pIdf += 12;
        }
        if (tofile) {
            // Guardar como TIF
            string nombre = "imagen_" + to_string(bloque->numBloq) + ".tif";
            ofstream out(nombre, ios::binary);
            out.write(PhA::buffer, sizeImage);
        }
        break;
    }
}

void DetailPhAImagen::printDetail() {
    cout << "Imagen: " << nombre << " ";
    coord.print();
    cout << " @ ";
    sizeRect.print();
    if (tipoImage == 0)
        cout << " Raw";
    else if (tipoImage == 1)
        cout << " Tiff";
    else
        cout << " Unknow";
    cout << " en " << posImage << " size=" << sizeImage << " rot=" << rotImage << " sizeRotated ";
    sizeRotated.print();
    cout << " bloques sombra " << blqSombra << " aux1 " << blqAux1 << " aux2 " << blqAux2;
    bloque->file->seekg(posImage + sizeImage);
    bloque->file->read(PhA::buffer, 48);
    cout << endl << "                                   ";
    PhA::printBloqueHexa((unsigned char *)PhA::buffer, 48);
}

// Tipo 7
DetailPhAIndexThumb::DetailPhAIndexThumb(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t size;
    bloque->file->read((char*)&size, 4); // num bloques definidos
    bloque->file->read(PhA::checkBuffer(size * 4), size * 4);
    int32_t* punt = (int32_t*)PhA::buffer;
    for (int i = 0; i < size; i++, punt++)
        bloques.push_back(*punt);
}

void DetailPhAIndexThumb::printDetail() {
    cout << "Bloques minuaturas paginas: ";
    for (int n : bloques)
        cout << dec << setw(0) << n << " ";
}

// Tipo 8
DetailPhAPagina::DetailPhAPagina(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t size;
    bloque->file->read((char*)&size, 4); // num bloques definidos
    bloque->file->read(PhA::checkBuffer(size * 4), size * 4);
    int32_t* punt = (int32_t*)PhA::buffer;
    for (int i = 0; i < size; i++, punt++)
        bloques.push_back(*punt);
}

void DetailPhAPagina::printDetail() {
    cout << "Bloques paginas: ";
    for (int n : bloques)
        cout << dec << setw(0) << n << " ";
}

// Tipo 9
DetailPhAIndex::DetailPhAIndex(BloquePhA* _bloque) : DetailPhA(_bloque) {
    int32_t size;
    bloque->file->read((char*)&size, 4); // num bloques definidos
    bloque->file->read(PhA::checkBuffer(size * 4), size * 4);
    int32_t* punt = (int32_t*)PhA::buffer;
    for (int i = 0; i < size; i++, punt++)
        bloques.push_back(*punt);
}

void DetailPhAIndex::printDetail() {
    cout << "Bloques elementos: ";
    for (int n : bloques)
        cout << dec << setw(0) << n << " ";
}

// Tipo 19
DetailPhASombra::DetailPhASombra(BloquePhA* _bloque) : DetailPhA(_bloque) {
    bloque->file->read((char*)&color, 4);
    bloque->file->read((char*)&angulo, 4);
    bloque->file->read((char*)&distancia, 4);
    bloque->file->read((char*)&tamanno, 4);
    bloque->file->read((char*)&opacidad, 4);
}

void DetailPhASombra::printDetail() {
    cout << dec << setw(0) << " opacidad " << opacidad << " (" << (opacidad * 100) / 256 << "%) angulo " << angulo
        << " distancia " << distancia << " tamano " << tamanno << " color " << hex << setw(6) << color;
}
