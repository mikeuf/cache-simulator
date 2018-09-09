/**
 * cacheSim.cpp
 * Basic cache simulator.
 *
 * This program simulates a memory cache based on a source
 * file provided to it. It prints a report based on its
 * activity.
 ************************************************************/

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <bitset>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

// for readability
typedef boost::tokenizer< boost::char_separator<char> > Tokens;

// tracks whether a memory reference is read or write
enum class ReadOrWrite {ERROR, READ, WRITE};

class MemRef {
/* keeps track of memory references. this is used for comparison with
the cache table and for printing the summary at the end */

  public:

    // parameterized constructor
    MemRef(int refNum, ReadOrWrite rW, int size, unsigned long address)
      : refNum_(refNum), rW_(rW), size_(size), address_(address) {}

  public:

    // setters
    void setHM(bool hM) {
      hM_ = hM;
    }

    void setNewTag(unsigned long newTag) {
      newTag_ = newTag;
    }

    // getters 
    ReadOrWrite getRW() {
      return rW_;
    }

    int getRefNum() {
      return refNum_;
    }

    unsigned long getAddress() {
      return address_;
    }

    unsigned long getTag() {
      return tag_;
    }

    unsigned long getNewTag() {
      return newTag_;
    }

    int getIndex() {
      return index_;
    }

    int getOffset() {
      return offset_;
    }

    bool getHM() {
      return hM_;
    }

    int getSize() {
      return size_;
    }

    // these calculate various parts of the cache line
    void calculate_tag(unsigned long indexSize, unsigned long offsetSize) {
      tag_ = address_ >> (indexSize + offsetSize);
    }

    void calculate_index(unsigned long indexMask, unsigned long offsetSize) {
      index_ = address_ & indexMask;
      index_ = index_ >> offsetSize;
    }

    void calculate_offset(unsigned long offsetMask) {
      offset_ = address_ & offsetMask;
    }

  private:

    ReadOrWrite 
      rW_;

    int 
      refNum_,
      size_;

    bool 
      hM_;

    unsigned long
      address_,
      tag_,
      newTag_,
      index_,
      offset_;

}; // end class MemRef


class CacheLine {

  /* these make up a set, and store the actual tags */

  public:

    // constructors
    CacheLine() {}

    CacheLine(unsigned long tag) : tag_(tag) {}

    void set_LRU() {
      // set LRU for most recently used to 0
      LRUFlag_ = 0;
    }

    void setTag(unsigned long tag) {
      tag_ = tag;
    }

    unsigned long get_LRU() {
      return LRUFlag_;
    }

    unsigned long getTag() {
      return tag_;
    }

    void increment_LRU() {
      // increment LRU by 1 to indicate aging
      LRUFlag_++;
    }

  private:

    unsigned long 
      tag_,
      LRUFlag_;

    bool
      valid_;

}; // end class CacheLine


class CacheSet {

  /* this is a set of cacheLines. the size varies */

  public: 

    // constructors
    CacheSet(int setSize) : setSize_(setSize) {}

    unsigned int getIndex() {
      return index_;
    }

    // not used, but can be used to generate blank entries for a set
    void create_cache_lines() {
      for (int i = 0; i < setSize_; ++i)
      {
        CacheLine *cacheLine = new CacheLine;
        cacheLine_.push_back(*cacheLine);
      }
    }

    // adds just one cache line
    void add_new_cache_line(unsigned long tag) {
      CacheLine *cacheLine = new CacheLine(tag);
      cacheLine->set_LRU();
      cacheLine_.push_back(*cacheLine);
    }

    // checks cache lines in a set for a tag
    bool check_cache_lines(unsigned long tag) {
      for (std::vector<CacheLine>::iterator it = cacheLine_.begin(); 
          it != cacheLine_.end(); ++it) {
        // compare the LRU of the currentLRU to the cacheline and update
        if (tag == it->getTag()) {
          // HIT
          // update LRU for that entry
          update_LRUs();
          it->set_LRU();
          return true;
        }
      }
      // if tag was not found in set
      return false;
    }

    // increment all LRUs
    void update_LRUs() {
      for (std::vector<CacheLine>::iterator it = cacheLine_.begin(); 
          it != cacheLine_.end(); ++it) {
        it->increment_LRU();
      }
    }

    // update tag for a cache entry
    void update_cache_lines(unsigned long tag) {
      // is there room for a new entry?
      if (cacheLine_.size() < setSize_) {
        add_new_cache_line(tag);
      } else {
        // if no room, then replace the LRU entry
        std::vector<CacheLine>::iterator lineToReplace = find_LRU();
        update_LRUs();
        lineToReplace->setTag(tag);
        lineToReplace->set_LRU();

      }
    }

    // returns an iterator to the LRU cacheLine
    std::vector<CacheLine>::iterator find_LRU() {

      // points to LRU entry
      std::vector<CacheLine>::iterator currentLRU = cacheLine_.begin();

      // iterate through the cache lines in the set
      for (std::vector<CacheLine>::iterator it = cacheLine_.begin(); 
          it != cacheLine_.end(); ++it) {

        // compare the LRU of the currentLRU to the cacheline and update
        if (currentLRU->get_LRU() < it->get_LRU()) {
          currentLRU = it;
        }
      }
      return currentLRU;
    }

    void setIndex(unsigned long index) {
      index_ = index;
    }

  private:

    unsigned int
      setSize_,
      indexSize_,
      index_;

    std::vector<CacheLine>
      cacheLine_;

}; // end class CacheSet


class CacheTable
{
  /* the main cache table that stores the sets and lines */

  public:

    CacheTable(){}

    // parameterized constructor
    CacheTable 
      (int totalCacheSize, int lineSize, int setSize) 
      : totalCacheSize_(totalCacheSize), lineSize_(lineSize), 
      setSize_(setSize) {}

    int print_summary() {
      std::cout << std::dec
        << "\nTotal Cache Size:  " << get_total_cache_size() << "B"
        << "\nLine Size:  " << get_line_size() << "B"
        << "\nSet Size:  " << get_set_size()
        << "\nNumber of Sets:  " << get_number_of_sets() << "\n"
        << std::endl;

      // much of this formatting is from Dr. Hughes supplement

      std::cout << std::setw(8)  << std::left << "RefNum";
      std::cout << std::setw(10) << std::left << "  R/W";
      std::cout << std::setw(13) << std::left << "Address";
      std::cout << std::setw(6)  << std::left << "Tag";
      std::cout << std::setw(8)  << std::left << "Index";
      std::cout << std::setw(10) << std::left << "Offset";
      std::cout << std::setw(8)  << std::left << "H/M";
      std::cout << std::setfill('*') << std::setw(64) << "\n" << std::setfill(' ');
      std::cout << "\n";

      std::vector<MemRef>::iterator it = memRef_.begin();
      for (int i = 0; i < (memRef_.size()); ++i) {

        std::cout << "   " << std::setw(5) << std::left << std::dec 
          << it->getRefNum();

        if (it->getRW() == ReadOrWrite::READ) {
          std::cout << std::setw(8) << " Read";
        } else {
          std::cout << std::setw(8) << "Write";
        }

        std::cout << "  " << std::setfill('0') << std::setw(8) 
          << std::right << std::hex << it->getAddress();
        std::cout << std::setfill(' ') << std::setw(7) << it->getTag()

          << std::setw(8) << std::dec << it->getIndex()
          << std::setw(8) << it->getOffset();

        if (it->getHM() == false) {
          std::cout << std::setw(10) << "Miss";
        } else {
          std::cout << std::setw(10) << "Hit";
        }
        std::cout << std::dec << std::endl;

        ++it;
      }

      // cast as doubles for division
      hitRate = (double)(totalHits) / (double)totalAccess;
      missRate = (double)totalMiss / (double)totalAccess;

      std::cout << "\n";
      std::cout << "    Simulation Summary\n";
      std::cout << "**************************\n";
      std::cout << "Total Hits:\t"   << (totalHits) << "\n";
      std::cout << "Total Misses:\t" << totalMiss << "\n";
      std::cout << "Hit Rate:\t"     << std::setprecision(5) << hitRate   << "\n";
      std::cout << "Miss Rate:\t"    << std::setprecision(5) << missRate  << "\n";

    }

    void increment_number_of_sets() {
      numberOfSets_++;
    }

    void calculate_number_of_sets() {
      // make sure lineSize_ > 0
      if ((lineSize_ != 0) && (setSize_ != 0)) {
        numberOfSets_ = (totalCacheSize_ / lineSize_) / setSize_;
      }
    }

    // these calculate the dimensions of various cache properties
    void calculate_index_size() {
      indexSize_ = log2(numberOfSets_);
    }

    void calculate_offset_size() {
      offsetSize_ = log2(lineSize_);
    }

    void calculate_tag_size() {
      tagSize_ = (lineSize_ * 8) - indexSize_ - offsetSize_;
    }

    void calculate_offset_mask() {
      offsetMask_ = pow(2, offsetSize_) - 1;
    }

    void calculate_index_mask() {
      indexMask_ = (pow(2, (indexSize_ + offsetSize_)) - 1) - offsetMask_;
    }

    void calculate_tag_mask() {
      tagMask_ = pow(2, (lineSize_ * 8)) - 1 - indexMask_ - offsetMask_;
    }

    // reads the cache configuration files
    int read_cache_config(char* filename) {
      // open the input file
      std::ifstream is;
      // read first file
      is.open(filename, std::ios::in);
      // return 1 if file not found
      if (is.fail()) {
        std::cerr << "\nError opening file: \"" << filename 
          << "\"\n" << std::endl;
        return 1;
      }
      is >> setSize_;
      is >> lineSize_;
      is >> totalCacheSize_;

      is.close();
    }

    // reads and parses the memory trace files 
    int read_mem_trace(char* filename) {
      /* The memory trace should have the format: 
         <accesstype>:<size>:<hexaddress>
         */
      // open the input file
      std::ifstream is;
      std::string input;
      // read first file
      is.open(filename, std::ios::in); // input stream
      // return 1 if file not found
      if (is.fail()) {
        std::cerr << "\nError opening file: \"" << filename 
          << "\"\n" << std::endl;
        return 1;
      }
      int refNum = 0;
      ReadOrWrite rW = ReadOrWrite::ERROR;
      int size = 0;
      unsigned long address = 0;


      // clearing the whitespace seemed necessary for the getline
        std::ws(is);
      while (std::getline(is, input)) {

        std::ws(is);
        // define delimeters
        boost::char_separator<char> delimeter(":");

        // tokenize string according to delimeter
        Tokens tokens(input, delimeter);

        // iterate through tokens
        Tokens::iterator token = tokens.begin();
        if (*token == "R") {
          rW = ReadOrWrite::READ;
        } 
        else if (*token == "W") {
          rW = ReadOrWrite::WRITE;
        }

        // grab the size and convert to an int
        size = atoi((*(++token)).c_str());

        // grab address and convert to unsigned long
        address = strtoul((*(++token)).c_str(), NULL, 16);


        // create & configure new MemRef based on info that was just read
        MemRef *memRef = new MemRef(refNum, rW, size, address);
        memRef->calculate_tag(indexSize_, offsetSize_);
        memRef->calculate_index(indexMask_, offsetSize_);
        memRef->calculate_offset(offsetMask_);
        //memRef->setNewTag(memRef->getTag());

        // set hit or miss for memRef based on return from determine function
        memRef->setHM(determine_hit_or_miss(memRef->getIndex(), memRef->getTag()));
        memRef_.push_back(*memRef); 

        refNum++;
        totalAccess++;
      }
    }


    // determine whether the mem reference was a hit or miss
    bool determine_hit_or_miss(unsigned long index, unsigned long tag) {
      // iterate through all cacheSets and
      // compare memRef index to cache lines index

      for (std::vector<CacheSet>::iterator it = cacheSet_.begin(); 
          it != cacheSet_.end(); ++it) {

        // does memRef index match cacheSets index?
        if (index == it->getIndex()) {
          // if index match
          // compare memRef tag to cache lines tag for that cache set

          if (it->check_cache_lines(tag)) {
            // if tag matches cacheline then report hit
            totalHits++;
            return true;
          } else {
            // if no match
            it->update_cache_lines(tag);
            break;
          }
        } 
        }
          // if no index match
          // then MISS
          totalMiss++;
          return false;
    }

    // generates the cache sets according to info from config file
    void create_cache_sets(int numberOfSets_) {
      for (int i = 0; i < numberOfSets_; ++i) {
        CacheSet *cacheSet = new CacheSet(setSize_);
        cacheSet_.push_back(*cacheSet); 
      }
    }

    // iterate through all of the cacheSets and set their index numbers
    void set_index_for_cache_sets() {
      int indexSetter = 0;
      for (std::vector<CacheSet>::iterator it = cacheSet_.begin(); 
          it != cacheSet_.end(); ++it) {
        it->setIndex(indexSetter++);
      }
    }

    // setters
    int set_total_cache_size(int totalCacheSize) {
      totalCacheSize_ = totalCacheSize;
    }

    int set_line_size(int lineSize) {
      lineSize_ = lineSize;
    }

    int set_set_size(int setSize) {
      setSize_ = setSize;
    }

    // getters
    int get_total_cache_size() {
      return totalCacheSize_;
    }

    int get_line_size() {
      return lineSize_;
    }

    int get_set_size() {
      return setSize_;
    }

    int get_number_of_sets() {
      return numberOfSets_;
    }

  private:

    std::vector<CacheSet> 
      cacheSet_;

    std::vector<MemRef> 
      memRef_;

    int 
      totalCacheSize_,
      lineSize_,
      setSize_,
      numberOfSets_,
      indexSize_,
      tagSize_,
      offsetSize_,
      totalHits,
      totalMiss,
      totalAccess;

    unsigned long 
      offsetMask_,
      indexMask_,
      tagMask_;

    double
      hitRate,
      missRate;

}; // end class CacheTable

int main(int argc, char* argv[]) {

  if (argc != 2) {
// create and config a cache table
    CacheTable *cacheTable = new CacheTable;

    cacheTable->read_cache_config(argv[1]);
    cacheTable->calculate_number_of_sets();
    cacheTable->create_cache_sets(cacheTable->get_number_of_sets());
    cacheTable->set_index_for_cache_sets();
    cacheTable->calculate_index_size();
    cacheTable->calculate_offset_size();
    cacheTable->calculate_tag_size();
    cacheTable->calculate_offset_mask();
    cacheTable->calculate_index_mask();
    cacheTable->calculate_tag_mask();

    // parse memory trace and print summary
    cacheTable->read_mem_trace(argv[2]);
    cacheTable->print_summary();

    delete cacheTable;
  } else {
    // error if bad syntax
    std::cout << "\nSyntax: cacheSim <cacheConfig> <memTrace>"
      << std::endl;
  }

  return 0;
}

