#include "common.h"

void initFile(int numElems, char* fileName) {
    // I will create a zero-filled file of (2+numElems)*sizeof(int) bytes
    // where the last 8 bytes are used for storing read & write indexes
    FILE* fp = fopen(fileName, "wb");
    if (fp == NULL) handle_error("initFile fopen");

    int i, ret, value=0;
    for (i=0; i<numElems+2; ++i) {
        ret = fwrite(&value, sizeof(int), 1, fp);
        if (ret != 1) handle_error("initFile fwrite");
    }    

    ret = fclose(fp);
    if (ret == EOF) handle_error("initFile fclose");
}

void writeToBufferFile(int value, int numElems, char* fileName) {
    int ret, write_index;
    FILE* fp = fopen(fileName, "r+b");
    if (fp == NULL) handle_error("writeToBufferFile fopen");

    // write_index is after (numElems+1)*sizeof(int) bytes
    int windex_offset = (numElems+1)*sizeof(int);
    ret = fseek(fp, windex_offset, SEEK_SET);
    if (ret) handle_error("writeToBufferFile fseek index");
    ret = fread(&write_index, sizeof(int), 1, fp);
    if (ret != 1) handle_error("writeToBufferFile fread index");

    // write element to file
    int element_offset = write_index*sizeof(int);
    ret = fseek(fp, element_offset, SEEK_SET);
    if (ret) handle_error("writeToBufferFile fseek element") ;
    ret = fwrite(&value, sizeof(int), 1, fp);
    if (ret != 1) handle_error("writeToBufferFile fwrite element");

    // update write_index
    write_index = (write_index + 1) % numElems;
    ret = fseek(fp, windex_offset, SEEK_SET);
    if (ret) handle_error("writeToBufferFile fseek index");
    ret = fwrite(&write_index, sizeof(int), 1, fp);
    if (ret != 1) handle_error("writeToBufferFile fwrite index"); 

    ret = fclose(fp);
    if (ret == EOF) handle_error("writeToBufferFile fclose");
}

int readFromBufferFile(int numElems, char* fileName) {
    int ret, read_index, value;
    FILE* fp = fopen(fileName, "r+b");
    if (fp == NULL) handle_error("readFromBufferFile fopen");

    // read_index is after numElems*sizeof(int) bytes
    int rindex_offset = numElems*sizeof(int);
    ret = fseek(fp, rindex_offset, SEEK_SET);
    if (ret) handle_error("readFromBufferFile fseek index");
    ret = fread(&read_index, sizeof(int), 1, fp);
    if (ret != 1) handle_error("readFromBufferFile fread index");

    // read element from file
    int element_offset = read_index*sizeof(int);
    ret = fseek(fp, element_offset, SEEK_SET);
    if (ret) handle_error("readFromBufferFile fseek element");
    ret = fread(&value, sizeof(int), 1, fp);
    if (ret != 1) handle_error("readFromBufferFile fread element");

    // update read_index
    read_index = (read_index + 1) % numElems;
    ret = fseek(fp, rindex_offset, SEEK_SET);
    if (ret) handle_error("readFromBufferFile fseek index");
    ret = fwrite(&read_index, sizeof(int), 1, fp);
    if (ret != 1) handle_error("readFromBufferFile fwrite index");

    ret = fclose(fp);
    if (ret == EOF) handle_error("readFromBufferFile fclose");

    return value;
}