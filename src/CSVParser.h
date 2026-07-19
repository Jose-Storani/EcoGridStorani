#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include "orden.h"

using namespace std;

class CSVParser {
public:
    static vector<Orden> leerOfertas(int hora, uint64_t& secuencia) {

        string nombreArchivo = string("datos/ofertas_")
                               + (hora < 10 ? "0" : "")
                               + to_string(hora)
                               + ".csv";
        ifstream archivo(nombreArchivo);
        vector<Orden> ordenes;
        string linea;
        // saltear encabezado
        getline(archivo, linea);

        // leer líneas
        while (getline(archivo, linea)) {

            if (linea.empty()) continue;

            stringstream ss(linea);
            string campo;

            getline(ss, campo, ',');
            int idOrden = stoi(campo);

            getline(ss, campo, ',');
            bool esCompra = (campo == "compra");

            getline(ss, campo, ',');
            int idNodo = stoi(campo);

            getline(ss, campo, ',');
            double kwh = stod(campo);

            getline(ss, campo, ',');
            double precio = stod(campo);

            Orden orden(idOrden, esCompra, idNodo, precio, kwh, secuencia++);
            ordenes.push_back(orden);

        }

        cout << "[CSV] Hora " << hora << ": "
             << ordenes.size() << " ordenes cargadas." << endl;

        return ordenes;
    }
};
