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

            // se descuenta apenas se oferta; si no se vende, vuelve como parte
            // del excedente no vendido al final del tick (evita duplicar la carga)
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
            auto mejorBid = bidMap_.begin(); // precio de compra m�s alto
            auto mejorAsk = askMap_.begin(); // precio de venta m�s bajo

            double precioBid = mejorBid->first;// devuelve 0 ---
            double precioAsk = mejorAsk->first; // devuelve 20 - 50

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


    double calcularExcedenteNoVendido() const {
        double total = 0.0;
        for (auto [precio, cola] : askMap_) {
            while (!cola.empty()) {
                total += cola.front().kwh;
                cola.pop();
            }
        }
        return total;
    }


    void limpiarLibro() {
        bidMap_.clear();
        askMap_.clear();
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


