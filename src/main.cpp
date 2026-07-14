#include <iostream>
#include <memory>
#include <vector>

#include "ecogrid.h"


using namespace std;

//    id_orden  lado    id_nodo  kwh  precio
//    101       venta   5        10   2.5
//    102       venta   3        5    2.8
//    201       compra  7        8    3.0
//    202       compra  9        6    2.4
//
//  Resultado esperado (según el enunciado):
//    - 1 transacción: vendedor 5, comprador 7, 8 kWh, precio 2.75
//    - Orden 101 queda con 2 kWh remanentes (sin comprador)
//    - Orden 201 se completa
//    - Orden 202 (compra a 2.4) NO cruza con 101 (venta a 2.5)
// ============================================================

void imprimirSeparador(const string titulo) {
    cout << "\n========== " << titulo << " ==========" << endl;
}

int main() {
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
