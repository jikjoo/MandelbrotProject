#ifndef _MANDELBROT_
#define _MANDELBROT_

struct complex
{
	double real;
	double imag;
};
double real_max = 0.8;
double real_min = -1.6;
double imag_max = 1.2;
double imag_min = -1.2;

int cal_pixel(complex c) {
	int count, max;
	complex z;
	double temp, lengthsq = 0.0;
	max = 256;
	z.real = 0; z.imag = 0;
	count = 0;
	do {
		temp = z.real * z.real - z.imag * z.imag + c.real;
		z.imag = 2.0 * z.real * z.imag + c.imag;
		z.real = temp;
		lengthsq = z.real * z.real + z.imag * z.imag;
		count++;
	} while (lengthsq < 4.0 && count < max);
	return count - 1;
}
#endif