#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mandelbrot.h"

int main(int argc, char* argv[]) {

	printf("mandelbrot start\n");
	int np, me, col, color, pos;
	const int display_height = 400, display_width = 400;
	const int N_rectangle = 40;
	const double scale_real = (real_max - real_min) / display_width;
	const double scale_imag = (imag_max - imag_min) / display_height;
	const int m = display_width / N_rectangle; // 10
	const int tag_col = 55, tag_result = 65, master = 0;
	const int msgsize = N_rectangle * display_height + 1; // 40 * 400 + col
	int* display = (int*)malloc(display_height * display_width * sizeof(int));
	int* recv_buf = (int*)malloc(msgsize * sizeof(int)); // colors
	int task = m; // task remained

	complex c;

	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	MPI_Comm_rank(MPI_COMM_WORLD, &me);
	printf("np = %d\n", np);
	if (np == 1) {
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
		if (me == master) {// I am master
			col = 0;
			for (int i = 1; i < np; i++, col += N_rectangle,task--) {
				int dest = i % (np - 1) + 1;
				MPI_Send(&col, 1, MPI_INT, dest, tag_col, MPI_COMM_WORLD);
				printf("Send col %d to process %d\n", col, dest);
			}
			for (int i = 0; i < m; i++) {
				MPI_Recv(recv_buf, msgsize, MPI_INT, MPI_ANY_SOURCE, tag_result, MPI_COMM_WORLD, &status);
				col = recv_buf[msgsize - 1];
				for (int x = 0; x < N_rectangle; x++) {
					for (int y = 0; y < display_height; y++) {
						int temp = y + display_height + x;
						pos = temp + col * N_rectangle;
						display[pos] = recv_buf[temp];
					}
				}
			}
		}
		else {//Slave
			MPI_Recv(&col, 1, MPI_INT, master, tag_col, MPI_COMM_WORLD, &status);
			printf("Receive process %d with col = %d\n", me, col);
			recv_buf[msgsize - 1] = col;
			for (int x = col; x < col + N_rectangle; x++) {
				for (int y = 0; y < display_height; y++) {
					c.real = real_min + ((double)x * scale_real);
					c.imag = imag_min + ((double)y * scale_imag);
					color = cal_pixel(c);
					pos = y * N_rectangle + x;
					recv_buf[pos] = color;
				}
			}
			MPI_Send(recv_buf, msgsize, MPI_INT, master, tag_result, MPI_COMM_WORLD);
		}

	}
	MPI_Finalize();
	char fname[0x100];
	snprintf(fname, sizeof(fname), "display_static_%d.pgm", np);
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
	exit(0);
}