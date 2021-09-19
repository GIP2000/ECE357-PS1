#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h> 

#define BATCHSIZE 4096
enum ErrorType{WRITE,READ,OPEN,CLOSE};
char* names[] = {"writing", "reading","opening","closing"}; 

//prints the standard message after finishing reading a file
void printMSG (int readCount,int writeCount,int byteCount,char * filename, bool binary){
    fprintf(stderr, "The file %s transferred %d bytes and read %d times and wrote %d times\n",strcmp(filename,"-") == 0 ? "standard input":filename,byteCount,readCount,writeCount); 
    if(binary)
        fprintf(stderr,"WARNING THE FILE %s CONTAINS BINARY DATA\n",strcmp(filename,"-") == 0 ? "standard input":filename); 
}

// Prints generic error message 
void error(char * filename, enum ErrorType type ){
    fprintf(stderr,"Error %s file %s\n",names[type],strcmp(filename,"-") == 0 ? "standard input":filename);
    exit(-1); 
}

// handles opening files 
int myOpen(char* filename,int oflag){
    int fd = open(filename,oflag,0666); 
    if(fd < 0) error(filename,OPEN); 
    return fd;
}

// handles writing files 
int myWrite(int* fd, char* buf, int size,char* filename, int* writeCount){
    int n = write(*fd,buf,size);
    (*writeCount)++; 
    if(n<0) error(filename,WRITE); 
    return n; 
}
// handles reading files 
int myRead(int* fd,char* buf,int size,char* filename, int* readCount){
    int n = read(*fd,buf,size);
    (*readCount)++; 
    if(n<0) error(filename,READ); 
    return n; 
}
// hanles closing files 
void myClose(int* fd,char* filename){
    if(close(*fd) < 0) error(filename,CLOSE); 
}

//handles the parital write case 
void partialWrite(int* writeCount, int* outputFileD, char* buf, int readLen,int writeLen, char* inputFileName){
    char newBuf[readLen-writeLen]; 
    int i;
    for(i = writeLen; i<readLen; i++)
        newBuf[i-writeLen] = buf[i]; 
    int wn = myWrite(outputFileD,newBuf,readLen-writeLen,inputFileName,writeCount); 
    if(wn != readLen-writeLen) error(inputFileName,WRITE); 
}

//returns true if the file contains binary characters 
bool checkForBinary(char* buf, char* fileName, int size){
    int i;
    for(i = 0; i<size; i++){
        if(!(isprint(buf[i])||isspace(buf[i]))) return true; 
    }
    return false; 
}

//writes the contents of a input file to an output file 
void concatenate(int* outputFileD, char* inputFileName){
    int fdI = strcmp(inputFileName,"-") != 0 ? myOpen(inputFileName,O_RDONLY) : STDIN_FILENO; // opens the input file 
    char buf[BATCHSIZE]; 
    int readCount = 0,byteCount = 0,writeCount = 0;
    int n = myRead(&fdI,buf,BATCHSIZE,inputFileName,&readCount);
    bool binary = false; 
    while(n > 0) { // while there is data to read
        binary = binary ? binary : checkForBinary(buf,inputFileName,n);
        int wn = myWrite(outputFileD,buf,n,inputFileName,&writeCount); 
        if(wn != n)  // if correct amount of bytes were not written 
            partialWrite(&writeCount, outputFileD,buf,n,wn,inputFileName); 
        byteCount += n; 
        if(n < BATCHSIZE) break;  // if we didn't read the full amount (meaning we reached EOF)
        n = myRead(&fdI,buf,BATCHSIZE,inputFileName,&readCount);
    }
    if(strcmp(inputFileName,"-") != 0) // if the file isn't standard input close the file 
        myClose(&fdI,inputFileName); 
    
    printMSG(readCount,writeCount,byteCount,inputFileName,binary); 
}

int main(int argc, char *argv[]){
    int c,i; 
    int outputFile = STDOUT_FILENO;
    while ((c = getopt (argc, argv, "o:")) != -1){  // check for the o option 
        if(c == 'o') 
            outputFile = myOpen(optarg,O_WRONLY|O_CREAT|O_TRUNC);  // opens the output file if it exists 
        else abort(); // if other flags are present abort 
    }
    for(i = optind; i<argc; i++)
        concatenate(&outputFile,argv[i]); 

    if(optind == argc) // if there were no other arguments then default to standard input
        concatenate(&outputFile,"-");
    exit(0); 
}