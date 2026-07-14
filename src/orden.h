#pragma once

struct Orden {
    int      idOrden;
    bool     esCompra;    // true = orden de compra | false = orden de venta
    int      idNodo;
    double   precio;
    double   kwh;
    uint64_t secuencia;   // contador FIFO

    Orden() = default;

    Orden(int idOrden_, bool esCompra_, int idNodo_,
          double precio_, double kwh_, uint64_t secuencia_)
    {
        idOrden   = idOrden_;
        esCompra  = esCompra_;
        idNodo    = idNodo_;
        precio    = precio_;
        kwh       = kwh_;
        secuencia = secuencia_;
    }
};

