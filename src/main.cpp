#include <iostream>
#include <memory>
#include <vector>
#include "ecogrid.h"

using namespace std;

void imprimirSeparador(const string titulo) {
    cout << "\n========== " << titulo << " ==========" << endl;
}

int main() {
    imprimirSeparador("Creacion de nodos");
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


    auto bateriaPtr = make_unique<NodoBateria>(99, "Bateria Comunitaria", 0.0);
    NodoBateria& bateria = *bateriaPtr; //referencia a nodoBateria para que se pueda seguir llamando en el main
    nodos.push_back(move(bateriaPtr));//se le pasa propiedad del puntero al vector

    //unique_ptr tiene que pasarse por referencia siempre o tira error
    for (const auto& nodo : nodos) {
        nodo->infoNodo();
        cout << "excedente=" << nodo->calcularExcedente() << " kWh"
                  << endl;
    }

    try {
        auto config = ConfigReader::leer("src/util/config.ini");
        CapaDatos capaDatos(config.at("db_path"));
        cout << "Conexion establecida correctamente.\n";

        capaDatos.inicializarEsquema();
        capaDatos.sembrarNodos(nodos);
        capaDatos.sembrarTarifas();

        imprimirSeparador("Carga de ordenes");

        GridManager gridManager;
        uint64_t secuencia = 1;

        for (int hora = 0; hora < 24; hora++) {

            cout << "\n===== TICK hora " << hora << " =====" << endl;

            double precioBase = capaDatos.obtenerPrecioBase(hora);

            // leer las ordenes del CSV
            vector<Orden> ordenes = CSVParser::leerOfertas(hora, secuencia);

            // procesarTick: oferta de bateria + matching + excedente a bateria
            vector<TransaccionEnergia> transacciones =
                gridManager.procesarTick(ordenes, bateria, precioBase, secuencia);

            cout << "Transacciones: " << transacciones.size() << endl;
            for (const auto& trans : transacciones) {
                trans.imprimir();
            }

            // persistencia transaccional del tick: si falla, se loguea y se sigue con el proximo tick
            if (!capaDatos.persistirTransacciones(transacciones, hora)) {
                cerr << "[GridManager] Tick " << hora
                     << " rechazado por la BD, se continua con el siguiente tick." << endl;
            }
        }

    } catch (exception const& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
