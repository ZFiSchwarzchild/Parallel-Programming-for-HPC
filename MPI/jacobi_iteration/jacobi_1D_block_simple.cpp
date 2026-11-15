// jacobi_1D_block_simple.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <mpi.h>

void ReadInput(std::string file, int rows, int cols, float* data){
    // Read input data from file
    // FILE* fp = fopen(file.c_str(),"rb");

    // if(fp == NULL){
    //     std::cout << "ERROR: could not open file " << file << std::endl;
    //     MPI_Abrot(MPI_COMM_WORLD,1);
    // }

    // for(int i = 0; i < rows*cols; i++){
    //     if(!fscanf(fp,"%lf",&data[i])){
    //         std::cout << "ERROR: Not enough value in file " << file << std::endl;
    //     }
    //     fclose(fp);
    //     MPI_Abrot(MPI_COMM_WORLD,1);
    // }
    // fclose(fp);

    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            data[i*cols+j] = (i/121 + j/121) % 2;
        }
    }
}

void PrintOutput(std::string file, int rows, int cols, float* data){
    // Write data to the output file
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
    MPI_Init(&argc,&argv);

    int numP, myId; 
    MPI_Comm_size(MPI_COMM_WORLD,&numP);
    MPI_Comm_rank(MPI_COMM_WORLD,&myId);

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

    if((rows < 1) || (cols < 1)){
        if(!myId){
            std::cout << "ERROR: the number of rows and cols must be higher than 0"
                        << std::endl;
        }
       MPI_Abort(MPI_COMM_WORLD,1);
    }

    if(rows%numP){
        // Only the first process prints the output message
        if(!myId){
            std::cout << "ERROR: the number of rows must be multiple of the number of processes" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    float* data = nullptr;

    // Only the first process reads the data form the inputfile and boradcasts to the other process
    if(!myId){
        data = new float[rows*cols];
        ReadInput(inputFile, rows, cols, data);
    }

    // The computation is divided by rows
    int myRows = rows / numP;

    // Array for the chunck of data to work
    float* myData = new float[myRows*cols];
    float* buff = new  float[myRows*cols];

    MPI_Barrier(MPI_COMM_WORLD);

    // Measure the current time
    double start = MPI_Wtime();

    // Scatter the input matrix
    MPI_Scatter(data,   myRows*cols,    MPI_FLOAT,
                myData, myRows*cols,    MPI_FLOAT,
                0,      MPI_COMM_WORLD);
    memcpy(buff,myData,myRows*cols*sizeof(float));

    float error = errThreshold + 1.0;
    float myError;
    // double myError;
    // double error = errThreshold + 1.0;


    // Buffers to receive the rows
    float* prevRow = new float[cols];
    float* nextRow = new float[cols];
    
    int iter = 0;  // 打印迭代次数，并行版检查
    while(error > errThreshold){
        iter++;
        if(myId > 0){
            // Send the first row to the previous process
            MPI_Send(myData,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD);
        }

        if(myId < numP-1){
            // Receive the next row form the next process
            MPI_Recv(nextRow,cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Send the last row to the next process
            MPI_Send(&myData[(myRows-1)*cols],cols,MPI_FLOAT,myId+1,0,MPI_COMM_WORLD);
        }

        if(myId > 0){
            // Receive the previous row form the previous process
            MPI_Recv(prevRow,cols,MPI_FLOAT,myId-1,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Update the first row
        if((myId > 0 ) && (myRows > 1)){
            for(int j = 1; j < cols-1; j++){
                buff[j] = 0.25f * (myData[cols+j] + myData[j-1] + myData[j+1] + prevRow[j]);
            }
        }

        // Update the main block
        for(int i = 1; i < myRows-1; i++){
            for(int j = 1; j < cols-1; j++){
                buff[i*cols+j] = 0.25f * (myData[i*cols+j-1] + myData[i*cols+j+1] + myData[(i-1)*cols+j] + myData[(i+1)*cols+j]);
            }
        }

        // Update the last row
        if((myId < numP-1) && (myRows > 1)){
            for(int j = 1; j < cols-1; j++){
                buff[(myRows-1)*cols+j] = 0.25f * (nextRow[j] + myData[(myRows-1)*cols+j-1] + myData[(myRows-1)*cols+j+1]+myData[(myRows-2)*cols+j]);
            }
        }

        // Calculate the error of the block
        myError = 0.0;
        
        for(int i = 0; i < myRows; i++){            
            for(int j = 1; j < cols-1; j++){
                // determine the difference between the 'data' and 'buff' and add up error
                myError += (myData[i*cols+j]-buff[i*cols+j]) * (myData[i*cols+j]-buff[i*cols+j]);
                // double d = (double)myData[i*cols+j] - buff[i*cols+j];
                // myError += d * d;
            }
        }

        // memcpy(myData,buff,myRows*cols*sizeof(float));
        std::swap(myData,buff);

        // Sum the error of all processes 
        // The output is stored in the variable 'error' of all processes
        // MPI_Allreduce(&myError,&error,1,MPI_FLOAT,MPI_SUM,MPI_COMM_WORLD);
        // MPI_Allreduce(&myError, &error, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        // 优化--->>>> 每 10 轮才 Allreduce 一次
        if(iter % 10 == 0){
            MPI_Allreduce(&myError, &error, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        }
    }

    // 循环结束后再 Allreduce 一次，确保最终收敛判据准确
    MPI_Allreduce(&myError, &error, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

        if(!myId) std::cout << "Parallel iterations: " << iter << std::endl;
    // Only the first process wirtes
    // Gather the final matrix to the memory of process 0
    MPI_Gather(myData,myRows*cols,MPI_FLOAT,data,myRows*cols,MPI_FLOAT,0,MPI_COMM_WORLD);

    // Measure the current time
    double end = MPI_Wtime();

    if(!myId){
        std::cout << "Time with " << numP << " processes: " << end - start << "seconds" << std::endl;
        PrintOutput(outputFile, rows, cols, data);
        delete [] data;
    }

    delete [] prevRow;
    delete [] nextRow;
    delete [] myData;
    delete [] buff;

    MPI_Finalize();
    return 0;
}


// mpirun --use-hwthread-cpus \
//        --mca btl self,vader --mca pml ob1 \
//        -np 10 ./build/jacobi_1D_block_simple test 250 250 out_2 1e-6