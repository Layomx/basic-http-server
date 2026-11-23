# Servidor HTTP básico

Un servidor HTTP sencillo escrito en C para Linux. Sirve `index.html`
en la ruta `/` y devuelve respuestas `404` o `405` para rutas o métodos
no admitidos.

Compila y ejecuta el servidor con el puerto predeterminado:

```bash
make
./server
curl http://localhost:8080
```

También puedes indicar otro puerto:

```bash
./server 9090
curl http://localhost:9090
```

Luego puedes abrir http://localhost:8080 desde el navegador.
