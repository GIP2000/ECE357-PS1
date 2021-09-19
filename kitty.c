#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h> 

#define BATCHSIZE 4096


//prints the standard message after finishing reading a file
void printMSG (int readCount,int writeCount,int byteCount,char * filename, bool binary){
    fprintf(stderr, "The file %s transferred %d bytes and read %d times and wrote %d times\n",strcmp(filename,"-") == 0 ? "standard input":filename,byteCount,readCount,writeCount); 
    if(binary)
        fprintf(stderr,"WARNING THE FILE %s CONTAINS BINARY DATA\n",filename); 
}

//prints a message for an error opening a file 
void openError(char* fileName){
    fprintf(stderr, "Error opening file %s \n",fileName); 
    exit(-1); 
}

//prints an error for writting to a file 
void writeError(char* fileName){
    fprintf(stderr,"Error writing to file %s\n", fileName);
    exit(-2); 
}

//handles the parital write case 
void partialWrite(int* writeCount, int* outputFileD, char* buf, int readLen,int writeLen, char* inputFileName){
    char newBuf[readLen-writeLen]; 
    for(int i = writeLen; i<readLen; i++)
        newBuf[i-writeLen] = buf[i]; 
    int wn = write(*outputFileD,newBuf,readLen-writeLen); 
    *writeCount++; 
    if(wn < 0 || wn != readLen-writeLen) // idk if I am supposed to add this or of if the second one should just never happen
        writeError(inputFileName); 
}

//returns true if the file contains binary characters 
bool checkForBinary(char* buf, char* fileName, int size){
    for(int i = 0; i<size; i++){
        if(!(isprint(buf[i])||isspace(buf[i]))){
            return true; 
            break; 
        }
    }
    return false; 
}

//writes the contents of a input file to an output file 
void concatenate(int* outputFileD, char* inputFileName){
    int fdI = strcmp(inputFileName,"-") != 0 ? open(inputFileName,O_RDONLY,0666) : STDIN_FILENO; // opens the input file 
    if(fdI < 0) openError(inputFileName); 
    char buf[BATCHSIZE]; 
    int byteCount = 0,writeCount = 0,n = read(fdI,buf,BATCHSIZE),readCount = 1; 
    bool binary = false; 
    while(n > 0) { // while there is data to read
        binary = binary ? binary : checkForBinary(buf,inputFileName,n);
        int wn = write(*outputFileD,buf,n); 
        if(wn != n)  // if correct amount of bytes were not written 
            partialWrite(&writeCount, outputFileD,buf,n,wn,inputFileName); 
        byteCount += n; 
        writeCount++; 
        if(n < BATCHSIZE) break;  // if we didn't read the full amount (meaning we reached EOF)
        n = read(fdI,buf,BATCHSIZE);
        readCount++; 
    }
    if(strcmp(inputFileName,"-") != 0) // if the file isn't standard input close the file 
        close(fdI);
    printMSG(readCount,writeCount,byteCount,inputFileName,binary); 
}

int main(int argc, char *argv[]){
    int c; 
    bool oFlag = false;
    int outputFile = STDOUT_FILENO;
    char* outputFileName = NULL; 
    while ((c = getopt (argc, argv, "o:")) != -1){  // check for the o option 
        if(c == 'o') {
            outputFileName = optarg;
            outputFile = open(optarg,O_WRONLY|O_CREAT|O_TRUNC,0666);  // opens the output file if it exists 
            oFlag = true; 
        } else abort(); // if other flags are present abort 
    }

    for(int i = optind; i<argc; i++)
        concatenate(&outputFile,argv[i]); 
        
    if(optind == argc) // if there were no other arguments then default to standard input
        concatenate(&outputFile,"-");
    exit(0); 
}