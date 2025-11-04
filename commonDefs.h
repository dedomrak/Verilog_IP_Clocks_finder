#ifndef COMMONDEFS_H
#define COMMONDEFS_H

#include "./verilog/veri_file.h"
#include "./verilog/VeriModule.h"
#include "./verilog/VeriMisc.h"
#include "./verilog/VeriExpression.h"

#include "./util/Strings.h"
#include "./containers/Array.h" // a dynamic array
#include "./containers/Set.h"   // a simple hash table
#include "./containers/Map.h"
#include <vector>
#include <map>
#include <list>

#ifdef VERIFIC_NAMESPACE
using namespace Verific ;
#endif
struct instStruct {
    VeriModule *module;
    std::string instance_name;
    std::map<std::string,std::string> connections;
};

struct modStruct {
    std::string name;
    VeriModule *module;
    Array *ports;
    Array instances;
};

struct clkStruct {
    std::string name;
    VeriModule *module = 0;
    std::string port ;
    bool isMasterClk = false;
    bool isGenClk = false;
    friend bool operator<(const clkStruct& left, const clkStruct& right)
    {
        return (left.name) <= (right.name);
    }
};

std::map<std::string, std::string> extractInstPortConnections(std::string str);
char *extractInstName( std::string str, std::string module_name);
void recTraverseInstances(int level,std::list<instStruct> instList);
void createFullDesignGraph(VeriModule *top);
std::list<std::string> getAllInstHierPaths(VeriModule *module);
std::string getInstHierPath(VeriModule *module, bool resetPath);
std::string stringFlattenMap(std::map<std::string,std::string> map);
void insertInModuleToGraphLevel(VeriModule *module,int level);
std::list<std::string> convertInsListToStringList(std::list<instStruct> instanceList);
bool checkInstPathIsInsert( VeriModule *mod,std::string path);
bool instListContainModule(std::list<instStruct> instanceList,VeriModule *mod);
instStruct getInstFromInstList(std::list<instStruct> instanceList,VeriModule *mod);
void findAllClocksHierPaths();
std::string reversInstPath(std::string path);
void printClksSummary();
void TraverseInstance(VeriModule *module);


#endif // COMMONDEFS_H
