#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h> 

#define BATCHSIZE 4096

void printMSG (int readCount,int writeCount,int byteCount,char * filename, bool binary){
    fprintf(stderr, "The file %s transferred %d bytes and read %d times and wrote %d times\n",strcmp(filename,"-") == 0 ? "standard input":filename,byteCount,readCount,writeCount); 
    if(binary)
        fprintf(stderr,"WARNING THE FILE %s CONTAINS BINARY DATA\n",filename); 
}

void openError(char* fileName){
    fprintf(stderr, "Error opening file %s \n",fileName); 
    exit(-1); 
}

void writeError(char* fileName){
    fprintf(stderr,"Error writting to file %s\n", fileName);
    exit(-2); 
}

void partialWrite(int* writeCount, int* outputFileD, char* buf, int readLen,int writeLen, char* inputFileName){
    char newBuf[readLen-writeLen]; 
    for(int i = writeLen; i<readLen; i++)
        newBuf[i-writeLen] = buf[i]; 
    int wn = write(*outputFileD,newBuf,readLen-writeLen); 
    *writeCount++; 
    if(wn < 0 || wn != readLen-writeLen) // idk if I am supposed to add this or of if the second one should just never happen
        writeError(inputFileName); 
}

bool checkForBinary(char* buf, char* fileName, int size){
    for(int i = 0; i<size; i++){
        if(!(isprint(buf[i])||isspace(buf[i]))){
            return true; 
            break; 
        }
    }
    return false; 
}

void concatenate(int* outputFileD, char* inputFileName){
    int fdI = strcmp(inputFileName,"-") != 0 ? open(inputFileName,O_RDONLY,0666) : STDIN_FILENO; 
    if(fdI < 0) openError(inputFileName); 
    char buf[BATCHSIZE]; 
    int byteCount,writeCount = 0;  
    int n = read(fdI,buf,BATCHSIZE);
    int readCount = 1; 
    bool binary = false; 
    while(n > 0) {
        binary = binary ? binary : checkForBinary(buf,inputFileName,n);
        int wn = write(*outputFileD,buf,n); 
        if(wn != n)
            partialWrite(&writeCount, outputFileD,buf,n,wn,inputFileName); 
        byteCount += n; 
        writeCount++; 
        if(n < BATCHSIZE) break;
        n = read(fdI,buf,BATCHSIZE);
        readCount++; 
    }
    printMSG(readCount,writeCount,byteCount,inputFileName,binary); 
}

int main(int argc, char *argv[]){
    int c; 
    bool oFlag = false; 
    bool once = false;
    int outputFile = STDOUT_FILENO;
    while ((c = getopt (argc, argv, "o::")) != -1){
        if(c == 'o') oFlag = true;
    }
    for(int i = optind; i<argc; i++){
        if(oFlag && i == optind){
            outputFile = open(argv[i],O_WRONLY|O_CREAT|O_TRUNC,0666); 
            continue; 
        }
        once = true;
        concatenate(&outputFile,argv[i]); 
    }
    if(!once)
        concatenate(&outputFile,"-");
    exit(0); 
}