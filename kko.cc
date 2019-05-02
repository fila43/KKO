//
// Created by filip on 24.4.19.
//

#include "kko.h"

/************************************************************************
 * Debug print functions                                                *
 ************************************************************************/

void mapItem::print() {
    {
        std::cout<<"[";
        for (auto item1 : keys ){
            std::cout<<(int)item1<<",";
        }
        std::cout<<"] ";
      //  std::cout<<value<<std::endl;
        std::cout<<"left: "<<tree.left<<" | right: "<<tree.right<<" | parent: "<<tree.parent<<std::endl<<std::endl;

    }
}
void HTree::printHistogram() {
    for (unsigned int i = 0; i<histogram.size();i++){
        std::cout<<i<<" : "<<histogram[i].value<<std::endl;
    }
}
void HTree::printTree() {
    for (auto item : tree.tree){
        std::cout<<(int)item.keys[0]<<" : "<<item.codeSize()<<std::endl;
    }
}
void mapItem::printCode() {
    if (keys.size() != 1)
        return;
    std::cout<<(int)keys[0]<<"\t";
    std::cout<<codeSize()<<"\t";

    for (auto item: code){
        std::cout<<item;
    }
    std::cout<<std::endl;
}


mapItem::mapItem() {
    keys.clear();
    tree.left = -1;
    tree.right = -1;
    tree.parent = -1;
    value = 0;
    size = 0;
}
unsigned int mapItem::codeSize() {
    if (code.size() == 0)
        return size;
    else
        return code.size();
}
HTree::HTree(std::vector<uint8_t> &buffer) {
    this->histogram = std::vector<mapItem>();
    this->buffer = buffer;
    this->tree = BTree ();
    model = false;
}

void HTree::writeToFile(void *data, unsigned int size) {
    this->outbin->write((char*)data,size);
}

void HTree::closeStream() {
    this->outbin->close();
}

std::vector<uint8_t > * mapItem::getKey() {
    if (keys.size() == 1)
        return &keys;
    else
        return nullptr;
}

void mapItem::setSize(unsigned int size) {
    this->size = size;
}

bool cmp(mapItem a,mapItem b){
    if (a.value > b.value)
        return true;
    else if (a.value == b.value)
        return a.keys[0] < b.keys[0];
    else
        return false;
}

/******************************************************************************************************************************
 *                                                                                                                            *
 *                                             STATIC CODING                                                                  *
 *                                                                                                                            *
 ******************************************************************************************************************************/
void HTree::runStaticCompress(std::string outFile) {

    generate_histogram();

    set_keys();

    std::sort(histogram.begin(),histogram.end(),cmp);

    buildTree();

    this->tree.generateCodeBook();

    this->tree.sortTreeByCodeLength();

    this->tree.tree = this->tree.canonize();

    this->tree.sortTreeByKey();


    unsigned padding;

    openStream(outFile);

    std::vector<uint8_t > treeCoding = generateTreeCoding(); //coding tree

    std::vector<uint8_t > data = codeData(padding);         //coded data

    Header  tmp = generateHeader(padding,treeCoding.size(),treeCoding.size()+data.size()+ sizeof(struct header),0);

    writeToFile(&tmp, sizeof(struct header));
    writeToFile(treeCoding.data(), sizeof(uint8_t)*treeCoding.size());
    writeToFile(data.data(), sizeof(uint8_t)*data.size());
    closeStream();
   // std::cout<<" "<<tmp.totalSize/(double)buffer.size()<<std::endl;


}

/******************************************************************************************************************************
 *                                                                                                                            *
 *                                             STATIC DECODING                                                                *
 *                                                                                                                            *
 ******************************************************************************************************************************/

void HTree::runStaticDeCompress(std::string outFile) {


    auto head = (Header*)buffer.data();

    unsigned int padd = (unsigned int)head->padding;
    uint8_t *tmpcodeLen = buffer.data()+ sizeof(struct header);
    histogram.clear();
    tree.tree.clear();

    /*
     * read tree (lengths) from file
     */
    for (int i =0; i<256;i++) {
        mapItem tmp;
        tmp.setSize((int) (*tmpcodeLen));
        if ((int) (*tmpcodeLen) == 0) {
            tmpcodeLen += sizeof(uint8_t);
            continue;
        }
        tmp.keys.emplace_back(i);
        tree.tree.emplace_back(tmp);
        tmpcodeLen += sizeof(uint8_t);
    }


    /*
     * reconstruct tree for decoding
     */
    this->tree.sortTreeByCodeLength();

    std::vector<mapItem> canonizedTree = tree.canonize();

    this->tree.tree = canonizedTree;

    this->tree.reconstructTree();

    buffer.erase(buffer.begin(),buffer.begin()+265);    //remove header from loaded file

    std::vector<uint8_t > decodedData = decodeData(buffer,padd);

    if (model)                                          // apply model
        decodedData = useModel(decodedData);


    openStream(outFile);
    writeToFile(decodedData.data(), sizeof(uint8_t)*decodedData.size());
    closeStream();

}
/******************************************************************************************************************************
 *                                                                                                                            *
 *                                             ADAPTIVE CODING                                                                *
 *                                                                                                                            *
 ******************************************************************************************************************************/
void HTree::runAdaptiveCompress(std::string outFile, std::vector<uint8_t> buffer) {
  unsigned padding =0;
  unsigned rebuild =0;
  unsigned long x =0;
  unsigned int size = 0;
  std::vector<mapItem> newHistogram;

    histogram.resize(256);

    for (unsigned int i =0;i<histogram.size();i++) {
        if (i ==0){
            histogram[i].value =20;
            continue;
        }
        histogram[i].value = 1;
    }

  //  for(auto &item:histogram)
   //     item.value= 1;

    //build default tree
    newHistogram = histogram;
    set_keys();
    buildTree();
    this->tree.generateCodeBook();
    openStream(outFile);

    //write empty header
    Header head = generateHeader(0,0,0,1);
    writeToFile(&head, sizeof(Header));

 //create hash table for coding
    std::vector<std::vector<bool>> dict;
    dict.resize(256);
    for (auto item: tree.tree) {
        if (item.keys.size() == 1)
            dict[item.keys[0]] = item.code;
    }

    // code bytes from input
    for (auto item: buffer){
        x++;
        newHistogram[item].value++;
        this->buffer.clear();
        this->buffer = std::vector<uint8_t >();
        this->buffer.emplace_back(item);

        if (x-1 == buffer.size()-1) {
            size = codeAdaptiveData(padding, dict, true);      //write last byte
        }
        else
           size = codeAdaptiveData(padding,dict,false);       //code byte and write


        if ( heuristic(x,item,newHistogram[item].value) ){ //check if tree rebuild is required

            //rebuild
            rebuild++;
            histogram = newHistogram;
            set_keys();
            buildTree();
            this->tree.generateCodeBook();
            dict.clear();
            dict.resize(256);
            for (auto item: tree.tree) {
                if (item.keys.size() == 1)
                    dict[item.keys[0]] = item.code;
            }

        }

    }
    //write header
    this->outbin->seekp(0);
    head = generateHeader(padding,0,x,1);
    writeToFile(&head, sizeof(Header));


    closeStream();

  //  std::cout<<" "<<size/(double)buffer.size()<<std::endl;


}

/******************************************************************************************************************************
 *                                                                                                                            *
 *                                             ADAPTIVE DECODING                                                              *
 *                                                                                                                            *
 ******************************************************************************************************************************/
void HTree::runAdaptiveDeCompress(std::string outFile) {

    //read header
    Header head;
    Header * tmpHead;
    tmpHead =(Header*) buffer.data();
    head = (*tmpHead);

    //remove header from buffer
    buffer.erase(buffer.begin(),buffer.begin()+ sizeof(Header));

    std::vector<mapItem> newHistogram;

    //prepare tree
    histogram.clear();
    histogram.resize(256);

   for (unsigned int i =0;i<histogram.size();i++) {
       if (i ==0){
           histogram[i].value =20;
           continue;
       }
       histogram[i].value = 1;
   }
    //for(auto &item:histogram)
      //  item.value= 1;
    newHistogram = histogram;
    set_keys();
    buildTree();
    for (unsigned int i = 0; i< tree.tree.size(); i++)
        if (tree.tree[i].tree.parent == -1)
            tree.root = i;
    this->tree.generateCodeBook();



    std::vector<uint8_t > decodedData = decodeAdaptiveData(buffer,head.padding,newHistogram);

  if (model)
        decodedData = useModel(decodedData);

    openStream(outFile);
    writeToFile(decodedData.data(), sizeof(uint8_t)*decodedData.size());
    closeStream();



}
/**
 * read buffer by bits
 * @param buffer input
 * @param padding returns last n non-used bits in file
 * @return value of bit or -1 if EOF occurs
 */
int HTree::readBit(std::vector<uint8_t> &buffer, unsigned int padding) {
    static unsigned long i=0;
    static unsigned int j=0;
    unsigned int mask =0;
    for (;i<buffer.size();){

        mask = (unsigned int)1<<(7-(j%8));
        if (i == buffer.size() -1 && padding == j%8) {
            return -1;
        }
            if ((buffer[i]&mask)!=0){
                j++;
                if (j%8 == 0)
                    i++;
                return 1;
            }else{
                j++;
                if (j%8 == 0)
                    i++;
                return 0;
            }

    }

    return -1;
}
/**
 * read input buffer by bits and find right values in tree
 * @param buffer
 * @param padding
 * @return vector with original file
 */
std::vector<uint8_t > HTree::decodeData(std::vector<uint8_t> &buffer, unsigned int padding) {
    std::vector<uint8_t >result;
    int x;
    unsigned long p =0;

    unsigned int node = tree.root;
    while (true){
        x = readBit(buffer,padding);
        if (x == -1) {
            break;
        }
            p++;
            if (x == 1) {
                node = tree.tree[node].tree.right;
            }
            else {
                node = tree.tree[node].tree.left;
            }

       if(tree.tree[node].tree.left == -1 && tree.tree[node].tree.right == -1) {
           result.emplace_back(tree.tree[node].keys[0]);
           node = tree.root;
       }

    }
    if(tree.tree[node].tree.left == -1 && tree.tree[node].tree.right == -1)
        result.emplace_back(tree.tree[node].keys[0]);
    else if(node == 0 || node == tree.root){
        ;
    }else{

    }

    return result;

}
/**
 * apply difference model to input data
 */
void HTree::applyModel() {
    std::vector<uint8_t > newBuffer;
    newBuffer.emplace_back(buffer[0]);
    for(unsigned int i=1 ; i<buffer.size(); i++){
        newBuffer.emplace_back(buffer[i-1]-buffer[i]);
    }
    buffer =newBuffer;
}
/**
 *
 * @param buffer
 * @return buffer after application model
 */
std::vector<uint8_t > HTree::applyModel(std::vector<uint8_t> buffer) {
    std::vector<uint8_t > newBuffer;
    newBuffer.emplace_back(buffer[0]);
    for(unsigned int i=1 ; i<buffer.size(); i++){
        newBuffer.emplace_back(buffer[i-1]-buffer[i]);
    }
   return newBuffer;
}
/**
 * decode model data
 * @param buffer
 * @return
 */
std::vector<uint8_t >HTree::useModel(std::vector<uint8_t> buffer) {
    std::vector<uint8_t > newBuffer;
    newBuffer.emplace_back(buffer[0]);
    for (unsigned int i =1; i<buffer.size();i++){
        newBuffer.emplace_back(newBuffer[i-1]-buffer[i]);
    }
    return newBuffer;
}
void HTree::setModel(bool model) {
    this->model = model;
}
/**
 * decode data and rebuild tree
 * @param buffer
 * @param padding
 * @param newHistogram
 * @return
 */
std::vector<uint8_t > HTree::decodeAdaptiveData(std::vector<uint8_t> &buffer, unsigned int padding,std::vector<mapItem> &newHistogram) {
    std::vector<uint8_t >result;
    int x;
    unsigned long p =0;
    unsigned int  counter=0;
    unsigned int rebuild =0;

    unsigned int node = tree.root;
    while (true){
        x = readBit(buffer,padding);
        if (x == -1) {
            break;
        }
        p++;
        if (x == 1) {
            node = tree.tree[node].tree.right;
        }
        else {
            node = tree.tree[node].tree.left;
        }


        if(tree.tree[node].tree.left == -1 && tree.tree[node].tree.right == -1) {
            counter++;
            newHistogram[(int)tree.tree[node].keys[0]].value++;

            if ( heuristic(counter,(int)tree.tree[node].keys[0],newHistogram[(int)tree.tree[node].keys[0]].value)) {
                //rebuild
                rebuild++;
                histogram = newHistogram;
                set_keys();
                tree = BTree();
                buildTree();
                for (unsigned int i = 0; i < tree.tree.size(); i++)
                    if (tree.tree[i].tree.parent == -1)
                        tree.root = i;
                this->tree.generateCodeBook();
            }
            //add byte to result
            result.emplace_back(tree.tree[node].keys[0]);
            node = tree.root;
        }

    }
    if(tree.tree[node].tree.left == -1 && tree.tree[node].tree.right == -1)
        result.emplace_back(tree.tree[node].keys[0]);
    else if(node == 0){
        ;
    }else{

    }


    return result;
}
/**
 * heuristic function for decision when rebuild tree in adaptive mode
 * @param counter number of processed pixels
 * @param pixelValue  value of actual byte (pixel)
 * @param histogramValue actual updated value from histogram
 * @return truu for rebuild
 */
bool HTree::heuristic(unsigned counter, unsigned pixelValue, unsigned histogramValue) {
    static int i = 0;

    i++;
    if (i<10){
        return true;
    }
    if (i<100){
        if (i%5 == 0)
            return true;
    }

    if(counter%850 == 0)
        return true;
    if (!(histogramValue & (histogramValue - 1))){
        if ((histogramValue & 0b1111111111111111)){
            return true;
        }
    }

    return false;

}
std::vector<uint8_t > HTree::codeData(unsigned &cnt) {
    std::vector<std::vector<bool>> dict;
    dict.resize(256);
    for (auto item: tree.tree){
        if (item.keys.size() == 1)
            dict[item.keys[0]] = item.code;
    }

    std::vector<uint8_t > result;


    int x;
    unsigned long i = 0;
    uint8_t tmp_value = 0;
    unsigned int offset = 0;
    uint8_t write_value=0;
     while (1) {
         x = readBit(dict);
         if (x == -1)
             break;

         offset = 7-(i%8);
         tmp_value = (uint8_t)x;
         tmp_value = tmp_value<<offset;
         write_value= write_value|tmp_value;
         i++;
         if (i%8 == 0){
             result.emplace_back(write_value);
             write_value=0;
         }

     }
    result.emplace_back(write_value);
     cnt = i%8;
    return result;
}
/**
 *
 * @param cnt   padding
 * @param dict  hash table for faster acces
 * @param last falg if write value after eof file , used to use for last byte in adaptive mode
 * @return
 */
unsigned int HTree::codeAdaptiveData(unsigned &cnt,std::vector<std::vector<bool>> &dict,bool last) {

    int x;
    static unsigned int size = 0;
    static unsigned long i = 0;
    static uint8_t tmp_value = 0;
    static unsigned int offset = 0;
    static  uint8_t write_value=0;
    while (1) {
        x = readAdaptiveBit(dict);
        if (x == -1)
            break;

        offset = 7-(i%8);
        tmp_value = (uint8_t)x;
        tmp_value = tmp_value<<offset;
        write_value= write_value|tmp_value;
        i++;
        if (i%8 == 0){
            size++;
            writeToFile(&write_value, sizeof(uint8_t));
            write_value=0;
        }

    }

    if (last){
        writeToFile(&write_value, sizeof(uint8_t));
        write_value=0;

    }

    cnt = i%8;
    return size;
}
/**
 * get coded bit
 * @param dict
 * @return
 */
int HTree::readBit(std::vector<std::vector<bool>> &dict) {
    static unsigned i =0;
    static unsigned j =0;
    for (;i<buffer.size();) {
        std::vector<bool> tmp = dict[buffer[i]];
        for (; j < tmp.size(); ) {

            if (tmp[j]){
                j++;
                return 1;
            } else{
                j++;
                return 0;
            }
        }
        i++;
        j=0;
    }

    return -1;
}
/**
 * read coded bit from only one byte
 * @param dict
 * @return
 */
inline int HTree::readAdaptiveBit(std::vector<std::vector<bool>> &dict) {
    static unsigned j =0;
        std::vector<bool> tmp = dict[buffer[0]];
        for (; j < tmp.size(); ) {

            if (tmp[j]){
                j++;
                return 1;
            } else{
                j++;
                return 0;
            }
        }

        j=0;


    return -1;
}
Header HTree::generateHeader(unsigned padding, unsigned offset,unsigned size, unsigned mode) {
    Header newHeader;
    newHeader.codeType = mode;
    newHeader.model = 0;
    newHeader.padding = padding;
    newHeader.totalSize = size;
    newHeader.dataOffset = offset;

    return newHeader;

}
/**
 * generate tree for decoding canonized tree
 * @return
 */
std::vector<uint8_t > HTree::generateTreeCoding() {
    std::vector<uint8_t > tmp;
    tmp.resize(256);
    unsigned int empty = 0;

    if(tree.tree.size() == 1){
        for(unsigned int i =0; i<256;i++){
            if (tree.tree[0].keys[0] == i){
                tmp[i] = tree.tree[0].codeSize();
                continue;
            }
            tmp[i] = 0;

        }
        return tmp;
    }

    for (unsigned int i =0; i<tree.tree.size();i++){
        tmp[tree.tree[i].keys[0]] = tree.tree[i].codeSize();
    }
/*
    for (unsigned int i = 0; i< 256; i++){
        if(i<tree.tree[i-empty].keys[0]) {
            tmp[i] = 0;
            empty++;

        }
        else {
            tmp[i] = tree.tree[i-empty].codeSize();
        }
    }
    */
    return tmp;
}
/**
 * create histogram from buffer
 * @return
 */
bool HTree::generate_histogram() {
    histogram.resize(256);

    for (unsigned int i = 0; i<buffer.size();i++){
        histogram[buffer[i]].value++;
    }

    return true;
}
/**
 * set keys to histogram before sort
 * @return
 */
bool HTree::set_keys() {
    for (unsigned int i=0; i<histogram.size();i++) {
        if (histogram[i].keys.empty())
            histogram[i].keys.emplace_back(i); //????
    }
    auto tmp = histogram.begin();

    while (tmp != histogram.end()){
        if ((*tmp).value == 0) {
            tmp = histogram.erase(tmp);
            continue;
        }
        tmp++;
    }

    return true;
}
bool HTree::buildTree() {
    std::vector<mapItem> tree;
    tree = histogram;


    while (histogram.size()>1){
        mapItem tmpItem = mergeLast2(histogram);
        tree.emplace_back(tmpItem);
        insertItem(tmpItem,histogram);
    }

    connectTree(tree);

    return true;

}
/**
 * merge last two items in histogram use to for creating tree
 * @param histogram
 * @return
 */
mapItem HTree::mergeLast2(std::vector<mapItem> &histogram) {
    mapItem newItem;
    newItem.keys.insert((newItem.keys.begin()),(histogram.end()-1)->keys.begin(),(histogram.end()-1)->keys.end());
    newItem.keys.insert((newItem.keys.begin()+1),(histogram.end()-2)->keys.begin(),(histogram.end()-2)->keys.end());
    newItem.value = (histogram.end()-1)->value + (histogram.end()-2)->value;

    histogram.erase(histogram.end());
    histogram.erase(histogram.end());

    return newItem;

}
/**
 * insert item at start of histogram
 * @param item
 * @param histogram
 * @return
 */
bool HTree::insertItem(mapItem item, std::vector<mapItem> & histogram) {
    int tmp = 0;

    for (unsigned int i = 0 ; i< histogram.size();i++){
        if (histogram[i].value <=item.value) {
            tmp = i;
            break;
        }
    }

    histogram.insert(histogram.begin()+tmp,item);

    return true;
}
/**
 * connect tree in vector
 * @param tree
 * @return
 */
bool HTree::connectTree(std::vector<mapItem> &tree) {


    unsigned int tmp = 0;       //child

    while(tmp != tree.size()-1){
        unsigned int item = tmp +1; //parent
        while (!subvector(tree[item].keys,tree[tmp].keys) && item != tree.size()) {
            item++;
            if  (item >= tree.size()){
                std::cerr<<"chyba indexu!!\n";
                std::cerr<<"item: "<<item<<" SIZE: "<<tree.size()<<std::endl;

              //  for (auto x : tree[tmp].keys)
                //    std::cout<<(int)x<<",";
                //std::cout<<std::endl;

                exit(-1);
            }
        }

        if (tree[item].tree.left == -1){
            tree[item].tree.left = tmp;
        }else if(tree[item].tree.right == -1){
            tree[item].tree.right = tmp;
        }else{
            if (item == tree.size())
                break;
            std::cerr<<"chyba stromu!!!\n";
            exit(-1);
        }

        if (tree[tmp].tree.parent == -1) {
            tree[tmp].tree.parent = item;
        }else{
            std::cerr<<"chyba!!\n";
            exit(-1);
        }
        tmp++;
    }
    this->tree = BTree(tree);
    return true;
}

void HTree::openStream(std::string fileName) {
    outbin = new std::ofstream( fileName, std::ios::binary );
}


BTree::BTree(std::vector<mapItem> tree) {
    this->tree = tree;
    for (unsigned int i = 0; i< tree.size(); i++)
        if (tree[i].tree.parent == -1)
            root = i;
}

BTree::BTree() {
    root = 0;
    tree = std::vector<mapItem>();
}
/**
 * return height of tree
 * @return
 */
unsigned int BTree::treeHeight() {
    unsigned int height = 0;
    for (auto leaf : this->getLeafs()){
        unsigned int tmp_height = this->pathLen(leaf);
        if (tmp_height >height)
            height =  tmp_height;
    }
    return height;


}
/**
 * create tree from canonized value
 */
void BTree::reconstructTree() {
    sortTreeByCodeLength(false);
    root = 0;

    std::vector<mapItem> newTree;
    newTree.emplace_back(tree[0]);

    unsigned int node = 0;
    for (unsigned int i = 0; i < tree.size(); i++) {
        node = 0;
        for (unsigned int j = 0; j < tree[i].codeSize(); j++) {
            if (tree[i].code[j]) {
                if (newTree[node].tree.right != -1) {
                    newTree[node].keys.emplace_back(tree[i].keys[0]);
                    node = newTree[node].tree.right;

                } else {
                    mapItem tmp;
                    newTree.emplace_back(tmp);
                    newTree[node].tree.right = newTree.size() - 1;
                    newTree[node].keys.emplace_back(tree[i].keys[0]);
                    node = newTree[node].tree.right;

                }
            } else {
                if (newTree[node].tree.left != -1) {
                    newTree[node].keys.emplace_back(tree[i].keys[0]);
                    node = newTree[node].tree.left;

                } else {
                    mapItem tmp;
                    newTree.emplace_back(tmp);
                    newTree[node].tree.left = newTree.size() - 1;
                    newTree[node].keys.emplace_back(tree[i].keys[0]);
                    node = newTree[node].tree.left;


                }
            }
            if (j == tree[i].codeSize()-1){
                newTree[node].keys.emplace_back(tree[i].keys[0]);
            }

        }
    }
    tree = newTree;

}
/**
 * return all leafs from tree
 * @return
 */
std::vector<std::vector<uint8_t> > BTree::getLeafs() {
    std::vector<std::vector<uint8_t >> tmp;
    for(auto item : tree){
        if (item.keys.size() == 1)
            tmp.emplace_back(item.keys);
    }
    return tmp;
}
/**
 * return length of way from root to current leaf
 * @param to
 * @return
 */
unsigned int BTree::pathLen(std::vector<uint8_t> to) {
    auto node = root;
    unsigned int length = 1;
    while ((tree[node].tree.left != -1) && (tree[node].tree.right != -1)) {
        if (subvector(tree[tree[node].tree.left].keys, to)) {
            length++;
            node = tree[node].tree.left;
        } else if (subvector(tree[tree[node].tree.right].keys, to)) {
            length++;
            node = tree[node].tree.right;
        }else{
            std::cout<<"chyba polozka neni ve stromu!!!\n";
        }
    }
    return length;
}
/**
 * generate bit code for current value
 * @param to
 * @return
 */
std::vector<bool> BTree::generateCode(std::vector<uint8_t> to) {
    auto node = root;
    std::vector<bool> tmp;
    while ((tree[node].tree.left != -1) && (tree[node].tree.right != -1)) {
        if (subvector(tree[tree[node].tree.left].keys, to)) {
            tmp.emplace_back(false);
            node = tree[node].tree.left;
        } else if (subvector(tree[tree[node].tree.right].keys, to)) {
            tmp.emplace_back(true);
            node = tree[node].tree.right;
        }else{
            tree[node].print();
            std::cout<<"chyba polozka neni ve stromu!!!\n";
        }
    }
    if (tree.size() == 1){
        tmp.emplace_back(false);
        return tmp;
    }
    return tmp;
}
/**
 * debug function
 * @param book
 */
void BTree::printCodebook(std::vector<std::vector<bool>> book) {
    for(auto item : book){
        for (auto item1 : item){
            std::cout<<item1;
        }
    }
}
/**
 * generate bit code for each value in tree
 * @return
 */
bool BTree::generateCodeBook() {

    for(unsigned i =0; i<tree.size(); i++){
        if(tree[i].tree.parent == -1 || tree[i].keys.size() != 1)
            continue;

        tree[i].code = generateCode(tree[i].keys);
    }
    if (tree.size() == 1)
        tree[0].code = generateCode(tree[0].keys);
    return true;
}
/**
 * sort asc/desc
 * @param flag
 */
void BTree::sortTreeByCodeLength(bool flag) {
    if (flag) {
        std::sort(tree.begin(), tree.end(), [=](mapItem &a, mapItem &b) {
            if (a.codeSize() == b.codeSize())
                return a.keys[0] < b.keys[0];
            else
                return a.codeSize() < b.codeSize();
        });
    }else{
        std::sort(tree.begin(), tree.end(), [=](mapItem &a, mapItem &b) {
            if (a.codeSize() == b.codeSize())
                return a.keys[0] > b.keys[0];
            else
                return a.codeSize() > b.codeSize();
    });
    }
}
/**
 * canonize tree
 * @return caninized tree
 */
std::vector<mapItem> BTree::canonize() {

    std::vector<mapItem> result;
    bool first = true;
    unsigned int c = 0;
    unsigned int l1 = 0;

    for (auto item : tree){
        if (item.keys.size() > 1)
            continue;
        mapItem newItem;
        if(first){
            l1 = item.codeSize();
            for (unsigned int i = 0; i< item.codeSize();i++)
                newItem.code.emplace_back(false);

            newItem.keys = item.keys;
            result.emplace_back(newItem);
            first = false;

            continue;
        }
        c = (c+1)<<(item.codeSize() - l1);
        initBoolVector(newItem.code,c);
        while(newItem.codeSize() < item.codeSize())
            newItem.code.emplace(newItem.code.begin(), false); //adding 0 if is missing
        newItem.keys = item.keys;

        result.emplace_back(newItem);
        l1 = item.codeSize();
    }
    return result;

}

void BTree::sortTreeByKey() {
    std::sort(tree.begin(),tree.end(),[=](mapItem &a,mapItem &b){
       return a.keys[0]<b.keys[0];
    });
}
/**
 * chceck if part is subvector of base
 * @param base
 * @param part
 * @return
 */
bool subvector(std::vector<uint8_t > &base, std::vector<uint8_t > &part){

    for (auto &item : part){
        bool flagIn = false;
        for (auto &itemB : base){
            if (itemB == item) {
                flagIn = true;
                break;
            }
        }
     if (!flagIn)
         return false;
    }
    return true;
}
/**
 *
 * @param v input free vector
 * @param value for writing into vector
 * @param size number of bits to write from value
 * @return return v
 */
std::vector<bool> & initBoolVector(std::vector<bool> &v, unsigned int value){

    double size = floor(log2(value))+1;
    unsigned int bitTemplate = 1;
    for (unsigned int i = 0; i < size; i++){
        ((value & bitTemplate) == bitTemplate) ? v.emplace(v.begin(),true) : v.emplace(v.begin(),false);
        bitTemplate=bitTemplate<<(unsigned int)1;
    }
    return v;

}



