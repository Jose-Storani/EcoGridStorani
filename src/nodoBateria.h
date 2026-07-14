#pragma once
#include "NodoRed.h"
#include <stdexcept>

class NodoBateria : public NodoRed {
    private:
    double cargaActual_;

    public:
        NodoBateria(int id, string ubicacion, double saldo,
                            double cargaInicial = 0.0)
            : NodoRed(id, ubicacion, 0.0, saldo){
                cargaActual_ = cargaInicial;
            }
        double calcularExcedente() const override {
            return cargaActual_;
            }

        string getTipo() const override {
        return "Bateria";
        }

        void infoNodo() const override {
            NodoRed::infoNodo();
            cout << "cargaActual=" << cargaActual_ << " kWh"
                      <<endl;
        }

        void absorberExcedente(double kwh) {
            if (kwh < 0.0)
                throw invalid_argument("No se puede absorber una cantidad negativa de energía.");
            cargaActual_ += kwh;
        }

        void liberarEnergia(double kwh) {
            if (kwh < 0.0)
                throw invalid_argument("No se puede liberar una cantidad negativa de energía.");
            if (kwh > cargaActual_)
                throw invalid_argument("No se puede liberar más energía de la que la batería tiene acumulada.");
            cargaActual_ -= kwh;
        }

        double getCargaActual() const { return cargaActual_; }

};
