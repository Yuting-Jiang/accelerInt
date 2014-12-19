/** Main function file for exponential integration of H2 problem project.
 * \file main.c
 *
 * \author Kyle E. Niemeyer
 * \date 08/27/2013
 *
 * Contains main and integration driver functions.
 */
 
/** Include common code. */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <complex.h>

/** Include CUDA libraries. */
#include <cuda.h>
#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <cuComplex.h>

#include "header.h"
#include "exprb43.cuh"
#include "mass_mole.h"
#include "timer.h"

#include "rates.cuh"

#include "cuda_profiler_api.h"
#include "cudaProfiler.h"

// load same initial conditions for all threads
#define SAME_IC
#define REPEATS 1

// shuffle initial conditions randomly
//#define SHUFFLE


static inline double getSign()
{
	return ((double)rand()/(double)RAND_MAX) > 0.5 ? 1.0 : -1.0;
}

static inline double getRand()
{
	return ((double)rand()/(double)RAND_MAX);// * getSign();
}

/////////////////////////////////////////////////////////////////////////////

bool errorCheck (cudaError_t status) {
    if (status != cudaSuccess) {
        printf ("%s\n", cudaGetErrorString(status));
        return false;
    }
    return true;
}

void populate(int NUM, Real pres, Real* y, Real* conc_arrays)
{
	// mass-averaged density
	Real rho;
	rho = (y[1] / 2.01594) + (y[2] / 1.00797) + (y[3] / 15.9994) + (y[4] / 31.9988)
	  + (y[5] / 17.00737) + (y[6] / 18.01534) + (y[7] / 33.00677) + (y[8] / 34.01474)
	  + (y[9] / 12.01115) + (y[10] / 13.01912) + (y[11] / 14.02709) + (y[12] / 14.02709)
	  + (y[13] / 15.03506) + (y[14] / 16.04303) + (y[15] / 28.01055) + (y[16] / 44.00995)
	  + (y[17] / 29.01852) + (y[18] / 30.02649) + (y[19] / 31.03446) + (y[20] / 31.03446)
	  + (y[21] / 32.04243) + (y[22] / 25.03027) + (y[23] / 26.03824) + (y[24] / 27.04621)
	  + (y[25] / 28.05418) + (y[26] / 29.06215) + (y[27] / 30.07012) + (y[28] / 41.02967)
	  + (y[29] / 42.03764) + (y[30] / 42.03764) + (y[31] / 14.0067) + (y[32] / 15.01467)
	  + (y[33] / 16.02264) + (y[34] / 17.03061) + (y[35] / 29.02137) + (y[36] / 30.0061)
	  + (y[37] / 46.0055) + (y[38] / 44.0128) + (y[39] / 31.01407) + (y[40] / 26.01785)
	  + (y[41] / 27.02582) + (y[42] / 28.03379) + (y[43] / 41.03252) + (y[44] / 43.02522)
	  + (y[45] / 43.02522) + (y[46] / 43.02522) + (y[47] / 42.01725) + (y[48] / 28.0134)
	  + (y[49] / 39.948) + (y[50] / 43.08924) + (y[51] / 44.09721) + (y[52] / 43.04561)
	  + (y[53] / 44.05358);
	rho = pres / (8.31451000e+07 * y[0] * rho);

	// species molar concentrations
	Real conc[53];
	conc[0] = rho * y[1] / 2.01594;
	conc[1] = rho * y[2] / 1.00797;
	conc[2] = rho * y[3] / 15.9994;
	conc[3] = rho * y[4] / 31.9988;
	conc[4] = rho * y[5] / 17.00737;
	conc[5] = rho * y[6] / 18.01534;
	conc[6] = rho * y[7] / 33.00677;
	conc[7] = rho * y[8] / 34.01474;
	conc[8] = rho * y[9] / 12.01115;
	conc[9] = rho * y[10] / 13.01912;
	conc[10] = rho * y[11] / 14.02709;
	conc[11] = rho * y[12] / 14.02709;
	conc[12] = rho * y[13] / 15.03506;
	conc[13] = rho * y[14] / 16.04303;
	conc[14] = rho * y[15] / 28.01055;
	conc[15] = rho * y[16] / 44.00995;
	conc[16] = rho * y[17] / 29.01852;
	conc[17] = rho * y[18] / 30.02649;
	conc[18] = rho * y[19] / 31.03446;
	conc[19] = rho * y[20] / 31.03446;
	conc[20] = rho * y[21] / 32.04243;
	conc[21] = rho * y[22] / 25.03027;
	conc[22] = rho * y[23] / 26.03824;
	conc[23] = rho * y[24] / 27.04621;
	conc[24] = rho * y[25] / 28.05418;
	conc[25] = rho * y[26] / 29.06215;
	conc[26] = rho * y[27] / 30.07012;
	conc[27] = rho * y[28] / 41.02967;
	conc[28] = rho * y[29] / 42.03764;
	conc[29] = rho * y[30] / 42.03764;
	conc[30] = rho * y[31] / 14.0067;
	conc[31] = rho * y[32] / 15.01467;
	conc[32] = rho * y[33] / 16.02264;
	conc[33] = rho * y[34] / 17.03061;
	conc[34] = rho * y[35] / 29.02137;
	conc[35] = rho * y[36] / 30.0061;
	conc[36] = rho * y[37] / 46.0055;
	conc[37] = rho * y[38] / 44.0128;
	conc[38] = rho * y[39] / 31.01407;
	conc[39] = rho * y[40] / 26.01785;
	conc[40] = rho * y[41] / 27.02582;
	conc[41] = rho * y[42] / 28.03379;
	conc[42] = rho * y[43] / 41.03252;
	conc[43] = rho * y[44] / 43.02522;
	conc[44] = rho * y[45] / 43.02522;
	conc[45] = rho * y[46] / 43.02522;
	conc[46] = rho * y[47] / 42.01725;
	conc[47] = rho * y[48] / 28.0134;
	conc[48] = rho * y[49] / 39.948;
	conc[49] = rho * y[50] / 43.08924;
	conc[50] = rho * y[51] / 44.09721;
	conc[51] = rho * y[52] / 43.04561;
	conc[52] = rho * y[53] / 44.05358;

  	for(int j = 0; j < NSP; j++)
  	{
		for (int i = 0; i < NUM; i++)
		{
			conc_arrays[i + j * NUM] = conc[j];
  		}
	}
}

#define tid (threadIdx.x + (blockDim.x * blockIdx.x))
__global__
void rxnrates_driver (const int NUM, const Real* T_global, const Real* conc, Real* fwd_rates, Real* rev_rates) {

	Real local_fwd_rates[FWD_RATES];
	Real local_rev_rates[REV_RATES];
	// local array with initial values
	Real local_conc[NSP];

	// load local array with initial values from global array
	#pragma unroll
	for (int i = 0; i < NSP; i++)
	{
		local_conc[i] = conc[tid + i * NUM];
	}
	#pragma unroll
	for (int repeat = 0; repeat < REPEATS; repeat++)
	{
		if (tid < NUM)
		{
			eval_rxn_rates (T_global[tid], local_conc, local_fwd_rates, local_rev_rates);
		}
	}
	//copy back
	for (int i = 0; i < FWD_RATES; i++)
	{
		fwd_rates[tid + i * NUM] = local_fwd_rates[i];
	}

	//copy back
	for (int i = 0; i < REV_RATES; i++)
	{
		rev_rates[tid + i * NUM] = local_rev_rates[i];
	}
}

__global__
void presmod_driver (const int NUM, const Real* T_global, const Real* pr_global, const Real* conc, Real* pres_mod) {

	Real local_pres_mod[PRES_MOD_RATES];
	// local array with initial values
	Real local_conc[NSP];

	// load local array with initial values from global array
	#pragma unroll
	for (int i = 0; i < NSP; i++)
	{
		local_conc[i] = conc[tid + i * NUM];
	}

	#pragma unroll
	for (int repeat = 0; repeat < REPEATS; repeat++)
	{
		if (tid < NUM)
		{
			get_rxn_pres_mod (T_global[tid], pr_global[tid], local_conc, local_pres_mod);
		}
	}
	//copy back
	for (int i = 0; i < PRES_MOD_RATES; i++)
	{
		pres_mod[tid + i * NUM] = local_pres_mod[i];
	}
}

__global__
void specrates_driver (const int NUM, const Real* fwd_rates, const Real* rev_rates, const Real* pres_mod, Real* spec_rates) {

	Real local_fwd_rates[FWD_RATES];
	Real local_rev_rates[REV_RATES];
	Real local_pres_mod[PRES_MOD_RATES];
	// local array with initial values
	Real local_spec_rates[NSP];

	// load local array with initial values from global array
	//copy in
	for (int i = 0; i < FWD_RATES; i++)
	{
		local_fwd_rates[i] = fwd_rates[tid + i * NUM];
	}
	//copy in
	for (int i = 0; i < REV_RATES; i++)
	{
		local_rev_rates[i] = rev_rates[tid + i * NUM];
	}
	//copy in
	for (int i = 0; i < PRES_MOD_RATES; i++)
	{
		local_pres_mod[i] = pres_mod[tid + i * NUM];
	}
	#pragma unroll
	for (int repeat = 0; repeat < REPEATS; repeat++)
	{
		if (tid < NUM)
		{
			eval_spec_rates (local_fwd_rates, local_rev_rates, local_pres_mod, local_spec_rates);
		}
	}
	//copy back
	for (int i = 0; i < NSP; i++)
	{
		spec_rates[tid + i * NUM] = local_spec_rates[i];
	}
}

/////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[]) {

  int NUM = 16384;
  srand(1);

  // check for problem size given as command line option
  if (argc > 1) {
    int problemsize = NUM;
    if (sscanf(argv[1], "%i", &problemsize) != 1 || (problemsize <= 0)) {
      printf("Error: Problem size not in correct range\n");
      printf("Provide number greater than 0\n");
      exit(1);
    }
    NUM = problemsize;
  }
  
  /* Block size */
  int BLOCK_SIZE = 128;
	
	/////////////////////////////////////////////////
	// arrays

	// size of data array in bytes
	size_t size = NUM * sizeof(Real);

	//temperature array
	Real* T_host = (Real *) malloc (size);
	// pressure/volume arrays
	Real* pres_host = (Real *) malloc (size);
	// conc array
	Real* conc_host = (Real *) malloc (NUM * NSP * sizeof(Real));
  
  	Real sum = 0;
  	Real y_dummy[NN];
  	//come up with a random state
  	for (int i = 1; i < NN; i++)
  	{
  		Real r = getRand();
  		sum += r * r;
  		y_dummy[i] = r;
  	}
  	//normalize
  	sum = sqrt(sum);
  	 for (int i = 1; i < NN; i++)
  	{
  		y_dummy[i] /= sum;
  	}
  	Real T = 1200;
  	y_dummy[0] = T;
  	Real P = 101325;
  	for (int j = 0; j < NUM; j++)
  	{
  		pres_host[j] = P;
  		T_host[j] = T;
  	}

  	populate(NUM, P, y_dummy, conc_host);

  
	// set & initialize device using command line argument (if any)
	cudaDeviceProp devProp;
	if (argc <= 2) {
		// default device id is 0
		checkCudaErrors (cudaSetDevice (0) );
		checkCudaErrors (cudaGetDeviceProperties(&devProp, 0));
	} else {
		// use second argument for number

		// get number of devices
		int num_devices;
		cudaGetDeviceCount(&num_devices);

		int id = 0;
		if (sscanf(argv[2], "%i", &id) == 1 && (id >= 0) && (id < num_devices)) {
			checkCudaErrors (cudaSetDevice (id) );
		} else {
			// not in range, error
			printf("Error: GPU device number not in correct range\n");
			printf("Provide number between 0 and %i\n", num_devices - 1);
			exit(1);
		}
		checkCudaErrors (cudaGetDeviceProperties(&devProp, id));
	}
  
  	// initialize GPU

	// block and grid dimensions
	dim3 dimBlock ( BLOCK_SIZE, 1 );
	#ifdef QUEUE
	dim3 dimGrid ( numSM, 1 );
	#else
	int g_num = NUM / BLOCK_SIZE;
	if (g_num == 0)
		g_num = 1;
	dim3 dimGrid ( g_num, 1 );
	#endif
  
  	cudaError_t status = cudaSuccess;
  
	// Allocate device memory
	Real* T_device;
	cudaMalloc (&T_device, size);
	// transfer memory to GPU
	status = cudaMemcpy (T_device, T_host, size, cudaMemcpyHostToDevice);
	errorCheck (status);
  
	// device array for pressure or density
	Real* pr_device;
	cudaMalloc (&pr_device, size);
	status = cudaMemcpy (pr_device, pres_host, size, cudaMemcpyHostToDevice);
	errorCheck (status);

	Real* conc_device;
	// device array for concentrations
	cudaMalloc (&conc_device, NSP * size);
	status = cudaMemcpy (conc_device, conc_host, NSP * size, cudaMemcpyHostToDevice);
  	errorCheck (status);

  	//allocate fwd and reverse rate arrays
  	Real* fwd_rates;
  	status = cudaMalloc (&fwd_rates, FWD_RATES * NUM * sizeof(Real));
  	errorCheck (status);

  	Real* rev_rates;
  	status = cudaMalloc (&rev_rates, REV_RATES * NUM * sizeof(Real));
  	errorCheck (status);

  	//pres_mod
  	Real* pres_mod;
  	status = cudaMalloc (&pres_mod, PRES_MOD_RATES * NUM * sizeof(Real));
  	errorCheck (status);

  	//finally species rates
  	Real* spec_rates;
  	status = cudaMalloc (&spec_rates, NSP * NUM * sizeof(Real));
  	errorCheck (status);
     

	//////////////////////////////
	// start timer
	StartTimer();
	cuProfilerStart();
	//////////////////////////////
	rxnrates_driver <<<dimGrid, dimBlock>>> (NUM, T_device, conc_device, fwd_rates, rev_rates);
	cuProfilerStop();
	/////////////////////////////////
	// end timer
	double runtime = GetTimer();
	/////////////////////////////////
	printf("rxnrates: %le ms\n", runtime);

	//////////////////////////////
	// start timer
	StartTimer();
	cuProfilerStart();
	//////////////////////////////
	presmod_driver <<<dimGrid, dimBlock>>> (NUM, T_device, pr_device, conc_device, pres_mod);
	cuProfilerStop();
	/////////////////////////////////
	// end timer
	runtime = GetTimer();
	/////////////////////////////////
	printf("pres_mod: %le ms\n", runtime);

	//////////////////////////////
	// start timer
	StartTimer();
	cuProfilerStart();
	//////////////////////////////
	specrates_driver <<<dimGrid, dimBlock>>> (NUM, fwd_rates, rev_rates, pres_mod, spec_rates);
	cuProfilerStop();
	/////////////////////////////////
	// end timer
	runtime = GetTimer();
	/////////////////////////////////
	printf("spec_rates: %le ms\n", runtime);
  
  free (T_host);
  free (pres_host);
  free (conc_host);
  
  cudaFree (T_device);
  cudaFree (pr_device);
  cudaFree (conc_device);
  cudaFree (fwd_rates);
  cudaFree (rev_rates);
  cudaFree (pres_mod);
  cudaFree (spec_rates);
  
  status = cudaDeviceReset();
  errorCheck (status);
	
	return 0;
}
