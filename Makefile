serial:
	g++ equalize.cpp -lX11 -lpthread -o serial
omp:
	g++ equalize-omp.cpp -lX11 -lpthread -fopenmp -o omp
mpi:
	mpic++ equalize-mpi.cpp -lX11 -lpthread -o mpi
clean:
	rm -f mpi omp serial
