#include <iostream>
#include <memory>
#include <vector>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include "ecogrid.h"

using namespace std;

void imprimirSeparador(const string titulo) {
    cout << "\n========== " << titulo << " ==========" << endl;
}
int main() {
    /*try {
        // Esto crea (o abre) el archivo en el disco
        soci::session sql(soci::sqlite3, "tp.db");
        cout << "Conexión a SQLite exitosa." << endl;

        // Acá va el resto de tu código que ya hiciste...

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }*/
    try {
        soci::session sql(soci::sqlite3, "ecogrid.db");
        std::cout << "Conexion establecida correctamente.\n";

        // Habilitar el chequeo de claves foraneas (SQLite lo trae apagado por defecto)
        sql << "PRAGMA foreign_keys = ON";

        // Borra las tablas si ya existian, para poder correr el script varias veces
        sql << "DROP TABLE IF EXISTS TRANSACCIONES";
        sql << "DROP TABLE IF EXISTS LECTURAS_HISTORICAS";
        sql << "DROP TABLE IF EXISTS CONFIG_TARIFAS";
        sql << "DROP TABLE IF EXISTS NODOS";

        sql <<
            "CREATE TABLE NODOS ("
            "    id_nodo INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    id_persona INTEGER,"
            "    ubicacion TEXT NOT NULL,"
            "    tipo TEXT CHECK (tipo IN ('Consumidor','Prosumidor','Bateria')),"
            "    saldo_cuenta REAL DEFAULT 0 CHECK (saldo_cuenta >= 0),"
            "    perfil_consumo TEXT"
            ")";

        sql <<
            "CREATE TABLE LECTURAS_HISTORICAS ("
            "    id_lectura INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    id_nodo INTEGER NOT NULL,"
            "    tick_hora TEXT NOT NULL,"
            "    produccion_kwh REAL DEFAULT 0,"
            "    consumo_kwh REAL DEFAULT 0,"
            "    excedente_neto REAL,"
            "    FOREIGN KEY (id_nodo) REFERENCES NODOS(id_nodo)"
            ")";

        sql <<
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

        sql <<
            "CREATE TABLE CONFIG_TARIFAS ("
            "    hora INTEGER PRIMARY KEY CHECK (hora BETWEEN 0 AND 23),"
            "    precio_base_kwh REAL NOT NULL"
            ")";

        sql <<
            "CREATE TRIGGER trg_validar_saldo "
            "BEFORE INSERT ON TRANSACCIONES "
            "FOR EACH ROW "
            "BEGIN "
            "    SELECT CASE "
            "        WHEN (SELECT saldo_cuenta FROM NODOS WHERE id_nodo = NEW.id_comprador) < (NEW.kwh * NEW.precio_unitario) "
            "        THEN RAISE(ABORT, 'Saldo insuficiente para realizar la compra') "
            "    END; "
            "END";

        std::cout << "Esquema creado correctamente (4 tablas + trigger).\n";

        // --- Prueba rapida (podes borrar este bloque una vez que confirmes que anda) ---
        sql << "INSERT INTO NODOS (ubicacion, tipo, saldo_cuenta) VALUES ('Casa A', 'Consumidor', 5)";
        sql << "INSERT INTO NODOS (ubicacion, tipo, saldo_cuenta) VALUES ('Casa B', 'Prosumidor', 100)";

        // Esta insercion deberia fallar: comprador (id 1) tiene saldo 5, compra cuesta 10*1=10
        try {
            sql << "INSERT INTO TRANSACCIONES (id_vendedor, id_comprador, kwh, precio_unitario) VALUES (2, 1, 10, 1)";
            std::cout << "ERROR: se esperaba que el trigger rechazara esta insercion.\n";
        } catch (soci::soci_error const& e) {
            std::cout << "Trigger funciono correctamente, rechazo la insercion: " << e.what() << "\n";
        }

        // Esta insercion deberia funcionar: comprador (id 2) tiene saldo 100, compra cuesta 5*1=5
        sql << "INSERT INTO TRANSACCIONES (id_vendedor, id_comprador, kwh, precio_unitario) VALUES (1, 2, 5, 1)";
        std::cout << "Insercion valida realizada correctamente.\n";

    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

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

    // si la bater�a tiene carga del tick anterior, la oferta primero
    if (bateria.getCargaActual() > 0.001) {
        double precioBase = 2.5; // por ahora fijo, despu�s viene de BD
        gridManager.ofertarEnergiaBateria(bateria, precioBase, secuencia++);
    }

    //leer las �rdenes del CSV
    vector<Orden> ordenes = CSVParser::leerOfertas(hora, secuencia);
    gridManager.cargarOrdenes(ordenes);

    //ejecutar el matching
    vector<TransaccionEnergia> transacciones = gridManager.ejecutarMatching();
    cout << "Transacciones: " << transacciones.size() << endl;
    for (const auto& trans : transacciones) {
        trans.imprimir();
    }

    // excedente restante va a la bater�a
    double excedente = gridManager.calcularExcedenteNoVendido();
    if (excedente > 0.001) {
        cout << "Excedente a bateria: " << excedente << " kWh" << endl;
        bateria.absorberExcedente(excedente); //revisar calculo, el excedente es muy alto al final decada tick
    }

    //limpiar el libro para el pr�ximo tick
    gridManager.limpiarLibro();
}


}
