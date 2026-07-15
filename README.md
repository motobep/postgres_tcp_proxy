# Postgres tcp proxy logging server

C++ Postgres proxy that logs client's sql queries to a log file.<br>
Default log file location: "/tmp/sql_queries.log" (see Makefile).<br>
*Make sure that ssl is disabled in Postgres.*<br><br>
\([README на Русском](README.ru.md)\)

## Dependencies

- gcc, g++
- make
- linux libraries (epoll)
- postgres, createdb, pgbench
- optional: docker, python


## Usage example

In one terminal:
```bash
make build_n_run
```

In another:
```bash
docker-compose up -d
make docker_create_stress_db
make docker_stress_test
```

Optional: In a 3-rd terminal (if you want to test for a big query):
```bash
cd py
python -m venv venv
pip install -r requirements.txt
python main.py
```

## Commands
### Build & Run server

```bash
make build_n_run
```

### Create stress_db

Before running stress test create db like so.

- Docker (Recommended)

```bash
make docker_create_stress_db
```

- Native
```bash
make create_stress_db
```

### Stress Test

Run test commands in another terminal after running the proxy server

- Docker (Recommended)

```bash
make docker_stress_test
```

- Stress Test - Native

Postgres must be configured, listening on 6432 port

```bash
make stress_test
```

### Test for a big query
In another terminal
```bash
cd py
python -m venv venv
pip install -r requirements.txt
python main.py
```

### Result

Tested on: CPU 8-core 3.6 Ghz + 16GB RAM

```
pgbench (14.23 (Debian 14.23-1.pgdg13+1))
starting vacuum...end.
transaction type: <builtin: TPC-B (sort of)>
scaling factor: 5
query mode: simple
number of clients: 50
number of threads: 4
duration: 310 s
number of transactions actually processed: 804150
latency average = 19.328 ms
initial connection time = 109.933 ms
tps = 2586.907398 (without initial connection time)
```

