# Postgres tcp proxy logging server

C/C++ Postgres proxy that logs client's sql queries to a log file.<br>
Default log file location: "/tmp/sql_queries.log" (see Makefile).
<br><br>
Based on my project [motobep/tcp_udp_epoll_server ](https://github.com/motobep/tcp_udp_epoll_server)

## Dependencies

- gcc, g++
- make
- linux libraries (epoll)
- postgres, createdb, pgbench
- optional: docker

## Build & Run server

```bash
make build_n_run
```

## Create stress_db

Before running stress test create db like so.

### Docker (Recommended)

Don't forget to start docker
```bash
docker-compose up -d 
```

Db creation
```bash
make docker_create_stress_db
```

### Native
```bash
make create_stress_db
```

## Stress Test

Run test commands in another terminal after running the proxy server

### Docker (Recommended)

Don't forget to start docker
```bash
docker-compose up -d 
```

Stress testing
```bash
make docker_stress_test
```

### Native

Postgres must be configured, listening on 6432 port

```bash
make stress_test
```

## Example

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
number of transactions actually processed: 739881
latency average = 20.942 ms
initial connection time = 124.608 ms
tps = 2387.507435 (without initial connection time)
```
