// jacobi_seq.cpp
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <chrono>

void ReadInput(std::string file, int rows, int cols, float* data){
    // Read data form input file
    FILE* fp = fopen(file.c_str(),"rb");

    // // guard for file input
    // if(fp == NULL){
    //     std::cout << "ERROR: could not open file " << file << std::endl;
    //     exit(1);
    // }

    // if((rows < 1) || (cols < 1)){
    //     std::cout << "ERROR: the number of rows and cols must be higher than 0"
    //                 << std::endl;
    //     exit(1);
    // }
    // for(int i = 0; i < rows*cols; i++){
    //     if(!fscanf(fp,"%lf",&data[i])){
    //         std::cout << "ERROR: not enough values in file " << file << std::endl;
    //         exit(1);
    //     }
    // }

    // fclose(fp);
    
    // data for test
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            data[i*cols+j] = (i/121 + j/121) % 2;
        }        
    }
}

void PrintOutput(std::string file, int rows, int cols, float* data){
    FILE* fp = fopen(file.c_str(),"wb");
    
    // guard 
    if(fp == NULL){
        std::cout << "ERROR: could not open file " << file << std::endl;
        exit(1);
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
    // Guard of input arguments
    if(argc < 6){
        std::cout << "ERROR: the syntax of the program is " << argv[0]
                        << " inputFile rows cols outputFile errTreshold"
                        << std::endl;
        exit(1);
    }

    std::string inputFile = argv[1];
    int rows = atoi(argv[2]);
    int cols = atoi(argv[3]);
    std::string outputFile = argv[4];
    float errThreshold = atof(argv[5]);

    // Guard of rows and cols 
    if((rows < 1) || (cols < 1)){
        std::cout << "ERROR: the number of rows and cols must be higher than 0 "
                    << std::endl;
        exit(1);
    }

    float* data = new float[rows*cols];
    float* buff = new float[rows*cols];

    ReadInput(inputFile,rows,cols,data);
    memcpy(buff,data,rows*cols*sizeof(float));

    // Measure the current time
    std::chrono::time_point<std::chrono::system_clock> start,end;
    start = std::chrono::system_clock::now();

    float error = errThreshold + 1.0;
    // double error = errThreshold + 1.0;
    int iter = 0;
    while(error > errThreshold){
        iter++;
        for(int i = 1; i < rows - 1; i++){
            for(int j = 1; j < cols - 1; j++){
                buff[i*cols+j] = 0.25f * (data[(i-1)*cols+j] + data[(i+1)*cols+j] + data[i*cols+j-1] + data[i*cols+j+1]);
            }
        }

        error = 0.0;
        for(int i = 1; i < rows - 1; i++){
            for(int j = 1; j < cols -1; j++){
                error += (data[i*cols+j] - buff[i*cols+j]) * (data[i*cols+j] - buff[i*cols+j]);
                // double d = (double)data[i*cols+j] - buff[i*cols+j];
                // error += d * d;
            }
        }

        // Swap the data and buff
        memcpy(data,buff,rows*cols*sizeof(float));
    }
    std::cout << "Sequential iterations: " << iter << std::endl;
    
    end = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = end - start;

    std::cout << "Sequential Jacobi with dimentions " << rows << " x " << cols << " in " << elapsed_seconds.count()
                << " seconds" << std::endl;

    PrintOutput(outputFile,rows,cols,data);

    delete [] data;
    delete [] buff;

    return 0;
}