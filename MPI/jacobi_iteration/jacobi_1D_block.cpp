// jacobi_1D_block.cpp
#include <stdlib.h>
#include <stdio.h>
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
    // write data the output file
    FILE* fp = fopen(file.c_str(),"wb");
    if(fp == NULL){
        std::cout << "ERROR: could not open file " << file  << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for(int i = 0; i < rows; i++){
        for(int j=0; j < cols; j++){
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

    // Guard of input arguments
    if(argc != 6){
        if(!myId){
            std::cout << "ERROR: the syntax of the program " << argv[0] << " is: " << std::endl
                        <<" inputFile rows cols outputFile errThreshold " << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    std::string inputFile = argv[1];
    int rows = atoi(argv[2]);
    int cols = atoi(argv[3]);
    std::string outputFile = argv[4];
    float errThreshold = atof(argv[5]);

    if((rows < 1) || (cols < 1)){
        if(!myId){
            std::cout << "ERROR: the number of rows and cols must be positive integers" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    float* data = nullptr;
    if(!myId)
    {
        data = new float[rows*cols];               
        ReadInput(inputFile, rows, cols, data);
    }
    // not need anymore since we are using MPI_Scatterv and MPI_Gatherv
    // if(!(rows % numP)){
    //     if(!myId){
    //         std::cout << "ERROR: the number of rows must be multiple of the number of processes" << std::endl;
    //     }
    //     MPI_Abort(MPI_COMM_WORLD, 1);
    // }

    int blockRows = rows/numP;
    int myRows = blockRows;

    if(myId < rows % numP){
        myRows++;           // 前rows % numP 个进程多分1行
    }

    double start = MPI_Wtime();
     
    float* myData = new float[myRows*cols];
    float* buff = new float[myRows*cols];

    int* sendCounts = nullptr;
    int* displs = nullptr;

    if(!myId){
        sendCounts = new int[numP];
        displs = new int[numP];

        displs[0] = 0;
        for(int i = 0; i < numP; i++){
            if(i > 0){
                displs[i] = displs[i-1] + sendCounts[i-1];
            }
            if(i < rows % numP){
                sendCounts[i] = (blockRows+1)*cols;
            }else{
                sendCounts[i] = blockRows*cols;
            }
        }
    } 

    // Scatter the input matrix to all processes
    MPI_Scatterv(data,sendCounts,displs,MPI_FLOAT,myData,myRows*cols,MPI_FLOAT,0,MPI_COMM_WORLD);
    memcpy(buff, myData, myRows*cols*sizeof(float));
    
    // Buffers to receive the rows from the previous and next processes
    float* prevRow = new float[cols];
    float* nextRow = new float[cols];

    float error = errThreshold + 1.0;
    float myError;
    
    while(error > errThreshold){
        // Send the first row to the previous process
        if(myId > 0){
            MPI_Send(myData, cols, MPI_FLOAT, myId-1, 0, MPI_COMM_WORLD);
        }

        if(myId < numP-1){
            // Receive the next row from the next process
            MPI_Recv(nextRow,cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            // Send the last row to the next process
            MPI_Send(&myData[(myRows-1)*cols],cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD);
        }

        if(myId > 0){
            // Receive the previous row from the previous process
            MPI_Recv(prevRow,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }

        // Update the first row of myData
        if((myId > 0) && (myRows > 1)){
            for(int j = 1; j < cols-1;j++){
                buff[j] = 0.25f * (myData[cols+j] + prevRow[j] + myData[j-1] + myData[j+1]);
            
            }
        }

        // Update the main block
        for(int i =1; i < myRows-1; i++){
            for(int j =1; j < cols-1; j++){
                buff[i*cols+j] = 0.25f * (myData[(i-1)*cols+j] + myData[(i+1)*cols+j] + myData[i*cols+j-1] + myData[i*cols+j+1]);
            }
        }

        // Update the last row of myData
        if((myId < numP-1) && (myRows > 1)){ 
            for(int j = 1; j < cols-1; j++){
                buff[(myRows-1)*cols+j] = 0.25f * (myData[(myRows-2)*cols+j]+nextRow[j]+myData[(myRows-1)*cols+j-1]+myData[(myRows-1)*cols+j+1]);
            }
        }

        // Calculate the local error of the block
        myError = 0.0;
        for(int i = 0; i < myRows; i++){
            for(int j = 1; j< cols-1;j++){
                myError += (myData[i*cols+j]-buff[i*cols+j]) * (myData[i*cols+j]-buff[i*cols+j]);
            }
        }

        // memcpy(myData,buff,myRows*cols*sizeof(float));
        std::swap(myData,buff);

        // Calculate the global error
        MPI_Allreduce(&myError,&error,1,MPI_FLOAT,MPI_SUM,MPI_COMM_WORLD);
    }

    // Gather the updated blocks from all processes to the root process
    MPI_Gatherv(myData,myRows*cols,MPI_FLOAT,data,sendCounts,displs,MPI_FLOAT,0,MPI_COMM_WORLD);

    double end = MPI_Wtime();

    if(!myId){
        std::cout << "Time taken for Jacobi iteration: " << end-start << " seconds" << " with "<< numP << " processes " << std::endl;
        PrintOutput(outputFile, rows, cols, data);
        delete[] data;
        delete[] sendCounts;
        delete[] displs;
    }

    delete[] myData;
    delete[] buff;
    delete[] prevRow;
    delete[] nextRow;

   

    MPI_Finalize();
    return 0;
}