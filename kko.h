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

/**
 * file header
 */
typedef struct  __attribute__ ((__packed__)) header{
    uint8_t codeType:1; //0 static 1 dynamic
    uint8_t model:3;    // 0 none ...
    uint8_t padding:4;
    uint32_t totalSize;
    uint32_t dataOffset;


}Header;

/**
 * class simulating tree in vector
 */
class TreeInfo{
public:
    ///Succesors
    int left;
    int right;
    ///predecessor
    int parent;



};
/**
 *
 * class represent one node in tree
 */
class mapItem{
    unsigned size;
public:
    void setSize(unsigned int size);

    /// number of occurence current value in  histogram
    unsigned long value;
    /// value of current value , vector is used in treebuilding
    std::vector<uint8_t > keys;
    /// info about node
    TreeInfo tree;
    ///binari code of current node
    std::vector<bool> code;
    std::vector<uint8_t > *getKey(); //return key if is leaf otherwise return null;

    mapItem();
    /// debug functions
    void print();
    void printCode();
    void printHistogram();
    /**
     *
     * @return length of binary code
     */
    unsigned int codeSize();
};
/**
 * class represented tree
 */
class BTree{
    /**
     *
     * @return vector with leafs
     */
    std::vector<std::vector<uint8_t> > getLeafs();

public:
    /// whole tree store in vector
    std::vector<mapItem> tree;
    /// index of root node
    int root;
    ///constructors
    explicit BTree(std::vector<mapItem>);
    BTree();
    /**
     *
     * @return depth of tree
     */
    unsigned int treeHeight();
    /**
     *
     * @param to specificate node key
     * @return length from root to node with value to
     */
    unsigned int pathLen(std::vector<uint8_t> to);
    /**
     * iterate over all nodes of tree and generate binary codes
     * @return
     */
    bool generateCodeBook();
    /**
     * generate binary code for current node
     * @param to
     * @return binary code
     */
    std::vector<bool> generateCode(std::vector<uint8_t> to);
    ///debug function
    void printCodebook(std::vector<std::vector<bool>>);
    /**
     * sort tree default asc if  flag = false desc
     * @param flag
     */
    void sortTreeByCodeLength(bool flag = true); //BIG side effect destruct connecting in tree !!!!
    void sortTreeByKey(); //sort by first key value used for sorting leafs
    /**
     * canonize current tree
     * @return canonized tree
     */
    std::vector<mapItem> canonize();
    /**
     * reconstruc tree from canonized form
     */
    void reconstructTree();




};
/// class represents functionality of application

class HTree{
    std::vector<mapItem> histogram;
    /// input file
    std::vector<uint8_t > buffer;
    BTree tree;
    //output file
    std::ofstream * outbin;
    //flag if use model
    bool model;



    bool generate_histogram();
    /**
     * set keys in tree before sorting
     * @return
     */
    bool set_keys();
    /**
     * merge last two nodes in tree from histogram
     * @param histogram
     * @return merged node
     */
    mapItem mergeLast2(std::vector<mapItem> &histogram);       //merge and delete used items
    /**
     * insert new node into histogram
     * @param item
     * @param histogram
     * @return
     */
    bool insertItem(mapItem item, std::vector<mapItem> &histogram);
    /**
     * set up tree info in vector - simulate tree in vector
     * @param tree
     * @return
     */
    bool connectTree(std::vector<mapItem> &tree);

    void openStream(std::string fileName);

    void writeToFile(void * data, unsigned int size);

    void closeStream();
    /**
     * code whole input file and every call return one bit value
     * @param dict hash table used to for coding
     * @return one bit value
     */
    inline int readBit(std::vector<std::vector<bool>> &dict);
    /**
     *
     * @param buffer read one bit from buffer
     * @param padding return number of nonused bits in last byte
     * @return
     */
    inline int readBit(std::vector<uint8_t> &buffer, unsigned int padding);
    /**
     * modification for adaptive reading
     * @param dict
     * @return
     */
    inline int readAdaptiveBit(std::vector<std::vector<bool>> &dict);
    /**
     * function counting time to rebuild treee
     * @param counter actual index of current value in input file
     * @param pixelValue value of proccesing value
     * @param histogramValue value of current histogram value
     * @return true if rebuild is needed
     */
    bool heuristic(unsigned counter, unsigned pixelValue,unsigned histogramValue);

public:
    HTree(std::vector<uint8_t > &buffer);
    void setModel(bool model);
    /// Interface for main
    void runStaticCompress(std::string outFile);
    void runStaticDeCompress(std::string outFile);
    void runAdaptiveCompress(std::string outFile, std::vector<uint8_t> buffer );
    void runAdaptiveDeCompress(std::string outFile);
    /**
     * build tree from histogram
     * @return
     */
    bool buildTree();

    Header generateHeader(unsigned padding, unsigned offset, unsigned size, unsigned mode);
    /**
     * generate lengths of tree leafs
     * @return
     */
    std::vector<uint8_t > generateTreeCoding();
    /**
     * code data from input
     * @param cnt padding
     * @return
     */
    std::vector<uint8_t > codeData(unsigned &cnt);
    /**
     * code data byte after byte according to dict
     * @param cnt
     * @param dict
     * @param last write last byte if not padded at 8 bits
     * @return
     */
    unsigned int codeAdaptiveData(unsigned &cnt,std::vector<std::vector<bool>> &dict, bool last);
    /**
     * preprocess data with model, use atributes
     */
    void applyModel();
    /**
     * preprocess buffer with model
     * @param buffer
     * @return changed buffer by model
     */
    std::vector<uint8_t > applyModel(std::vector<uint8_t> buffer);
    /**
     * reconstruct data after applying model in code time
     * @param buffer
     * @return reconstructed data
     */
    std::vector<uint8_t >useModel(std::vector<uint8_t> buffer);


    /**
     * decode data from buffer and return original data
     * @param buffer input
     * @param padding count non-used bits in last byte
     * @return
     */
    std::vector<uint8_t > decodeData(std::vector<uint8_t> &buffer,unsigned int padding);
    /**
     * decode data from buffer and return original data
     * @param buffer
     * @param padding
     * @param newHistogram vector for stor actual histogram use for rebuild tree
     * @return
     */
    std::vector<uint8_t > decodeAdaptiveData(std::vector<uint8_t> &buffer,unsigned int padding,std::vector<mapItem> &newHistogram);

    ///debug functions
    void printHistogram();
    void printTree();
};

/**
 *
 * @param v input free vector
 * @param value for writing into vector
 * @param size number of bits to write from value
 * @return return v
 */
std::vector<bool> & initBoolVector(std::vector<bool> &v, unsigned int value);
/**
 * chceck if part is subvector of base
 * @param base
 * @param part
 * @return
 */
bool subvector(std::vector<uint8_t > &base, std::vector<uint8_t > &part);






#endif //KKO_KKO_H
