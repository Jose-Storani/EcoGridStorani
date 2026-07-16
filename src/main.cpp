#include <iostream>
#include <memory>
#include <vector>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include "ecogrid.h"

using namespace std;

void imprimirSeparador(const string titulo) {
    cout << "\n========== " << titulo << " ==========" << endl;
}

int main() {
        try {
        // Esto crea (o abre) el archivo en el disco
        soci::session sql(soci::sqlite3, "tp.db");
        cout << "Conexión a SQLite exitosa." << endl;

        // Acá va el resto de tu código que ya hiciste...

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
    imprimirSeparador("Creación de nodos");
    //con puntero inteligente para ahorrar borrarlo manual
    vector<unique_ptr<NodoRed>> nodos;

    nodos.push_back(make_unique<NodoProsumidor>(
        5,  "EnergyCorp",
         100.0, 10.0,  0.0));

    nodos.push_back(make_unique<NodoProsumidor>(
         3,  "Epe",
       100.0, 5.0, 0.0));

    nodos.push_back(make_unique<NodoConsumidor>(
         7,  "Novogar",
        100.0, 8.0, PerfilConsumo::Comercial));

    nodos.push_back(make_unique<NodoConsumidor>(
         9, "Fravega",
        100.0,  6.0, PerfilConsumo::Residencial));

        nodos.push_back(make_unique<NodoConsumidor>(
         19,  "Kymco",
         100.0, 6.0, PerfilConsumo::Residencial));

    nodos.push_back(make_unique<NodoConsumidor>(
        29, "Honda",
        100.0, 6.0, PerfilConsumo::Residencial));

    NodoBateria bateria(99, "Bateria Comunitaria", 0.0);

    //unique_ptr tiene que pasarse por referencia siempre o tira error
    for (const auto& nodo : nodos) {
        nodo->infoNodo();
        cout << "excedente=" << nodo->calcularExcedente() << " kWh"
                  << endl;
    }

    imprimirSeparador("Carga de ordenes");

    GridManager gridManager;
    uint64_t secuencia = 1;

    for (int hora = 0; hora < 24; hora++) {

    cout << "\n===== TICK hora " << hora << " =====" << endl;

    // si la batería tiene carga del tick anterior, la oferta primero
    if (bateria.getCargaActual() > 0.001) {
        double precioBase = 2.5; // por ahora fijo, después viene de BD
        gridManager.ofertarEnergiaBateria(bateria, precioBase, secuencia++);
    }

    //leer las órdenes del CSV
    vector<Orden> ordenes = CSVParser::leerOfertas(hora, secuencia);
    gridManager.cargarOrdenes(ordenes);

    //ejecutar el matching
    vector<TransaccionEnergia> transacciones = gridManager.ejecutarMatching();
    cout << "Transacciones: " << transacciones.size() << endl;
    for (const auto& trans : transacciones) {
        trans.imprimir();
    }

    // excedente restante va a la batería
    double excedente = gridManager.calcularExcedenteNoVendido();
    if (excedente > 0.001) {
        cout << "Excedente a bateria: " << excedente << " kWh" << endl;
        bateria.absorberExcedente(excedente); //revisar calculo, el excedente es muy alto al final decada tick
    }

    //limpiar el libro para el próximo tick
    gridManager.limpiarLibro();
}


}
