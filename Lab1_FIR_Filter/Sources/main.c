#include <stdio.h>
//#include "fput_debug.c"
#include "arm_math.h"

typedef struct {
	float b[5];
}FIR_coeff;


FIR_coeff coeff = {
	.b[0] = 0.1,
	.b[1] = 0.15,
	.b[2] = 0.5,
	.b[3] = 0.15,
	.b[4] = 0.1
};

const float32_t firCoeffs32[5] = {
	0.1,0.15,0.5,0.15,0.1
};

static float32_t firStateF32[5];
extern int __FIR_A(float* InputArray, float* OutputArray, FIR_coeff* coeff, int Length);

void FIR_C(float* InputArray, float* OutputArray, FIR_coeff* coeff, int Length, int Order) {
	int bot = 0;
	int top = 0;
	
	while(top < Length) {
		for(int i = top; i >= bot; i--) {
			*(OutputArray+top) += (*(InputArray+i)) * coeff->b[top-i];
		}
		top++;
		if(top >= Order) bot++;
	}
}

void subtraction(float *input, float *output, float* result, int length) {
	for(int i = 0; i < length; i++) {
		*(result+i) = *(input+i) - *(output+i);
	}
}

void printArray(float *input, int length) {
	for(int i = 0; i < length; i++) 
		printf("%f ", *(input+i));
	printf("\n");
}

float average(float *input, int length) {
	float avg = 0;
	for(int i = 0; i < length; i++)
		avg += *(input+i);
	return avg/length;
}

float stdDev(float *input, int length) {
	float avg = average(input, length);
	float stdDev = 0;
	for(int i = 0; i < length; i++) 
		stdDev += (*(input+i)-avg)*(*(input+i)-avg);
	return sqrt(stdDev/(length-1));
}

float var(float *input, float *track, int length, float avgInp, float avgTrk) {
	float sum = 0;
	for(int i = 0; i < length; i++) {
		sum += (*(input+i)-avgInp)*(*(track+i)-avgTrk);
	}
	return sum;
}

float corr(float *input, float *track, int length) {
	float avgInp = average(input, length);
	float avgTrk = average(track, length);
	float varMix = var(input, track, length, avgInp, avgTrk);
	float varInp = var(input, input, length, avgInp, avgInp);
	float varTrk = var(track, track, length, avgTrk, avgTrk);
	return varMix/(sqrt(varInp*varTrk));
}

void corrArrayfn(float *input, float *track, float *corrArray, int length) {
	for(int i = 0; i < 2*length-1; i++) {
		for(int j = 0; j < length; j++) {
			int diff = j-i;
			//while(diff < 0) diff += length;
			(*(corrArray+i)) += (*(input+j)) * (*(track+diff));
		}
	}
}

int main()
{
	int order = 5;
	int length = 20;
	float input[20]	= {1.0,1.07,1.15,1.2,1.25,1.3,1.358,1.39,1.15,1.2,1.15,1.1,1.05,1.0,0.8,0.6,0.4,0.0,-0.3,-0.8};
	//{0.1,1.1,2.1,4.1,5.1,6.1,7.1,9.1,10.1,11.1,-3, -3. -3.1, 2, 3, 4, 5};
	//{1,1.07,1.15,1.2,1.25,1.3,1.358,1.39,1.15,1.2,1.15};
	
	float c_output[length];
	float s_output[length];
	float subResult[length];
	float corrArray[2*length-1];
	float* input_ptr = &input[0];
	float* c_output_ptr = &c_output[0];
	float* s_output_ptr = &s_output[0];
	
	float CMSIS_out[length];
	float CMSIS_sub[length];
	float CMSIS_std;
	float CMSIS_avgDiff;
	float CMSIS_cor[2*length-1];
	arm_fir_instance_f32 S;
	
	//C filter array
	FIR_C(input_ptr, c_output_ptr, &coeff, length, order);
	printf("C filter array\n");
	printArray(c_output_ptr, length);
	
	//Assembly filter array
	__FIR_A(input_ptr, s_output_ptr, &coeff, length);
	printf("Assembly filter array\n");
	printArray(s_output_ptr, length);

	//Subtraction array
	subtraction(input_ptr, s_output_ptr, &subResult[0], length);
	printf("Subtraction array\n");
	printArray(&subResult[0], length);
	
	//StdDev, average difference, correlation
	printf("StdDev, average difference, correlation\n");
	printf("%f\n", stdDev(&subResult[0], length));
	printf("%f\n", average(&subResult[0], length));
	printf("%f\n", corr(&subResult[0], input_ptr, length));
	corrArrayfn(&subResult[0], &input[0], &corrArray[0], length);
	printArray(&corrArray[0], 2*length-1);
	
	//CMSIS FIR filter array
	arm_fir_init_f32(&S, order,(float32_t *)&firCoeffs32[0], &firStateF32[0], 1);
	for(int i = 0; i < length; i++) {
		arm_fir_f32(&S, (float32_t *)(&input[0]+i), (float32_t *)(&CMSIS_out[0]+i), 1);
	}
	printf("CMSIS Fir filter array\n");
	printArray(&CMSIS_out[0], length);
	
	//CMSIS subtraction array
	for(int i = 0; i < length; i++) {
		arm_sub_f32((float32_t *) (&input[0]+i), (float32_t *)(&CMSIS_out[0]+i), (float32_t *)(&CMSIS_sub[0]+i),1);
	}
	printf("CMSIS Subtraction array\n");
	printArray(&CMSIS_sub[0], length);
	
	//CMSIS stdDev
	arm_std_f32((float32_t *)(&CMSIS_sub[0]), length, (float32_t *)(&CMSIS_std));
	printf("CMSIS stdDev\n");
	printf("%f\n", CMSIS_std);
	
	//CMSIS average Difference
	arm_mean_f32((float32_t *)(&CMSIS_sub[0]), length, (float32_t *)(&CMSIS_avgDiff));
	printf("CMSIS average difference\n");
	printf("%f\n", CMSIS_avgDiff);
	
	//CMSIS correlation
	arm_correlate_f32((float32_t *)(&input[0]), (uint32_t)length, (float32_t *)(&CMSIS_sub[0]), (uint32_t)length, (float32_t *)&CMSIS_cor[0]);
	//arm_correlate_fast_q15((q15_t *)(input_ptr), (uint32_t)12, (q15_t *)(&CMSIS_sub[0]), (uint32_t)12, (q15_t *)&CMSIS_cor[0]);

	printf("CMSIS  correlation\n");
	printArray(&CMSIS_cor[0], 2*length-1);
	
		return 0;
}