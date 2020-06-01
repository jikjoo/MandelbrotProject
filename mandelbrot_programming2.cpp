//#define PRINT

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mandelbrot.h"
#include <time.h>

int main(int argc, char* argv[]) {

//	printf("mandelbrot start\n");
	int np, me, col, color, pos;
	const int display_height = 400, display_width = 400;
	const int N_rectangle = 40;
	const double scale_real = (real_max - real_min) / display_width;
	const double scale_imag = (imag_max - imag_min) / display_height;
	const int m = display_width / N_rectangle; // 10
	const int data_tag = 55, result_tag = 65,termin_tag=75, master = 0;
	const int msgsize = display_height + 1; // 400 + col
	int* display = (int*)malloc(display_height * display_width * sizeof(int));
	int* recv_buf = (int*)malloc(msgsize * sizeof(int)); // colors
	int count;

	// clock
	clock_t t1, t2;
	double rtime, elapse;

	complex c;

	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	MPI_Comm_rank(MPI_COMM_WORLD, &me);
	//printf("np = %d\n", np);
	if (m == 0) {
		// refrence time
		double s = 0.0;
		t1 = clock();
		for (int i = 0; i < 100000000; i++) s += 1.0 / (rand() + 1);
		t2 = clock();
		rtime = (double)(t2 - t1) / CLOCKS_PER_SEC;
		printf("reference time = %d\n", rtime);
	}
	if (np == 1) {
		t1 = clock();
		for (int x = 0; x < display_width; x++) {
			for (int y = 0; y < display_height; y++) {
				c.real = real_min + ((double)x * scale_real);
				c.imag = imag_min + ((double)y * scale_imag);
				color = cal_pixel(c);
				pos = y * display_width + x;
				display[pos] = color;
			}
		}
	}
	else {
		col = 0;
		if (me == master) {// I am master
			t1 = clock();
			count = 0;
			for (int i = 1; i < np; i++) {
				int dest = i;
				MPI_Send(&col, 1, MPI_INT, dest, data_tag, MPI_COMM_WORLD);
#ifdef PRINT
				printf("Send col %d to process %d\n", col, dest);
#endif
				count++;
				col++;
			}
			do {
				MPI_Recv(recv_buf, msgsize, MPI_INT, MPI_ANY_SOURCE, result_tag, MPI_COMM_WORLD, &status);
				int dest = status.MPI_SOURCE;
				count--;

				if (col < display_width) {
					MPI_Send(&col, 1, MPI_INT, dest, data_tag, MPI_COMM_WORLD);
					col++;
					count++;
				}
				else MPI_Send(&col, 1, MPI_INT, dest, termin_tag, MPI_COMM_WORLD);
				for (int i = 0; i < display_height; i++) {
					pos = i * display_width + recv_buf[msgsize - 1];
					display[pos] = recv_buf[i];
				}
			} while (count > 0);
			
		}
		else {//Slave
			
				MPI_Recv(&col, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
#ifdef PRINT
				printf("Receive process %d with col = %d\n", me, col);
#endif
				while (status.MPI_TAG == data_tag) {
					pos = msgsize - 1;
					recv_buf[pos] = col;
					c.real = real_min + ((double)col * scale_real);
					for (int y = 0; y < display_height; y++) {
						c.imag = imag_min + ((double)y * scale_imag);
						color = cal_pixel(c);
						recv_buf[y] = color;
					}
					MPI_Send(recv_buf, msgsize, MPI_INT, master, result_tag, MPI_COMM_WORLD);
					MPI_Recv(&col, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				}

		}

	}
	if (me == 0) {// display
		t2 = clock();
		elapse = (double)(t2 - t1) / CLOCKS_PER_SEC;
		
		char fname[0x100];
		snprintf(fname, sizeof(fname), "display_dynamic_%d.pgm", np);
		FILE* fp;
		fopen_s(&fp, fname, "wb");
		fprintf(fp, "P2\n# %s\n400 400\n255\n", fname);
		for (int y = 0; y < display_height; y++) {
			for (int x = 0; x < display_width; x++) {
				pos = y * display_width + x;
				fprintf(fp, "%d ", display[pos]);
			}
			fprintf(fp, "\n");
		}
		printf("%s created %.2f ~ %.2f, %.2f ~%.2f \n", fname, real_max, real_min, imag_max, imag_min);
		printf("time = %.4f\n\n", elapse);
	}
	MPI_Finalize();
	exit(0);
}