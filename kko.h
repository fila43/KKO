//
// Created by filip on 24.4.19.
//

#ifndef KKO_KKO_H
#define KKO_KKO_H

#include <fstream>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <getopt.h>


typedef struct  __attribute__ ((__packed__)) header{
    uint8_t codeType:1; //0 static 1 dynamic
    uint8_t model:3;    // 0 none ...
    uint8_t padding:4;
    uint32_t totalSize;
    uint32_t dataOffset;


}Header;

class TreeInfo{
public:
    //Succesors
    int left;
    int right;
    int parent;



};
class mapItem{
    unsigned size;
public:
    void setSize(unsigned int size);


    unsigned long value;
    std::vector<uint8_t > keys;
    TreeInfo tree;
    std::vector<bool> code;
    std::vector<uint8_t > *getKey(); //return key if is leaf otherwise return null;

    mapItem();
    void print();
    void printCode();
    void printHistogram();
    unsigned int codeSize();
};
class BTree{
    std::vector<std::vector<uint8_t> > getLeafs();

public:
    std::vector<mapItem> tree;
    int root;

    explicit BTree(std::vector<mapItem>);
    BTree();
    unsigned int treeHeight();
    unsigned int pathLen(std::vector<uint8_t> to);
    bool generateCodeBook();
    std::vector<bool> generateCode(std::vector<uint8_t> to);
    void printCodebook(std::vector<std::vector<bool>>);
    void sortTreeByCodeLength(bool flag = true); //BIG side effect destruct connecting in tree !!!!
    void sortTreeByKey(); //sort by first key value used for sorting leafs
    std::vector<mapItem> canonize();
    void reconstructTree();




};


class HTree{
    std::vector<mapItem> histogram;
    std::vector<uint8_t > buffer;
    BTree tree;
    std::ofstream * outbin;



    bool generate_histogram();
    bool set_keys();
    mapItem mergeLast2(std::vector<mapItem> &histogram);       //merge and delete used items
    bool insertItem(mapItem item, std::vector<mapItem> &histogram);
    bool connectTree(std::vector<mapItem> &tree);
    void openStream(std::string fileName);
    void writeToFile(void * data, unsigned int size);
    void closeStream();
    inline int readBit(std::vector<std::vector<bool>> &dict); //inline???
    inline int readBit(std::vector<uint8_t> &buffer, unsigned int padding);
    inline int readAdaptiveBit(std::vector<std::vector<bool>> &dict);

public:
    HTree(std::vector<uint8_t > &buffer);
    void runStaticCompress(std::string outFile);
    void runStaticDeCompress(std::string outFile);
    void runAdaptiveCompress(std::string outFile, std::vector<uint8_t> buffer );
    void runAdaptiveDeCompress(std::string outFile);
    bool buildTree();
    Header generateHeader(unsigned padding, unsigned offset, unsigned size, unsigned mode);
    std::vector<uint8_t > generateTreeCoding();
    std::vector<uint8_t > codeData(unsigned &cnt);
    unsigned int codeAdaptiveData(unsigned &cnt,std::vector<std::vector<bool>> &dict, bool last);


    std::vector<uint8_t > decodeData(std::vector<uint8_t> &buffer,unsigned int padding);
    std::vector<uint8_t > decodeAdaptiveData(std::vector<uint8_t> &buffer,unsigned int padding,std::vector<mapItem> &newHistogram);

    void printHistogram();
    void printTree();
};




void writeToFile(std::string fileName,void * data, unsigned int size);
std::vector<bool> & initBoolVector(std::vector<bool> &v, unsigned int value);
bool subvector(std::vector<uint8_t > &base, std::vector<uint8_t > &part);






#endif //KKO_KKO_H
