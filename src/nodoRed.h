#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using namespace std;

class NodoRed {
private:
    int         id_;
    string      ubicacion_;
    double      balanceEnergia_;
    double      saldoCuenta_;

protected:
    NodoRed(int id, string ubicacion, double balance, double saldo)
    {
        id_ = id;
        ubicacion_ = ubicacion;
        balanceEnergia_ = balance;
        saldoCuenta_ = saldo;

        if (saldo < 0.0)
            throw invalid_argument("El saldo no puede ser negativo.");
    }


public:
    virtual double calcularExcedente() const = 0;

    virtual void actualizarBalance(double variacionKwh) {
         balanceEnergia_ += variacionKwh;
    };

    virtual string getTipo() const = 0;

    virtual void infoNodo() const {
        cout << fixed << setprecision(2)
                  << "id=" << id_
                  << " tipo=" << getTipo()
                  << " ubicacion=" << ubicacion_
                  << " balance=" << balanceEnergia_ << " kW"
                  << " saldo=" << saldoCuenta_ << " creditos"
                  << endl;
    }

    virtual ~NodoRed() = default; //destructor

    int getId() const {
         return id_; }
    string getUbicacion() const {
         return ubicacion_; }
    double getBalanceEnergia() const {
         return balanceEnergia_; }
    double getSaldoCuenta() const {
         return saldoCuenta_; }


    void setSaldoCuenta(double nuevoSaldo) {
        if (nuevoSaldo < 0.0)
            throw invalid_argument(
                "Saldo negativo no permitido para nodo id=" +
                to_string(id_));
        saldoCuenta_ = nuevoSaldo;
    }

    void ajustarSaldo(double variacionCreditos) {
        setSaldoCuenta(saldoCuenta_ + variacionCreditos);
    }

    void setBalanceEnergia(double balance) {
        balanceEnergia_ = balance;
    }
};

