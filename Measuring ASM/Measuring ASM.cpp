// Measuring ASM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include <Windows.h>
#include <vector>

typedef void (*_cdecl void_fcn)();

std::chrono::steady_clock::time_point start1;
std::chrono::steady_clock::time_point end1;
long long duration1;

void execute(LPBYTE bytes, DWORD len_bytes)
{
    LPBYTE exec_region = (LPBYTE)VirtualAlloc(NULL, len_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    DWORD dwProtect = 0;

    if (exec_region != NULL)
    {
        memcpy(exec_region, bytes, len_bytes);
        //VirtualProtect(exec_region, len_bytes, PAGE_EXECUTE_READWRITE, &dwProtect);
        void_fcn fun = (void_fcn)exec_region;
        fun();
        //VirtualProtect(exec_region, len_bytes, dwProtect, &dwProtect);
        VirtualFree(exec_region, 0, MEM_RELEASE);
        fun = NULL;
        exec_region = NULL;
    }
}

void SaveStart()
{
    start1 = std::chrono::high_resolution_clock::now();
}

void SaveEnd()
{
    end1 = std::chrono::high_resolution_clock::now();
    duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
    std::cout << "\t" << "Duracion de instruccion: " << duration1 << " nanosegundos" << std::endl;

    duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    std::cout << "\t" << "Duracion de instruccion: " << duration1 << " milisegundos" << std::endl << std::endl;
}

void BuildASMShellCode(std::vector<BYTE> byteArr)
{
    /*
    call SaveStart (5)
    ___
    ___
    call SaveEnd (5)
    ret (1)
    */
    DWORD dwProtect;
    int index = 0;
    std::vector<BYTE> b(11 + byteArr.size());

    VirtualProtect(b.data(), b.size(), PAGE_EXECUTE_READWRITE, &dwProtect);

    // Calcula la dirección relativa de la función
    int relativeAddress = (int)&SaveStart - ((int)&b[0] + 5);

    // Convierte la dirección relativa en un valor de 4 bytes
    char* pRelativeAddress = (char*)&relativeAddress;

    // Agrega los opcodes necesarios para hacer el "call" a la función
    b[0] = 0xE8;
    b[1] = pRelativeAddress[0];
    b[2] = pRelativeAddress[1];
    b[3] = pRelativeAddress[2];
    b[4] = pRelativeAddress[3];

    index = 5;

    for (size_t i = 0; i < byteArr.size(); i++)
    {
        b[index + i] = byteArr[i];
    }

    index += byteArr.size();

    relativeAddress = (int)&SaveEnd - ((int)&b[index] + 5);
    pRelativeAddress = (char*)&relativeAddress;

    b[index++] = 0xE8;
    b[index++] = pRelativeAddress[0];
    b[index++] = pRelativeAddress[1];
    b[index++] = pRelativeAddress[2];
    b[index++] = pRelativeAddress[3];

    b[index] = 0xC3;

    ((void(*)()) & b[0])();

    VirtualProtect(b.data(), b.size(), dwProtect, &dwProtect);
}

void BuildASMShellCode2(std::vector<BYTE> byteArr)
{
    /*
    call SaveStart (5)
    ___
    ___
    call SaveEnd (5)
    ret (1)
    */
    DWORD dwProtect;
    int index = 0;
    std::vector<BYTE> b(1 + byteArr.size());

    VirtualProtect(b.data(), b.size(), PAGE_EXECUTE_READWRITE, &dwProtect);

    for (size_t i = 0; i < byteArr.size(); i++)
    {
        b[i] = byteArr[i];
    }

    b[b.size() - 1] = 0xC3;

    ((void(*)()) & b[0])();

    VirtualProtect(b.data(), b.size(), dwProtect, &dwProtect);
}

#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

struct tSections
{
    std::string section;
    std::vector<BYTE> bytes;
};

//std::vector<std::vector<BYTE>> secciones;
std::vector<tSections> secciones;

// Función para eliminar espacios en blanco a la izquierda y derecha de una cadena.
inline std::string trim(const std::string& str)
{
    const auto begin = str.find_first_not_of(" \t");
    if (begin == std::string::npos)
    {
        return ""; // La cadena contiene solo espacios en blanco.
    }
    const auto end = str.find_last_not_of(" \t");
    const auto length = end - begin + 1;
    return str.substr(begin, length);
}

void ReadSections()
{
    std::ifstream file("archivo.txt");
    tSections section;

    std::string linea;
    while (std::getline(file, linea))
    {
        // Eliminar comentarios y espacios en blanco al inicio y final de la línea.
        const auto comentario_pos = linea.find_first_of(";");
        if (comentario_pos != std::string::npos)
        {
            linea = linea.substr(0, comentario_pos);
        }
        linea = trim(linea);

        // Si la línea comienza con "[", asumir que es el inicio de una nueva sección.
        if (!linea.empty() && linea[0] == '[')
        {
            // Almacenar los bytes de la sección anterior.
            if (!section.bytes.empty())
            {
                secciones.push_back(section);
                section.bytes.clear();
            }

            section.section = linea;
        }
        else
        {
            // Procesar la línea como un byte hexadecimal y almacenarla en un vector de bytes.
            std::istringstream iss(linea);
            int byte;
            while (iss >> std::hex >> byte)
            {
                section.bytes.push_back(static_cast<unsigned char>(byte));
            }
        }
    }

    // Almacenar los bytes de la última sección.
    if (!section.bytes.empty())
    {
        secciones.push_back(section);
    }

    // Imprimir los bytes de cada sección.
    /*for (const auto& seccion : secciones)
    {
        std::cout << "Seccion: ";
        for (const auto& byte : seccion)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }*/
}

void ReadSections2()
{
    std::ifstream file("archivo.txt");
    tSections section;

    std::string linea;
    while (std::getline(file, linea))
    {
        // Si la línea comienza con "[", asumir que es el inicio de una nueva sección.
        if (linea[0] == '[')
        {
            // Almacenar los bytes de la sección anterior.
            if (!section.bytes.empty())
            {
                secciones.push_back(section);
                section.bytes.clear();
            }
        }
        else
        {
            // Procesar la línea como un byte hexadecimal y almacenarla en un vector de bytes.
            std::istringstream iss(linea);
            int byte;
            while (iss >> std::hex >> byte)
            {
                section.bytes.push_back(static_cast<unsigned char>(byte));
            }
        }
    }

    // Almacenar los bytes de la última sección.
    if (!section.bytes.empty())
    {
        secciones.push_back(section);
    }

    // Imprimir los bytes de cada sección.
    /*for (const auto& seccion : secciones)
    {
        std::cout << "Seccion: ";
        for (const auto& byte : seccion)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }*/
}

int main()
{
    DWORD dwProtect;

    ReadSections();

    for (size_t i = 0; i < secciones.size(); i++)
    {
        std::cout << secciones[i].section << std::endl;
        std::cout << "\t\t";

        for (const auto& byte : secciones[i].bytes)
        {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        //BuildASMShellCode(secciones[i]);

        VirtualProtect(secciones[i].bytes.data(), secciones[i].bytes.size(), PAGE_EXECUTE_READWRITE, &dwProtect);

        SaveStart();
        for (size_t times = 0; times < 0xFFFFFFFF; times++)
        {
            //BuildASMShellCode2(secciones[i].bytes);
            ((void(*)()) & secciones[i].bytes[0])();
        }
        SaveEnd();

        VirtualProtect(secciones[i].bytes.data(), secciones[i].bytes.size(), dwProtect, &dwProtect);
    }

    //byteArr.push_back(0x90);

    //BuildASMShellCode(byteArr);

    /*int x = 0;
    DWORD dwProtect;

    //char b[] = {
    //            0xB8, 0x33, 0x03, 0x00, 0x00,
    //            0xC3
    //};

    std::vector<BYTE> b;

    b.push_back(0xB8);
    b.push_back(0x33);
    b.push_back(0x03);
    b.push_back(0x00);
    b.push_back(0x00);
    b.push_back(0xC3);

    VirtualProtect(b.data(), sizeof(b), PAGE_EXECUTE_READWRITE, &dwProtect);

    //int (*func)() = (int(*)()) & b[0];
    //x = func();
    x = ((int(*)()) & b[0])();

    std::cout << std::hex << x << std::dec << std::endl;*/

    /*char b[] = {
                    0xE8, 0x00, 0x00, 0x00, 0x00, // call SaveStart
                    0xC3                         // ret
    };

    VirtualProtect(b, sizeof(b), PAGE_EXECUTE_READWRITE, &dwProtect);

    // Calcula la dirección relativa de la función
    int relativeAddress = (int)&SaveStart - ((int)&b[0] + 5);

    // Convierte la dirección relativa en un valor de 4 bytes
    char* pRelativeAddress = (char*)&relativeAddress;

    // Agrega los opcodes necesarios para hacer el "call" a la función
    b[1] = pRelativeAddress[0];
    b[2] = pRelativeAddress[1];
    b[3] = pRelativeAddress[2];
    b[4] = pRelativeAddress[3];

    // Llama al shellcode
    ((void(*)()) & b[0])();
    */

    /*
    0:  b8 33 03 00 00          mov    eax,0x333
    5:  a3 78 56 34 12          mov    ds:0x12345678,eax 
    */
    /*int x = 0;
    DWORD ptr = (DWORD)&x;

    char b[] = {
                0xB8, 0x33, 0x03, 0x00, 0x00,
                0xA3, 0x00, 0x00, 0x00, 0x00,
                0xC3
                };

    for (size_t i = 0; i < 4; i++)
    {
        b[6 + i] = ((ptr >> (8 * i)) & 0xFF);
    }

    execute((LPBYTE)&b, sizeof(b));

    std::cout << std::hex << x << std::dec << std::endl;*/

    /*// Instrucción 1: mover un valor a un registro
    int x = 10;
    auto start1 = std::chrono::high_resolution_clock::now();

    //asm("movl %0, %%eax;" : "r" (x));
    //__asm__("movl %0, %%eax;": : "r" (x)); // En Visual Studio es necesario añadir __asm__ y un punto y coma antes del operador ':'

    __asm
    {
        ; mov eax, dword ptr[x]
    }

    DWORD ptr = (DWORD)&x;

    std::cout << std::hex << ptr << std::dec << std::endl;

    BYTE mByte;

    __asm _emit 0xA3;

    for (size_t i = 0; i < 4; i++)
    {
        //unsigned char byte = (num / (1 << (8 * i))) % 0x100;

        //mByte = ((ptr >> (8 * i)) & 0xFF);
        
        //std::cout << std::hex << ((ptr >> (8 * i)) & 0xFF) << std::dec << std::endl;

        //std::cout << std::hex << (((DWORD)ptr / (DWORD)pow(256, i)) & 0xFF) << std::dec << std::endl;
    }

    __asm
    {
        ; mov dword ptr[x], eax
    }

    std::cout << x << std::endl;

    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();

    // Instrucción 2: sumar dos valores en registros
    int y = 20, z = 30;
    auto start2 = std::chrono::high_resolution_clock::now();

    //asm("addl %1, %0" : "+r" (z) : "r" (y));

    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();

    std::cout << "Duración de instrucción 1: " << duration1 << " nanosegundos" << std::endl;
    std::cout << "Duración de instrucción 2: " << duration2 << " nanosegundos" << std::endl;*/

    std::cout << "Press any key to exit." << std::endl;

    std::cin.get();

    return EXIT_SUCCESS;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
