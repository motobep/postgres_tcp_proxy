# Логирующий Postgres tcp прокси сервер

Прокси-сервер PostgreSQL на C/C++, который записывает SQL-запросы клиента в лог-файл.<br>
Расположение лог-файла по умолчанию: "/tmp/sql_queries.log" (см. Makefile).<br>
*Убедитесь, что SSL отключен в PostgreSQL.*<br><br>
Основано на моем проекте [motobep/tcp_udp_epoll_server ](https://github.com/motobep/tcp_udp_epoll_server)<br>
\([English README](README.md)\)

## Зависимости

- gcc, g++
- make
- linux libraries (epoll)
- postgres, createdb, pgbench
- optional: docker

## Билд и Запуск

```bash
make build_n_run
```

## Создание stress_db

Перед запуском стресс-теста создайте базу данных следующим образом.

### Docker (Рекомендуется)

Не забудьте запустить Docker
```bash
docker-compose up -d 
```

Создание тестовой БД
```bash
make docker_create_stress_db
```

### Native
```bash
make create_stress_db
```

## Стресс тест

После запуска прокси-сервера выполните тестовые команды в другом терминале.

### Docker (Рекомендуется)

Не забудьте запустить Docker
```bash
docker-compose up -d 
```

Стресс тестирование
```bash
make docker_stress_test
```

### Native

Необходимо настроить PostgreSQL, чтобы он прослушивал порт 6432.

```bash
make stress_test
```

## Пример запуска

В одном терминале:
```bash
make build_n_run
```

В другом:
```bash
docker-compose up -d
make docker_create_stress_db
make docker_stress_test
```


### Результат

Протестировано на: CPU 8-core 3.6 Ghz + 16GB RAM

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

