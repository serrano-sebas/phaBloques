// PhA.cpp: fichero .PhA
//
using namespace std;

#include "bloquePhA.h"
#include <iostream>
#include <sstream>
#include <iomanip>

streamsize PhA::length = 0;
char* PhA::buffer = nullptr;

char* PhA::checkBuffer(streamsize size) {
    if (size > length) {
        if (buffer) delete[] buffer;
        buffer = new char[size];
        length = size;
    }
    return buffer;
};

void PhA::printBloqueHexa(unsigned char* punt, int size) {
    for (int i = 0; i < size; i++) {
        cout << hex << setw(2) << setfill('0') << (unsigned int)*(punt++) << " ";
    }
};

void PhA::printBloqueChar(unsigned char* punt, int size) {
    for (int i = 0; i < size; i++) {
        if (isprint(*punt))
            cout << *punt;
        else
            cout << '.';
        punt++;
    }
};

void PhA::printBloqueLong(int32_t* punt, int size) {
    for (int i = 0; i < size; i++) {
        if (i > 0)
            cout << " / ";
        cout << dec << setw(0) << *(punt++);
    }
};

PhA::PhA(ifstream* _file) : file(_file) {
    if (!PhA::buffer) PhA::checkBuffer(10240); // iniciar static

    file->seekg(0, ios::end);
    sizePha = file->tellg();
    file->seekg(0);
    streampos pos = readCabecera();

    while (pos < sizePha) {
        auto bloque = make_unique<BloquePhA>(file, pos);
        pos += 16 + bloque->size;
        if (bloque->tipo)
            pha.emplace(bloque->numBloq, std::move(bloque));
    }

}

PhA::~PhA() {
    if (file->is_open())
        file->close();
}

streampos PhA::readCabecera() {
    streampos pos = 0;
    file->read(firma, 8); pos += file->gcount();
    firma[8] = 0;
    file->read(PhA::buffer, 12); pos += file->gcount();
    int32_t* punt = (int32_t*)PhA::buffer;
    for (int i = 0; i < 3; i++)
        headData.push_back(*(punt++));
    return pos;
}

void PhA::printResumen(int tipo, int extract) {
    vector<int> tipoBloques(65536, 0);

    cout << endl << "Cabecera fichero (size = " << setw(0) << dec << sizePha << "):" << endl;
    cout << "  firma: " << firma << endl;
    cout << "  datos: ";
    for (int32_t data : headData)
        cout << dec << setw(0) << data << " ";
    cout << endl;
    cout << "Bloques:" << endl;
    cout << "   start num bloq 1 tipo          elem     size" << endl;
    cout << "-------- -------- - ----------- ------ --------" << endl;

    for (const auto& item : pha)
    {
        const std::unique_ptr<BloquePhA>& bloque = item.second;
        tipoBloques[bloque->tipo]++;
        bloque->printResumen();

        if (tipo == bloque->tipo) {
            bloque->printBloque();
        }
        if (extract == bloque->numBloq) {
            bloque->detail->extract(true);
        }

        cout << endl;
    }
    cout << endl << "Resumen bloques (tipos):" << endl;
    for (int i = 0; i < 65536; i++)
        if (tipoBloques[i] > 0)
            cout << dec << setw(5) << i << " = " << tipoBloques[i] << " veces" << endl;
}

void PhA::generate(string& outDir, bool verbose, bool split, int newh) {
#ifdef _WIN32
    char slash = '\\';
#else
    char slash = "/";
#endif
    if (outDir.back() != slash)
        outDir += slash;

    FreeImage_Initialise();
    if (verbose)
        cout << endl << "Libreria FreeImage cargada correctamente ..." << endl;

    if (verbose) {
        cout << "Generando imagenes en " << outDir << " ..." << endl;
    }
    if (verbose) {
        auto bloque = pha.find(1);
        if ((bloque == pha.end()) || (bloque->second->tipo != 1)) {
            cout << "Bloque 1 (header) no encontrado" << endl;
        }
        else {
            DetailPhAHeader* header = dynamic_cast<DetailPhAHeader*>(bloque->second->detail.get());
            cout << "Book de " << header->nPags << " paginas" << endl;
        }
    }

    auto pBloque = pha.find(2); // ix_thumb 7
    if ((pBloque == pha.end()) || (pBloque->second->tipo != 7)) {
        cout << "Bloque 2 (ix thmb - 7) no encontrado" << endl;
    }
    else { // indice de thumbnails encontrado. 
           // i-2 indice de paginas (para sacar nombre archivo)
           // i-1 indice de elementos
        DetailPhAIndexThumb* iThumb = dynamic_cast<DetailPhAIndexThumb*>(pBloque->second->detail.get());
        for (int bloqueThumb : iThumb->bloques) {
            if (!generatePag(outDir, bloqueThumb, verbose, split, newh))
                break;
        }
    }

    FreeImage_DeInitialise();
    if (verbose)
        cout << "Libreria FreeImage descargada" << endl << "Proceso finalizado" << endl;
}

bool PhA::generatePag(string& outDir, int nBloqueThumb, bool verbose, bool split, int newh) {
    auto pBloque = pha.find(nBloqueThumb);
    if ((pBloque == pha.end()) || (pBloque->second->tipo != 2)) {
        PhA::printError(nBloqueThumb, "Esperado tipo bloque 2 (thumb)");
        return false;
    }
    DetailPhAThumb* idx = dynamic_cast<DetailPhAThumb*>(pBloque->second->detail.get());
    pBloque = pha.find(idx->blqPags);
    if ((pBloque == pha.end()) || (pBloque->second->tipo != 8)) {
        PhA::printError(idx->blqPags, "Esperado tipo bloque 8 (pagina)");
        return false;
    }
    DetailPhAPagina* iPags = dynamic_cast<DetailPhAPagina*>(pBloque->second->detail.get());
    string nameFile = "";
    string leftFile = "";
    string rightFile = "";
    Rect canvasRect;
    canvasRect.left = 0; canvasRect.top = 0; canvasRect.right = 0; canvasRect.bottom = 0;
    bool first = true;
    if (verbose)
        cout << "... leyendo paginas ";
    for (int blqPag : iPags->bloques) {
        pBloque = pha.find(blqPag);
        if ((pBloque == pha.end()) || (pBloque->second->tipo != 3)) {
            PhA::printError(blqPag, "Esperado tipo bloque 3 (pag)");
            return false;
        }
        DetailPhAPag* iPag = dynamic_cast<DetailPhAPag*>(pBloque->second->detail.get());
        if (first)
            first = false;
        else
            nameFile += "-";
        if (iPag->nPag == -1)
            nameFile += "0000A_Tapas";
        else if (iPag->nPag == -2)
            ;
        else if (iPag->nPag == -3) {
            nameFile += "0000B_GDelantera";
            leftFile = outDir + "0000B_GDelantera1.jpg";
            rightFile = outDir + "0000B_GDelantera2.jpg";
        }
        else if (iPag->nPag == -4) {
            nameFile += "ZZZZ_GTrasera";
			leftFile = outDir + "ZZZZ_GTrasera1.jpg";
			rightFile = outDir + "ZZZZ_GTrasera2.jpg";
        }
        else if (iPag->nPag > 0) {
            ostringstream oss;
            oss << setw(4) << std::setfill('0') << iPag->nPag;
            nameFile += oss.str();
            if (leftFile.empty())
				leftFile = outDir + oss.str() + ".jpg";
            else
				rightFile = outDir + oss.str() + ".jpg";
        }
        else {
            PhA::printError(blqPag, "Numero de pagina no esperada");
            return false;
        }
        if (iPag->coord.right > canvasRect.right)
            canvasRect.right = iPag->coord.right;
        if (iPag->coord.bottom > canvasRect.bottom)
            canvasRect.bottom = iPag->coord.bottom;
        if (iPag->nPag == -4) // la anchura de la contraportada trasera es el doble
            canvasRect.right *= 2;
        if (verbose)
            cout << iPag->nPag << " ";
    }
    nameFile = outDir + nameFile + ".jpg";
    if (verbose) {
        cout << "... generando JPG " << nameFile;
        canvasRect.print();
        cout << endl;
    }

    // Crear lienzo RGB (32 bits)
    const int canvas_w = canvasRect.right - canvasRect.left;
    int canvas_h = canvasRect.bottom - canvasRect.top;
    FIBITMAP* canvas = FreeImage_Allocate(canvas_w, canvas_h, 24);

    // Fondo blanco
    for (int y = 0; y < canvas_h; y++)
    {
        BYTE* line = FreeImage_GetScanLine(canvas, y);
        for (int x = 0; x < canvas_w * 3; x++)
            line[x] = 255;
    }

    pBloque = pha.find(idx->blqElems);
    if ((pBloque == pha.end()) || (pBloque->second->tipo != 9)) {
        PhA::printError(idx->blqElems, "Esperado tipo bloque 9 (ix elem)");
        return false;
    }
    DetailPhAIndex* iElems = dynamic_cast<DetailPhAIndex*>(pBloque->second->detail.get());
    if (verbose)
        cout << "Elementos pagina:";
    for (auto iter = iElems->bloques.begin(); iter != iElems->bloques.end(); ++iter) {
        int blqElem = *iter;
        if (verbose)
            cout << " " << blqElem;
        pBloque = pha.find(blqElem);
        int tBloque = pBloque->second->tipo;
        if (pBloque == pha.end()) {
            PhA::printError(blqElem, "Tipo bloque elemento no encontrado");
            return false;
        }
        DetailPhAImagen* imag;
        DetailPhASombra* sombra;
        int offsetx,offsety;
        switch (tBloque) {
        case 6:
        case 11: // Fotoprix especificos
            break;
        case 4: // Imagen (raw / tiff)
        case 5: // Texto (raw / tiff)
            imag = dynamic_cast<DetailPhAImagen*>(pBloque->second->detail.get());
            imag->extract(false);
            offsetx = 0;
            offsety = 0;
            if (imag->blqSombra) { // hay sombra alrededor por lo que se calcula un offset para dibujar
                pBloque = pha.find(imag->blqSombra);
                if (pBloque == pha.end()) {
                    PhA::printError(blqElem, "Bloque sombra asociado no encontrado");
                    return false;
                }
                if (pBloque->second->tipo != 19) {
                    PhA::printError(blqElem, "Tipo bloque sombra asociado no esperado (debe ser 19)");
                    return false;
                }
                sombra = dynamic_cast<DetailPhASombra*>(pBloque->second->detail.get());
                float angulo = ((float)sombra->angulo) * PI / 180.0f;
                double dx = -((double)sombra->distancia) * cos(angulo);
                double dy = -((double)sombra->distancia) * sin(angulo);
                int dw = imag->sizeRotated.width - imag->sizeRect.width;
                double radio = (sombra->tamanno > abs(dx)) ? (0.5 * dw) : (- abs(dx) + dw);
                offsetx = (int)max(radio - dx, 0.0);
                offsety = (int)max(radio + dy, 0.0);
            }
            switch (imag->tipoImage) {
            case 0:
                pasteImageRaw(canvas, (unsigned char*)PhA::buffer,
                    imag->sizeRotated.width, imag->sizeRotated.height,
                    imag->coord.left - offsetx, imag->coord.top - offsety);
                break;
            case 1:
                pasteImageTiff(canvas, PhA::buffer, imag->sizeImage, 
                    imag->coord.left, imag->coord.top,
                    imag->sizeRect.width, imag->sizeRect.height);
                break;
            }
            break;
        default:
            PhA::printError(blqElem, "Tipo bloque elemento no esperado");
            return false;
        }
    }
    cout << endl;

	// Escalado vertical para ajustar a la altura del lienzo (si es necesario)
    if (!leftFile.empty() && (newh > 0)) {
        FIBITMAP* scaled = ExtendVerticalBlur(canvas, newh);
        FreeImage_Unload(canvas);
        canvas = scaled;
        canvas_h = 2970;
    }

    // Guardando imagen
    FreeImage_SetDotsPerMeterX(canvas, 10000);
    FreeImage_SetDotsPerMeterY(canvas, 10000);
    if (!split || leftFile.empty()) {
        FreeImage_Save(FIF_JPEG, canvas, nameFile.c_str(), JPEG_QUALITYSUPERB);
    }
    else {
        // Dividir en dos mitades
        int half = canvas_w / 2;

        // Recortar izquierda
        FIBITMAP* left = FreeImage_Copy(canvas, 0, 0, half, canvas_h);
        // Recortar derecha
        FIBITMAP* right = FreeImage_Copy(canvas, half, 0, canvas_w, canvas_h);
        // ?? Convertir a 24 bits para JPG (MUY IMPORTANTE)
        FIBITMAP* left24 = FreeImage_ConvertTo24Bits(left);
        FIBITMAP* right24 = FreeImage_ConvertTo24Bits(right);
        // Guardar JPG
        if (!leftFile.empty()) {
            FreeImage_Save(FIF_JPEG, left24, leftFile.c_str(), JPEG_QUALITYSUPERB);
            FreeImage_Save(FIF_JPEG, right24, rightFile.c_str(), JPEG_QUALITYSUPERB);
        }

        // Liberar memoria
        FreeImage_Unload(left);
        FreeImage_Unload(right);
        FreeImage_Unload(left24);
        FreeImage_Unload(right24);
    }
    FreeImage_Unload(canvas);
    return true;
}

// ------------------------------------------------------------
// Mezclar una imagen con transparencia sobre el lienzo
// ------------------------------------------------------------
void PhA::pasteImageTiff(FIBITMAP* canvas, char* buffer, size_t size, int x, int y, int width, int height) {
    FIMEMORY* mem = FreeImage_OpenMemory((BYTE*)buffer, size); // Cargar TIFF desde memoria
    FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(mem);
    FIBITMAP* src = FreeImage_LoadFromMemory(format, mem);

    int w = FreeImage_GetWidth(src);
    int h = FreeImage_GetHeight(src);
    y = FreeImage_GetHeight(canvas) - h - (y - (h - height) /2); // cambio coordenadas
    x = x - (w - width) / 2;

    // Convertir a 32 bits (RGBA)
    FIBITMAP* src32 = FreeImage_ConvertTo32Bits(src);

    for (int iy = 0; iy < h; iy++)
    {
        BYTE* srcPixel = FreeImage_GetScanLine(src32, iy);

        for (int ix = 0; ix < w; ix++)
        {
            int dx = x + ix;
            int dy = y + iy;

            if (dx < 0 || dy < 0 ||
                dx >= FreeImage_GetWidth(canvas) ||
                dy >= FreeImage_GetHeight(canvas))
                continue;

            BYTE* dstPixel = FreeImage_GetScanLine(canvas, dy) + dx * 3;

            BYTE b = srcPixel[ix * 4 + 0];
            BYTE g = srcPixel[ix * 4 + 1];
            BYTE r = srcPixel[ix * 4 + 2];
            BYTE a = srcPixel[ix * 4 + 3];

            float alpha = a / 255.0f;

            dstPixel[0] = (BYTE)(b * alpha + dstPixel[0] * (1 - alpha));
            dstPixel[1] = (BYTE)(g * alpha + dstPixel[1] * (1 - alpha));
            dstPixel[2] = (BYTE)(r * alpha + dstPixel[2] * (1 - alpha));
        }
    }

    FreeImage_Unload(src32);
    FreeImage_CloseMemory(mem);
}

void PhA::pasteImageRaw(FIBITMAP* canvas, unsigned char* rgbAlpha,
    int width, int height, int posX, int posY) {
    posY = FreeImage_GetHeight(canvas) - height - posY; // cambio coordenadas
    int ipos = 0;
    int jpos = ((width *3 +3) / 4) * 4 * height;
    for (int iy = height -1 ; iy >= 0; iy--) {
        for (int ix = 0; ix < width; ix++) {
            int dx = posX + ix;
            int dy = posY + iy;

            BYTE b = rgbAlpha[ipos++];
            BYTE g = rgbAlpha[ipos++];
            BYTE r = rgbAlpha[ipos++];
            BYTE a = rgbAlpha[jpos++];
            float alpha = (ix ? a / 255.0f : 0.0f);

            if (dx < 0 || dy < 0 ||
                dx >= FreeImage_GetWidth(canvas) ||
                dy >= FreeImage_GetHeight(canvas))
                continue;

            BYTE* dstPixel = FreeImage_GetScanLine(canvas, dy) + dx * 3;

            dstPixel[0] = (BYTE)(b * alpha + dstPixel[0] * (1 - alpha));
            dstPixel[1] = (BYTE)(g * alpha + dstPixel[1] * (1 - alpha));
            dstPixel[2] = (BYTE)(r * alpha + dstPixel[2] * (1 - alpha));
        }
        ipos = ((ipos + 3) / 4) * 4;
        jpos = ((jpos + 3) / 4) * 4;
    }
}

FIBITMAP* PhA::ExtendVerticalBlur(FIBITMAP* src, int newHeight)
{
    int width = FreeImage_GetWidth(src);
    int height = FreeImage_GetHeight(src);

	cout << "resizing from " << height << " to " << newHeight << " pixels vertically" << endl;

    if (newHeight <= height)
        return FreeImage_Clone(src);

    int extra = newHeight - height;
    int band = extra / 2;
    int sample = band / 3;

    // Convertir a 32 bits por seguridad
    FIBITMAP* src32 = FreeImage_ConvertTo32Bits(src);

    // Crear canvas destino
    FIBITMAP* dst = FreeImage_Allocate(width, newHeight, 32);

    // ----------- 1. Copiar imagen original centrada -----------
    FreeImage_Paste(dst, src32, 0, band, 256);

    // ----------- 2. Banda SUPERIOR -----------
    FIBITMAP* topSample = FreeImage_Copy(src32, 0, height - sample, width, height);
    FIBITMAP* topScaled = FreeImage_Rescale(topSample, width, band, FILTER_BILINEAR);
    FreeImage_FlipVertical(topScaled);
    FIBITMAP* tmp = FreeImage_Rescale(topScaled, width / 10, band / 10, FILTER_BILINEAR);
    FIBITMAP* blurred = FreeImage_Rescale(tmp, width, band, FILTER_BILINEAR);
	FreeImage_Unload(topSample);
    FreeImage_Unload(topScaled);
    FreeImage_Unload(tmp);

    FreeImage_Paste(dst, blurred, 0, band + height, 256);
    FreeImage_Unload(blurred);

    // ----------- 3. Banda INFERIOR -----------
    FIBITMAP* bottomSample = FreeImage_Copy(src32, 0, 0, width, sample);
    FIBITMAP* bottomScaled = FreeImage_Rescale(bottomSample, width, band, FILTER_BILINEAR);
    FreeImage_FlipVertical(bottomScaled);
    tmp = FreeImage_Rescale(bottomScaled, width / 10, band / 10, FILTER_BILINEAR);
    blurred = FreeImage_Rescale(tmp, width, band, FILTER_BILINEAR);
    FreeImage_Unload(bottomSample);
    FreeImage_Unload(bottomScaled);
    FreeImage_Unload(tmp);

    FreeImage_Paste(dst, blurred, 0, 0, 256);
    FreeImage_Unload(blurred);
 
    // ----------- 4. Liberar memoria -----------
    FreeImage_Unload(src32);

    return dst;
}
