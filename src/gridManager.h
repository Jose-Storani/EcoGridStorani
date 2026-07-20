#pragma once

#include <map>
#include <queue>
#include <vector>
#include <functional>
#include <iostream>
#include <iomanip>

#include "orden.h"
#include "transaccionEnergia.h"
#include "nodoBateria.h"

using namespace std;

class GridManager {
    private:
    map<double, queue<Orden>, greater<double>> bidMap_;
    map<double, queue<Orden>> askMap_;
    public:
        GridManager() = default;

        void insertarOrden(const Orden& orden) {
        if (orden.precio < 0.0)
            throw invalid_argument("El precio de una orden no puede ser negativo.");

        if (orden.esCompra) {
            bidMap_[orden.precio].push(orden);
        } else {
            askMap_[orden.precio].push(orden);
        }
    }

    void cargarOrdenes(const vector<Orden>& ordenes) {
        for (const auto& orden : ordenes) {
            insertarOrden(orden);
        }
    }

    //al inicio de cada tick(excepto el inicial), se carga el excedente del tick anterior como venta(askMap)
        void ofertarEnergiaBateria(NodoBateria& bateria,
                                double precioBaseHorario,
                                uint64_t secuencia) {
            double kwhDisponible = bateria.calcularExcedente();
            if (kwhDisponible <= 0.001) return;

            // se descuenta apenas se oferta; si no se vende, vuelve como parte del excedente no vendido al final del tick (evita duplicar la carga)
            bateria.liberarEnergia(kwhDisponible);

        Orden ordenBateria(
            static_cast<int>(secuencia),
            false,
            bateria.getId(),
            precioBaseHorario,
            kwhDisponible,
            secuencia
        );
        insertarOrden(ordenBateria);

    }


    //Algoritmo de Matching

        vector<TransaccionEnergia> ejecutarMatching() {
        vector<TransaccionEnergia> transacciones;

        while (!bidMap_.empty() && !askMap_.empty()) {
            auto mejorBid = bidMap_.begin(); // precio de compra mas alto
            auto mejorAsk = askMap_.begin(); // precio de venta mas bajo

            double precioBid = mejorBid->first;
            double precioAsk = mejorAsk->first;

            if (precioBid >= precioAsk) {
                Orden ordenCompra = mejorBid->second.front();
                Orden ordenVenta  = mejorAsk->second.front();

                double energia = min(ordenCompra.kwh, ordenVenta.kwh);
                double precioClearing = (ordenCompra.precio + ordenVenta.precio) / 2.0;

                transacciones.emplace_back(
                    ordenVenta.idNodo,   // idVendedor
                    ordenCompra.idNodo,  // idComprador
                    energia,
                    precioClearing
                );

                // Actualizar en cada orden
                ordenCompra.kwh -= energia;
                ordenVenta.kwh  -= energia;

                mejorBid->second.pop();
                mejorAsk->second.pop();

                // Si queda kwh pendiente, vuelve al final
                if (ordenCompra.kwh > 0.001)
                    mejorBid->second.push(ordenCompra);

                if (ordenVenta.kwh > 0.001)
                    mejorAsk->second.push(ordenVenta);

                //borro la entrada si la cola esta vacia, para que no quede ese precio con algo vacio
                if (mejorBid->second.empty())
                    bidMap_.erase(mejorBid);

                if (mejorAsk->second.empty())
                    askMap_.erase(mejorAsk);

            } else {
                break;
            }
        }

        return transacciones;
    }


    // Si la oferta remanente es de la propia bateria (no se vendio), la carga vuelve sin generar una transaccion contra la bateria misma.
    vector<TransaccionEnergia> liquidarExcedenteABateria(NodoBateria& bateria, double precioBaseHorario) {
        vector<TransaccionEnergia> transacciones;

        for (auto& [precio, cola] : askMap_) {
            while (!cola.empty()) {
                const Orden& orden = cola.front();
                if (orden.idNodo == bateria.getId()) {
                    bateria.absorberExcedente(orden.kwh);
                } else {
                    transacciones.emplace_back(orden.idNodo, bateria.getId(), orden.kwh, precioBaseHorario);
                    bateria.absorberExcedente(orden.kwh);
                }
                cola.pop();
            }
        }
        askMap_.clear();

        return transacciones;
    }

    void limpiarLibro() {
        if (!bidMap_.empty()) {
            for (const auto& [precio, cola] : bidMap_) {
                cout << "Demanda insatisfecha: " << cola.size()
                     << " orden de compra a precio " << precio << " sin vendedor." << endl;
            }
        }
        bidMap_.clear();
        askMap_.clear();
    }


    vector<TransaccionEnergia> procesarTick(const vector<Orden>& ofertasCSV,
                                             NodoBateria& bateria,
                                             double precioBaseHorario,
                                             uint64_t& secuencia) {
        if (bateria.getCargaActual() > 0.001) {
            ofertarEnergiaBateria(bateria, precioBaseHorario, secuencia++);
        }

        cargarOrdenes(ofertasCSV);

        vector<TransaccionEnergia> transacciones = ejecutarMatching();
        vector<TransaccionEnergia> excedente = liquidarExcedenteABateria(bateria, precioBaseHorario);
        transacciones.insert(transacciones.end(), excedente.begin(), excedente.end()); //inserta al final de transacciones todo lo que esta en excedente

        limpiarLibro();

        return transacciones;
    }

    void imprimirLibro() const {
        cout << fixed << setprecision(2);
        cout << "bidMap (compras)" << endl;
        for (const auto [precio, cola] : bidMap_) {
            cout << " precio=" << precio
                      << "----" << cola.size() << " ordenes en cola" << endl;
        }
        cout << "askMap (ventas)" << endl;
        for (const auto [precio, cola] : askMap_) {
            cout << "precio=" << precio
                      << "----" << cola.size() << " ordenes en cola" << endl;
        }
    }


};


