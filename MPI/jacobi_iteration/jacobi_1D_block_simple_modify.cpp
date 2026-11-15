// jacobi_1D_block_simple_modify.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <mpi.h>

void ReadInput(std::string file, int rows, int cols, float* data){
    for(int i = 0; i < rows; i++){
        for(int j=0; j < cols; j++){
            data[i*cols+j] = (i/121+j/121) % 2;
        }
    }
}

void PrintOutput(std::string file, int rows, int cols, float* data){
    // Write data to the output file
    FILE* fp = fopen(file.c_str(),"wb");

    if(fp == NULL){
        std::cout << "ERROR: could not open file " << file  << std::endl;
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            fprintf(fp,"%f ",data[i*cols+j]);            
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
}

int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);

    int numP, myId;
    MPI_Comm_size(MPI_COMM_WORLD, &numP);
    MPI_Comm_rank(MPI_COMM_WORLD, &myId);
    if(argc < 6){
        if(!myId){
            std::cout << "ERROR: the syntax of the program " << argv[0]
                        << " inputFile rows cols outputFile errThreshold "
                        << std::endl;  
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }  

    std::string inputFile = argv[1];
    int rows = atoi(argv[2]);
    int cols = atoi(argv[3]);
    std::string outputFile = argv[4];
    float errThreshold = atof(argv[5]);

    if(rows < 1 || cols < 1 ){
        if(!myId){
            std::cout << "ERROR: the number of rows and cols must be higher than 0 "
                        << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if(rows % numP){
        if(!myId){
            std::cout << "ERROR: the number of rows must be multiple of the number of processes" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    float* data;
    if(!myId){
        data = new float[rows*cols];
        ReadInput(inputFile,rows,cols,data);
    }

    int myRows = rows / numP;
    float* myData = new float[myRows*cols];
    float* buff = new float[myRows*cols];

    MPI_Barrier(MPI_COMM_WORLD);

    double start = MPI_Wtime();

    MPI_Scatter(data,   myRows*cols,    MPI_FLOAT,
                myData, myRows*cols,    MPI_FLOAT,
                0,      MPI_COMM_WORLD);
    
    memcpy(buff,myData,myRows*cols*sizeof(float));

    float error = errThreshold + 1.0;
    float myError;
    int iter = 0;

    float* prevRow = new float[cols];
    float* nextRow = new float[cols];

    while(error > errThreshold){
        iter++;

        if(myId > 0){
            MPI_Send(myData,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD);
        }

        if(myId < numP-1){
            MPI_Send(&myData[(myRows-1)*cols],cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD);
            MPI_Recv(nextRow,cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        if(myId > 0){
            MPI_Recv(prevRow,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }

        // Update the first row
        if((myId > 0) && (myRows > 1)){
            for(int j = 1; j < cols-1; j++){
                buff[j] = 0.25f * (myData[cols+j]+prevRow[j]+myData[j-1]+myData[j+1]);
            }
        }

        // Update the main block
        for(int i = 1; i < myRows-1; i++){
            for(int j = 1; j < cols-1; j++){
                buff[i*cols+j] = 0.25f * (myData[i*cols+j-1]+myData[i*cols+j+1]+myData[(i-1)*cols+j]+myData[(i+1)*cols+j]);
            }
        }

        // Update the last row
        if(myId < numP-1 && myRows >1){
            for(int j = 1; j < cols-1; j++){
                buff[(myRows-1)*cols+j] = 0.25f * (myData[(myRows-2)*cols+j]+nextRow[j]+myData[(myRows-1)*cols+j-1]+myData[(myRows-1)*cols+j+1]);
            }
        }

        myError = 0.0;
        for(int i = 0; i < myRows; i++){
            for(int j = 0; j < cols; j++){
                myError += (myData[i*cols+j]-buff[i*cols+j])*(myData[i*cols+j]-buff[i*cols+j]);
            }
        }

        MPI_Allreduce(&myError,&error,1,MPI_FLOAT,MPI_SUM,MPI_COMM_WORLD);

        std::swap(myData,buff);
    }

    if(!myId){
        std::cout << "Number of iterations: " << iter << std::endl;
    }

    // Gathe the matrix to the memory of process 0
    MPI_Gather(myData,  myRows*cols,    MPI_FLOAT,
               data,    myRows*cols,    MPI_FLOAT,
                0,      MPI_COMM_WORLD);

    double end = MPI_Wtime();

    if(!myId){
        std::cout << "Time with  "<< numP << " processes: " << end-start << " seconds" << std::endl;
        PrintOutput(outputFile,rows,cols,data);
        delete[] data;
    }

    delete[] myData;
    delete[] buff;
    delete[] prevRow;
    delete[] nextRow;  

    MPI_Finalize();
    return 0;
}