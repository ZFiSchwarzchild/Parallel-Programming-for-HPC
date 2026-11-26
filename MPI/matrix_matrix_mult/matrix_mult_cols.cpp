#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <mpi.h>

void ReadInput(std::string file, int rows, int cols, float* data){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            data[i*cols+j] = (i/121 + j/121) % 2;
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
    MPI_Init(&argc,&argv);

    int numP, myId;
    MPI_Comm_size(MPI_COMM_WORLD,&numP);
    MPI_Comm_rank(MPI_COMM_WORLD,&myId);

    if(argc < 7){
        if(!myId){
            std::cout << "ERROR: the syntax of the program " << argv[0] << " is: " << std::endl
                        << "inputFile rows cols outputFile errThreshold " << std::endl;
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
            std::cout << "ERROR: the number of rows and cols must be higher than 0" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    


    MPI_Finalize();
    return 0;
}