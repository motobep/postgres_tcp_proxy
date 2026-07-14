CXX=g++
FLAGS=-Wall
SRC=./src


build_n_run:
	make build && make run

build: $(SRC)/server.cpp
	$(CXX) $(FLAGS) $(SRC)/server.cpp $(SRC)/logger.cpp -o server

run: server
	./server "/tmp/sql_queries.log" "0.0.0.0" "8888"

clean:
	rm server



### Stress testing ###

# Native
create_stress_db:
	PGUSER=postgres PGPASSWORD="1234" createdb stress_db && PGUSER=postgres PGPASSWORD="1234" pgbench -i -s 5 stress_db

stress_test:
	PGUSER=postgres PGPASSWORD="1234" pgbench -p 8888 -c 50 -j 4 -T 310 stress_db

# Docker
docker_create_stress_db:
	docker exec -it postgres-container sh -c 'PGUSER=postgres PGPASSWORD="1234" createdb stress_db && PGUSER=postgres PGPASSWORD="1234" pgbench -i -s 5 stress_db'

docker_stress_test:
	docker exec -it postgres-container sh -c 'PGUSER=postgres PGPASSWORD="1234" pgbench -h host.docker.internal -p 8888 -c 50 -j 4 -T 310 stress_db'
