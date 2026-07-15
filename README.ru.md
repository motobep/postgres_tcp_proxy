# Логирующий Postgres tcp прокси сервер

Прокси-сервер PostgreSQL на C++, который записывает SQL-запросы клиента в лог-файл.<br>
Расположение лог-файла по умолчанию: "/tmp/sql_queries.log" (см. Makefile).<br>
*Убедитесь, что SSL отключен в PostgreSQL.*<br><br>
\([English README](README.md)\)

## Зависимости

- gcc, g++
- make
- linux libraries (epoll)
- postgres, createdb, pgbench
- optional: docker, python

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

Опционально: В 3-ем терминале (для теста большого sql запроса):
```bash
cd py
python -m venv venv
pip install -r requirements.txt
python main.py
```

## Команды
### Билд и Запуск

```bash
make build_n_run
```

### Создание stress_db

Перед запуском стресс-теста создайте базу данных следующим образом

- Docker (Рекомендуется)

```bash
make docker_create_stress_db
```

- Native
```bash
make create_stress_db
```

### Стресс тест

После запуска прокси-сервера выполните тестовые команды в другом терминале.

- Docker (Рекомендуется)

```bash
make docker_stress_test
```

- Native

Необходимо настроить PostgreSQL, чтобы он прослушивал порт 6432.

```bash
make stress_test
```

### Тест большого sql запроса

В другом терминале
```bash
cd py
python -m venv venv
pip install -r requirements.txt
python main.py
```

## Результат

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
number of transactions actually processed: 804150
latency average = 19.328 ms
initial connection time = 109.933 ms
tps = 2586.907398 (without initial connection time)
```

