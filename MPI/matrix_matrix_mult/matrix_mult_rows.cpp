    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <iostream>
    #include <mpi.h>

    void ReadInput(std::string file, int rows, int cols, float* data){
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                data[i*cols+j] = (i + j) % 2;
            }
        }
    }

    void PrintOutput(std::string file, int rows,int cols,float* data){
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

        int numP,myId;
        MPI_Comm_size(MPI_COMM_WORLD,&numP);
        MPI_Comm_rank(MPI_COMM_WORLD,&myId);

        if(argc < 7){
            if(!myId){
                std::cout << "ERROR: the syntax of the program is : ./matrix_mult_rows matA matB matC m k n"
                            << std::endl;
            }
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        std::string inputMatA = argv[1];
        std::string inputMatB = argv[2];
        std::string outputMatC = argv[3];
        int m = atoi(argv[4]);
        int k = atoi(argv[5]);
        int n = atoi(argv[6]);

        if((m < 1) || (k < 1) || (n < 1)){
            if(!myId){
                std::cout << "ERROR: the nomber of 'm' 'k' and 'n' must be greater than 0 " 
                            << std::endl;
            }
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        float* matA = nullptr;
        float* matB = new float[k*n];
        float* matC = nullptr;

        if(!myId){
            matA = new float[m*k];
            ReadInput(inputMatA,m,k,matA);
            ReadInput(inputMatB,k,n,matB);
            matC = new float[m*n];
        }

        int blockRows = m / numP;
        int myRows = blockRows;
        if(myId < m % numP){
            myRows++;
        }

        // Measure the current time
        double start = MPI_Wtime();

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
                if(i < m % numP){
                    sendCounts[i] = (blockRows+1)*k;
                }else{
                    sendCounts[i] = blockRows*k;
                }
            }
        }

        float* myA = new float[myRows*k];
        float* myC = new float[myRows*n];

        MPI_Scatterv(matA,sendCounts,displs,MPI_FLOAT,myA,myRows*k,MPI_FLOAT,0,MPI_COMM_WORLD);
        MPI_Bcast(matB,k*n,MPI_FLOAT,0,MPI_COMM_WORLD);

        // 三重循环
        for(int i = 0; i < myRows; i++){
            for(int j = 0; j < n; j++){
                myC[i*n+j] = 0.0;
                for(int l = 0; l < k; l++){
                    myC[i*n+j] += myA[i*k+l] * matB[l*n+j];    // 乘法顺序待优化，以提升缓存命中
                }
            }
        }

        // 重新计算收信计数和偏移向量
        if(!myId){
            for(int i = 0; i < numP; i++){
                if(i > 0){
                    displs[i] = displs[i-1] + sendCounts[i-1];
                }
                if(i < m % numP){
                    sendCounts[i] = (blockRows+1) * n; 
                }else{
                    sendCounts[i] = blockRows * n;
                }
            }
        }

        // 收信
        MPI_Gatherv(myC,myRows*n,MPI_FLOAT,matC,sendCounts,displs,MPI_FLOAT,0,MPI_COMM_WORLD);

        // Measure current time
        double end = MPI_Wtime();

        if(!myId){
            std::cout << "Time with " << numP << " processes: " << end - start << " scends" << std::endl;
            PrintOutput(outputMatC,m,n,matC);
            delete [] matA;
            delete [] matC;
            delete [] sendCounts;
            delete [] displs;
        }

        delete [] matB;
        delete [] myA;
        delete [] myC;

        MPI_Finalize();
        return 0;
    }