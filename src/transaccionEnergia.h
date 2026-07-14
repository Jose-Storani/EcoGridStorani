#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>

using namespace std;

class TransaccionEnergia {
    private:
    int      idVendedor_;
    int      idComprador_;
    double   kwh_;
    double   precio_;
    chrono::system_clock::time_point timestamp_;
    public:
        TransaccionEnergia(int idVendedor, int idComprador,
                            double kwh, double precio)
        {
            idVendedor_ = idVendedor;
            idComprador_ = idComprador;
            kwh_ = kwh;
            precio_ = precio;
            timestamp_ = chrono::system_clock::now();
        }

    int    getIdVendedor()  const { return idVendedor_; }
    int    getIdComprador() const { return idComprador_; }
    double getKwh()         const { return kwh_; }
    double getPrecio()      const { return precio_; }

    chrono::system_clock::time_point getTimestamp() const {
        return timestamp_;
    }

    double getCostoTotal() const { return kwh_ * precio_; }

        void imprimir() const {
        cout << fixed << setprecision(2)
                  << "Vendedor=" << idVendedor_
                  << " -> Comprador=" << idComprador_
                  << " | kwh=" << kwh_
                  << " | precio=" << precio_
                  << " | total=" << getCostoTotal()
                  << endl;

        }
};
