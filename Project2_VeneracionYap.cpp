#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <cmath>

using namespace std;
using namespace std::chrono;

//matrix dimension
int DIMENSION;

struct Matrices 
{
	int id;
    double** vector1;
    double** vector2;    
    double** result;
};

double random_num() 
{
    return (rand() * 1.5 )  / ( rand() + 1.0 );
}

double** init(double is_random) 
{
    int i;
    double** matrix = (double**)malloc(DIMENSION * sizeof(double));

    for (i = 0; i < DIMENSION; i++) 
    {
        matrix[i] = (double*)malloc(DIMENSION * sizeof(double));
        int j;
        
        for (j = 0; j < DIMENSION; j++) 
        {
            if (is_random == 1) 
            {
                matrix[i][j] = random_num();
            }
            else {
                matrix[i][j] = 0;
            }
        }
    }
    
    return matrix;
}

void printMatrix(double** matrix) 
{
    printf("Matrix values:");
    int i;

    for (i = 0; i < DIMENSION; i++) 
    {
        printf("\n");
        int j;

        for (j = 0; j < DIMENSION; j++) 
        {
            printf("%f, ", matrix[i][j]);
        }
    }

    printf("\n\n");
}

void multiplySingle(double** a, double** b, double** result) 
{
    int i,j,k;

    for (i = 0; i < DIMENSION; i++) 
    {
        double sum = 0.0;

        for (j = 0; j < DIMENSION; j++) 
        {

            for (k = 0; k < DIMENSION; k++) 
            {
                sum += a[i][k] * b[k][j];
            }

            result[i][j] = sum;
            sum = 0;
        }
    }
}

void* runRows(void* args) 
{
	struct Matrices* thread_data = (struct Matrices*) args;
	int id = (*thread_data).id;
	int i;
	double sum = 0.0;

	for (i = 0; i < DIMENSION; i++) {
		int j;

		for (j = 0; j < DIMENSION; j++) {
			sum += (*thread_data).vector1[id][j] * (*thread_data).vector2[j][i];
		}

		(*thread_data).result[id][i] = sum;
		sum = 0.0;
	}

    pthread_exit(NULL);

}


void* runElements(void* args) 
{
	struct Matrices* thread_data = (struct Matrices*) args;
	int id = (*thread_data).id;
	int i;
	double sum = 0.0;

	for (i = 0; i < DIMENSION; i++) {
		int j;

		for (j = 0; j < DIMENSION; j++) {
			sum += (*thread_data).vector1[id][j] * (*thread_data).vector2[j][i];
		}

		(*thread_data).result[id][i] = sum;
		sum = 0.0;
	}

    pthread_exit(NULL);

}

void multiplyMult(double** a, double** b, double** result) 
{
    pthread_t* threads;
    threads = (pthread_t*)malloc(DIMENSION * sizeof(pthread_t));
    struct Matrices thread_data[DIMENSION];
    int i;

    for (i = 0; i < DIMENSION; i++) {
		thread_data[i].id = i;
    	thread_data[i].vector1 = a;
    	thread_data[i].vector2 = b;
    	thread_data[i].result = result;

    	pthread_create(&threads[i], NULL, runRows, &thread_data[i]);
    }

    for (i = 0; i < DIMENSION; i++) {
    	pthread_join(threads[i], NULL);
    }
}

void multiplyElement(double** a, double** b, double** result) 
{
    pthread_t* elements;
    elements = (pthread_t*)malloc(DIMENSION*DIMENSION * sizeof(pthread_t));
    struct Matrices thread_data[DIMENSION*DIMENSION];
    int i, j, k;

    for (i = 0; i < DIMENSION; i++) 
    {
        for (j = 0; j < DIMENSION; j++)
        {
            
                thread_data[j].id = i;
                thread_data[j].vector1 = a;
                thread_data[j].vector2 = b;
                thread_data[j].result = result;
                pthread_create(&elements[j], NULL, runElements, &thread_data[j]);
            
        }
    }

    for (i = 0; i < DIMENSION; i++) 
    {
        for (j = 0; j < DIMENSION; j++)
        {
    	    pthread_join(elements[j], NULL);
        }
    }
}

int multTimer(double** a, double** b, double** result, double nSecs)
{
    // multTimer() will count how many times
    // multiplySingle(), mutltiplyMult(), multiplyElement() will execute in nSecs seconds.
   
    int nPasses=0;
    auto tStart = steady_clock::now();
    while (duration_cast<seconds>(steady_clock::now() - tStart).count() < nSecs)
    {
        multiplySingle(a,b,result);
        multiplyMult(a,b,result);
        multiplyElement(a,b,result);
        nPasses++;
    }
    return nPasses;
}

int main() 
{
    srand(time(0));

    unsigned int n = thread::hardware_concurrency();
    cout << n << " concurrent threads are supported." << endl << endl;

    cout << "Enter dimension of matrix: ";
    cin >> DIMENSION;
    cout << endl;
    cout << "Printing " << DIMENSION << "x" << DIMENSION << " matrices" << endl << endl;

    const int nTrials = 10;
    const double nSecs = 5.0;

    double** matrixA = init(1);
    double** matrixB = init(1);
    double** resultMatrix = init(0);

    printMatrix(matrixA);
    printMatrix(matrixB);

    multiplySingle(matrixA, matrixB, resultMatrix);
    printf("Single threaded multiplication\n\n");
    printMatrix(resultMatrix);

    resultMatrix = init(0);
    multiplyMult(matrixA, matrixB, resultMatrix);
    printf("\nMulti threaded (by row) multiplication\n\n");
    printMatrix(resultMatrix);

    resultMatrix = init(0);
    multiplyElement(matrixA, matrixB, resultMatrix);
    printf("\nMulti threaded (by element) multiplication\n\n");
    printMatrix(resultMatrix);

    int nSingleMults;
    int nMultipleMults;
    int nElementMults;

    vector <int> arraySingleMults;
    vector <int> arrayMultipleMults;
    vector <int> arrayElementMults;

    int singleMultSum = 0;
    int multipleMultSum = 0;
    int elementMultSum = 0;

    int singleMultAve;
    int multipleMultAve;
    int elementMultAve;

    int singleMultVar;
    int multipleMultVar;
    int elementMultVar;

    cout << "\tnSingleMults " << "\tnMultipleMults " << "\tnElementMults " <<"\n";

    for (int i = 0; i < nTrials; i++)
    {
        nSingleMults = multTimer(matrixA, matrixB, resultMatrix, nSecs);
        nMultipleMults = multTimer(matrixA, matrixB, resultMatrix, nSecs);
        nElementMults = multTimer(matrixA, matrixB, resultMatrix, nSecs);

        cout << "\t \t" <<  nSingleMults << "\t \t" << nMultipleMults << "\t \t" << nElementMults <<"\n";
        singleMultSum += nSingleMults;
        multipleMultSum += nMultipleMults;
        elementMultSum += nElementMults;

        arraySingleMults.push_back(nSingleMults);
        arrayMultipleMults.push_back(nMultipleMults);
        arrayElementMults.push_back(nElementMults);

        singleMultAve = singleMultSum*1.0/nTrials;
        multipleMultAve = multipleMultSum*1.0/nTrials; 
        elementMultAve = elementMultSum*1.0/nTrials;

        singleMultVar += ((arraySingleMults[i] - singleMultAve) * (arraySingleMults[i] - singleMultAve))/nTrials;
        multipleMultVar += ((arrayMultipleMults[i] - multipleMultAve) * (arrayMultipleMults[i] - multipleMultAve))/nTrials;
        elementMultVar += ((arrayElementMults[i] - elementMultAve) * (arrayElementMults[i] - elementMultAve))/nTrials;
    }

    auto minmax1 = minmax_element(arraySingleMults.begin(), arraySingleMults.end());
    auto minmax2 = minmax_element(arrayMultipleMults.begin(), arrayMultipleMults.end());
    auto minmax3 = minmax_element(arrayElementMults.begin(), arrayElementMults.end());

    cout << "Minimum: \n";
    cout << "\t " << *minmax1.first
        << "\t " << *minmax2.first
        << "\t " << *minmax3.first <<"\n";
    
    cout << "Maximum: \n";
    cout << "\t " << *minmax1.second
        << "\t " << *minmax2.second
        << "\t " << *minmax3.second <<"\n";

    cout << "Average: \n";
    cout << "\t " << singleMultAve
        << "\t " << multipleMultAve
        << "\t " << elementMultAve <<"\n";
    
    cout << "Variance: \n";
    cout << "\t " << singleMultVar
        << "\t " << multipleMultVar
        << "\t " << elementMultVar <<"\n";
    
    return 0;
}