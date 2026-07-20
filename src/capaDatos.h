#pragma once

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "nodoRed.h"
#include "transaccionEnergia.h"

using namespace std;

//saldo alto para que el trigger no rechace
const double SALDO_ILIMITADO_BATERIA = 999999.0;

class CapaDatos {
private:
    soci::session sql_;

public:
    CapaDatos(const string& dbPath) : sql_(soci::sqlite3, dbPath) {
        sql_ << "PRAGMA foreign_keys = ON";
    }

    void inicializarEsquema() {
        sql_ << "DROP TABLE IF EXISTS TRANSACCIONES";
        sql_ << "DROP TABLE IF EXISTS LECTURAS_HISTORICAS";
        sql_ << "DROP TABLE IF EXISTS CONFIG_TARIFAS";
        sql_ << "DROP TABLE IF EXISTS NODOS";

        sql_ <<
            "CREATE TABLE NODOS ("
            "    id_nodo INTEGER PRIMARY KEY,"
            "    ubicacion TEXT NOT NULL,"
            "    tipo TEXT CHECK (tipo IN ('Consumidor','Prosumidor','Bateria')),"
            "    saldo_cuenta REAL DEFAULT 0 CHECK (saldo_cuenta >= 0),"
            "    perfil_consumo TEXT"
            ")";

        sql_ <<
            "CREATE TABLE LECTURAS_HISTORICAS ("
            "    id_lectura INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    id_nodo INTEGER NOT NULL,"
            "    tick_hora TEXT NOT NULL,"
            "    produccion_kwh REAL DEFAULT 0,"
            "    consumo_kwh REAL DEFAULT 0,"
            "    excedente_neto REAL,"
            "    FOREIGN KEY (id_nodo) REFERENCES NODOS(id_nodo)"
            ")";

        sql_ <<
            "CREATE TABLE TRANSACCIONES ("
            "    id_transaccion INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    id_vendedor INTEGER NOT NULL,"
            "    id_comprador INTEGER NOT NULL,"
            "    kwh REAL NOT NULL,"
            "    precio_unitario REAL NOT NULL,"
            "    fecha_transaccion TEXT DEFAULT CURRENT_TIMESTAMP,"
            "    FOREIGN KEY (id_vendedor) REFERENCES NODOS(id_nodo),"
            "    FOREIGN KEY (id_comprador) REFERENCES NODOS(id_nodo),"
            "    CONSTRAINT chk_kwh_pos CHECK (kwh > 0),"
            "    CONSTRAINT chk_precio_pos CHECK (precio_unitario > 0)"
            ")";

        sql_ <<
            "CREATE TABLE CONFIG_TARIFAS ("
            "    hora INTEGER PRIMARY KEY CHECK (hora BETWEEN 0 AND 23),"
            "    precio_base_kwh REAL NOT NULL"
            ")";

        sql_ <<
            "CREATE TRIGGER trg_validar_saldo "
            "BEFORE INSERT ON TRANSACCIONES "
            "FOR EACH ROW "
            "BEGIN "
            "    SELECT CASE "
            "        WHEN (SELECT saldo_cuenta FROM NODOS WHERE id_nodo = NEW.id_comprador) < (NEW.kwh * NEW.precio_unitario) "
            "        THEN RAISE(ABORT, 'Saldo insuficiente para realizar la compra') "
            "    END; "
            "END";

        cout << "Esquema creado\n";
    }


    void sembrarNodos(const vector<unique_ptr<NodoRed>>& nodos) {
        for (const auto& nodo : nodos) {
            int id = nodo->getId();
            string ubicacion = nodo->getUbicacion();
            string tipo = nodo->getTipo();
            double saldo = (tipo == "Bateria") ? SALDO_ILIMITADO_BATERIA : nodo->getSaldoCuenta();
            string perfil = nodo->getPerfilConsumoStr();

            sql_ << "INSERT INTO NODOS (id_nodo, ubicacion, tipo, saldo_cuenta, perfil_consumo) "
                    "VALUES (:id, :ubic, :tipo, :saldo, :perfil)",
                soci::use(id), soci::use(ubicacion), soci::use(tipo),
                soci::use(saldo), soci::use(perfil);
        }

        cout << nodos.size() << " nodos sembrados en NODOS.\n";
    }

    void sembrarTarifas() {
        const vector<pair<int, double>> tarifasEjemplo = {
            {10, 1.00}, {12, 1.50}, {14, 1.00}, {15, 1.20}, {18, 2.00}
        };
        for (const auto& [hora, precio] : tarifasEjemplo) {
            sql_ << "INSERT INTO CONFIG_TARIFAS (hora, precio_base_kwh) VALUES (:hora, :precio)",
                soci::use(hora), soci::use(precio);
        }
        cout << "Tarifas de ejemplo sembradas en CONFIG_TARIFAS.\n";
    }

    double obtenerPrecioBase(int hora, double porDefecto = 1.5) {
        double precio = porDefecto;
        try {
            sql_ << "SELECT precio_base_kwh FROM CONFIG_TARIFAS WHERE hora = :hora",
                soci::use(hora), soci::into(precio);
        } catch (const soci::soci_error&) {
            cerr << "No hay tarifa configurada para la hora " << hora
                 << ", se usa el valor por defecto " << porDefecto << ".\n";
            precio = porDefecto;
        }
        return precio;
    }

    bool persistirTransacciones(const vector<TransaccionEnergia>& transacciones, int horaSimulada) {
        if (transacciones.empty()) return true;

        soci::transaction tr(sql_);
        try {
            for (const auto& t : transacciones) {
                int idVendedor = t.getIdVendedor();
                int idComprador = t.getIdComprador();
                double kwh = t.getKwh();
                double precio = t.getPrecio();

                sql_ << "INSERT INTO TRANSACCIONES (id_vendedor, id_comprador, kwh, precio_unitario) "
                        "VALUES (:v, :c, :kwh, :precio)",
                    soci::use(idVendedor), soci::use(idComprador),
                    soci::use(kwh), soci::use(precio);

                actualizarSaldoYLecturas(idVendedor, kwh, precio, "VENTA", horaSimulada);
                actualizarSaldoYLecturas(idComprador, kwh, precio, "COMPRA", horaSimulada);
            }
            tr.commit();
            return true;
        } catch (const soci::soci_error& e) {
            tr.rollback();
            cerr << "Tick rechazado, se ejecuto ROLLBACK: " << e.what() << "\n";
            return false;
        }
    }

private:
    void actualizarSaldoYLecturas(int idNodo, double kwh, double precio,
                                   const string& tipoOperacion, int horaSimulada) {
        double monto = kwh * precio;

        if (tipoOperacion == "COMPRA") {
            sql_ << "UPDATE NODOS SET saldo_cuenta = saldo_cuenta - :m WHERE id_nodo = :id",
                soci::use(monto), soci::use(idNodo);
            double excedenteNeto = -kwh;
            sql_ << "INSERT INTO LECTURAS_HISTORICAS (id_nodo, tick_hora, consumo_kwh, excedente_neto) "
                    "VALUES (:id, :hora, :kwh, :exc)",
                soci::use(idNodo), soci::use(horaSimulada), soci::use(kwh), soci::use(excedenteNeto);
        } else {
            sql_ << "UPDATE NODOS SET saldo_cuenta = saldo_cuenta + :m WHERE id_nodo = :id",
                soci::use(monto), soci::use(idNodo);
            sql_ << "INSERT INTO LECTURAS_HISTORICAS (id_nodo, tick_hora, produccion_kwh, excedente_neto) "
                    "VALUES (:id, :hora, :kwh, :exc)",
                soci::use(idNodo), soci::use(horaSimulada), soci::use(kwh), soci::use(kwh);
        }
    }
};
