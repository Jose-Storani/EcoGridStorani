#pragma once

#include "NodoRed.h"

using namespace std;

enum class PerfilConsumo {
    Residencial,
    Comercial,
    Industrial
};

string perfilToString(PerfilConsumo p) {
    switch (p) {
        case PerfilConsumo::Residencial:
            return "Residencial";
        case PerfilConsumo::Comercial:
            return "Comercial";
        case PerfilConsumo::Industrial:
            return "Industrial";
        default:
            return "No existe";
    };
};

class NodoConsumidor : public NodoRed {
    private:
        double consumo_;
        PerfilConsumo perfil_;
    public:
        NodoConsumidor(int id, string ubicacion, double saldo,
                       double consumo, PerfilConsumo perfil)
            : NodoRed(id, ubicacion, 0.0, saldo)
        {
            consumo_ = consumo;
            perfil_ = perfil;
        }

        //excedente siempre <= 0 porque no produce, solo consume
        double calcularExcedente() const override {
            return -consumo_;
        }

        string getTipo() const override {
            return "Consumidor";
        }

        void infoNodo() const override {
            NodoRed::infoNodo();
            cout << "perfil=" << perfilToString(perfil_)
                 << " consumo=" << consumo_ << " kWh" << endl;
        }

        double getConsumo() const {
            return consumo_;
             };

        PerfilConsumo getPerfil() const {
            return perfil_;
            };

        void setConsumo(double nuevoConsumo) {
             consumo_ = nuevoConsumo;
             };

};
