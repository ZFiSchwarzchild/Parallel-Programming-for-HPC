#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <mpi.h>

void ReadInput(std::string file, int rows, int cols, float* data){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            data[i*cols + j] = (i/121 + j/121) % 2;
        }
    }
}

void PrintOutput(std::string file, int rows, int cols, float* data){
    FILE* fp = fopen(file.c_str(),"wb");
    if(fp == NULL){
        std::cout << "ERROR: could not open file " << file << std::endl;
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
    // Initialize MPI
    MPI_Init(&argc,&argv);
    
    // Get the number of processes
    int numP,myId;
    MPI_Comm_size(MPI_COMM_WORLD,&numP);
    MPI_Comm_rank(MPI_COMM_WORLD,&myId);

    if(argc < 6){
        // Only the first process prints the output message
        if(!myId){
            std::cout << "ERROR : the syntax of the progam is ./jacobi_1D_nonblock inputFile rows cols outputFile errThreshold"
                        << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    std::string inputFile = argv[1];
    int rows = atoi(argv[2]);
    int cols = atoi(argv[3]);
    std::string outputFile = argv[4];
    float errThreshold = atof(argv[5]);

    if((rows < 1)||(cols < 1)){
        if(!myId){
            std::cout << "ERROR: the number of rows and cols must be higher than 0" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    float* data=nullptr;

    // Only the first process read the data form the file and broadcasts the data
    if(!myId){
        data = new float[rows*cols];
        ReadInput(inputFile,rows,cols,data);
    }

    // The computation is divided by rows
    int blockRows = rows/numP;
    int myRows =blockRows;

    // For the case that rows is not multiple of numP
    if(myId < rows % numP){
        myRows++;
    }

    // Measure the current time
    double start = MPI_Wtime();

    // Arrays for the chunck of data to work
    float* myData = new float[myRows*cols];
    float* buff = new float[myRows*cols];

    // The process 0 must specify how many rows are sent to each process
    int* SendCounts = nullptr;
    int* displs = nullptr;
    if(!myId){
        SendCounts = new int[numP];
        displs = new int[numP];

        displs[0] = 0;
        for(int i = 0; i < numP; i++){
            if(i > 0){
                displs[i] = displs[i-1] + SendCounts[i-1];
            }
            if(i < rows % numP){
                SendCounts[i] = (blockRows+1)*cols;
            }else{
                SendCounts[i] = blockRows*cols;
            }
        }
    }

    // Scatter the input matrix
    MPI_Scatterv(data,SendCounts,displs,MPI_FLOAT,myData,myRows*cols,MPI_FLOAT,0,MPI_COMM_WORLD);
    memcpy(buff,myData,myRows*cols*sizeof(float));

    float error = errThreshold+1.0;
    float myError;

    // Buffers to receive the rows
    float* prevRow = new float[cols];
    float* nextRow = new float[cols];
    MPI_Status status;
    MPI_Request request[4];

    while(error > errThreshold){
        if(myId > 0){
            // Send the first row to the previous process
            MPI_Isend(myData,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD,&request[0]);

            // Receive the previous row from the previous process
            MPI_Irecv(prevRow,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD,&request[1]);
        }
        if(myId < numP-1){
            // Send the last row to the next process
            MPI_Isend(&myData[(myRows-1)*cols],cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD,&request[2]);

            // Receive the next row from the next process
            MPI_Irecv(nextRow,cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD,&request[3]);
        }

        // Update the main block
        for(int i = 1; i < myRows-1; i++){
            for(int j = 1; j < cols-1; j++){
                buff[i*cols+j] = 0.25f * (  myData[(i-1)*cols+j]
                                            +myData[(i+1)*cols+j]
                                            +myData[i*cols+j-1]
                                            +myData[i*cols+j+1]);
            }
        }

        // Update the first row
        if(myId > 0){
            MPI_Wait(&request[1],&status);
            if(myRows > 1){
                for(int j =1; j < cols-1; j++){
                    buff[j] = 0.25f * (     prevRow[j]
                                         + myData[cols+j]   
                                         + myData[j-1] 
                                         + myData[j+1]);
                }
            }
        }

        // Update the last row
        if(myId < numP -1){
            MPI_Wait(&request[3],&status);
            if(myRows > 1){
                for(int j =1; j < cols-1;j++){
                    buff[(myRows-1)*cols+j] = 0.25f * ( nextRow[j]
                                                        +myData[(myRows-2)*cols+j]
                                                        +myData[(myRows-1)*cols+j-1]
                                                        +myData[(myRows-1)*cols+j+1]    ); 
                }
            }
        }

        // IMPORTANT! wait until Isend finished ,then cover the memory of myData
        if(myId > 0){
            MPI_Wait(&request[0],&status);
        }

        if(myId < numP-1){
            MPI_Wait(&request[2],&status);
        }

        // Calculate the error of the block
        myError = 0.0;
        for(int i = 0; i < myRows; i++){
            for(int j = 0; j <cols-1; j++){
                myError += (myData[i*cols+j]-buff[i*cols+j]) * (myData[i*cols+j]-buff[i*cols+j]);
            }
        }
        memcpy(myData,buff,myRows*cols*sizeof(float));

        MPI_Allreduce(&myError,&error,1,MPI_FLOAT,MPI_SUM,MPI_COMM_WORLD);
    }

    // Only process 0 writes
    // So , gather the final matrix to the memory of process 0
    MPI_Gatherv(myData,myRows*cols,MPI_FLOAT,data,SendCounts,displs,MPI_FLOAT,0,MPI_COMM_WORLD);

    // Measure the current time 
    double end = MPI_Wtime();

    if(!myId){
        std::cout << "Time with " << numP << " processes: "
                    << end -start << " seconds" << std::endl;
        PrintOutput(outputFile,rows,cols,data);
        delete [] data;
        delete [] SendCounts;
        delete [] displs;
    }

    delete [] buff;
    delete [] myData;
    delete [] prevRow;
    delete [] nextRow;

    MPI_Finalize();
    return 0;
}
