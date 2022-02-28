#include <iostream>
#include <complex>
#include <vector>
#define MAX 4096

using namespace std;

#define M_PI 3.1415926535897932384

int log2(int N)    /*function to calculate the log2(.) of int numbers*/
{
	int k = N, i = 0;
	while (k) {
		k >>= 1;
		i++;
	}
	return i - 1;
}

int checkPow2(int n)    //checking if the number of element is a power of 2
{
	return n > 0 && (n & (n - 1)) == 0;
}

int reverse(int N, int n)    //calculating bit reverse number
{
	int j, p = 0;
	for (j = 1; j <= log2(N); j++) {
		if (n & (1 << (log2(N) - j)))
			p |= 1 << (j - 1);
	}
	return p;
}

void ordina(complex<double>* f1, int N) //using the bit reverse order in the array
{
	//vector<bool>unused_index(N, true);
	//complex<double> holder;
	//int index;
	complex<double> f2[MAX];
	for (int i = 0; i < N; i++)
	{
		f2[i] = f1[reverse(N, i)];
		/*if (unused_index[i])
		{
			index = reverse(N, i);
			holder = f1[i];
			f1[i] = f1[index];
			f1[index] = holder;
			unused_index[index] = false;
		}*/
	}	
	for (int i = 0; i < N; i++)
		f1[i] = f2[i];
	
}

void transform(complex<double>* f, int N) //
{
	ordina(f, N);    //first: reverse order
	complex<double>* W = new complex<double>[N / 2];//(complex<double> *)malloc(N / 2 * sizeof(complex<double>));
	W[1] = polar(1., -2. * M_PI / N);
	W[0] = 1;
	for (int i = 2; i < N / 2; i++)
		W[i] = pow(W[1], i);
	int n = 1;
	int a = N / 2;
	for (int j = 0; j < log2(N); j++) 
	{
		for (int i = 0; i < N; i++) 
		{
			if (!(i & n)) 
			{
				complex<double> temp = f[i];
				complex<double> Temp = W[(i * a) % (n * a)] * f[i + n];
				f[i] = temp + Temp;
				f[i + n] = temp - Temp;
			}
		}
		n *= 2;
		a = a / 2;
	}
}

void fft(complex<double>* f, int N, double d)
{
	transform(f, N);
	for (int i = 0; i < N; i++)
		f[i] *= d; //multiplying by step
}
void ifft(complex<double>* f, int N, double d)
{
	size_t i;
	for (i = 0; i < N; i++)
		f[i] = std::conj(f[i]);
	fft(f, N, d);
	d = N;
	for (i = 0; i < N; i++)
		f[i] = std::conj(f[i])/d;
}