#pragma once
#include "NodoRed.h"
class NodoProsumidor : public NodoRed{
    private:
    double produccion_;
    double consumo_;

    public:
        NodoProsumidor(int id, string ubicacion, double saldo, double produccion, double consumo)
            :NodoRed(id,ubicacion,0.0,saldo){
                produccion_ = produccion;
                consumo_ = consumo;
            };

    //si es positivo, vendo
    //si es negativo, necesito comprar
    double calcularExcedente() const override{
        return produccion_ - consumo_;
    }

    string getTipo() const override {
        return "Prosumidor";
    }

    void infoNodo() const override {
        NodoRed::infoNodo();
        cout << "produccion=" << produccion_ << " kWh"
                  << "consumo=" << consumo_ << " kWh"
                  << "excedente=" << calcularExcedente() << " kWh"
                  << endl;
    }

    double getProduccion() const {
        return produccion_; }
    double getConsumo()    const {
        return consumo_; }

    void setProduccion(double nuevaProduccion) {
        produccion_ = nuevaProduccion; }
    void setConsumo(double nuevoConsumo){
         consumo_ = nuevoConsumo; }




};
