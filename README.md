# phaBloques
Aplicación de escritorio desarrollada en C++ con Microsoft Visual Studio
Lee un fichero *.PhA (álbum de fotos), muestra en pantalla su estructura de bloques y realiza funciones adicionales según los parámetros de llamada:
phaBloques struct: solo muestra la información resumida de los bloques en pantalla
phaBloques tipo n: además de la información resumida de los bloques muestra para los bloques de tipo n el contenido del bloque hexadecimal, caracteres y long.
phaBloques extract n: además de la información resumida extrae en un fichero el contenido del bloque (para tipos 4 y 5 imágenes y texto)
phaBloques generate: genera archivos JPG con el diseño de cada pareja de páginas del álbum (como se muestran en el editor del programa original)
                     los archivos se guardan en un directorio con el nombre del archivo PhA en el mismo directorio del archivo.

# Requisitos
El programa necesita la librería FreeImage (probado con versión 3.18)

# Instalación librería FreeImage
Este repositorio **no incluye la librería**, se debe descargar manualmente y colocar los archivos FreeImage.h y FreeImage.lib donde pueda localizarlos el compilador
El archivo FreeImage.dll debe estar en el mismo directorio que el ejecutable
Sitio de la librería: https://freeimage.sourceforge.io/
