
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
#include "commonDefs.h"

#ifdef VERIFIC_NAMESPACE
using namespace Verific ;
#endif


Array designModules;
std::map<int,std::list<instStruct>> fullDesignGraph;
std::map<int,std::list<instStruct>> fullDesignGraphCopy;
std::vector<modStruct> designModulesVector;
std::vector<clkStruct> designClocksVector;
std::map<VeriModule *,std::list<int>> moduleToGraphLevel;
std::map<VeriModule *,std::list<int>> moduleToGraphLevelCopy;
VeriModule *topModule;
std::map<VeriModule *,std::vector<std::string>> instModulePaths;
std::map<clkStruct,std::vector<std::string>> clockPaths;

extern std::vector<clkStruct> findClocksInDesign(const char *file_name );





int main(int argc, char **argv)
{
    const char *in_file = (argc > 1) ? argv[1] : "test.v" ;
    Array files(1) ;
    files.InsertLast(in_file) ;
    if (!veri_file::Analyze(in_file, veri_file::SYSTEM_VERILOG)) return 1 ;

    Array *top_mod_array = veri_file::GetTopModules() ;
    if (!top_mod_array) {
        Message::Error(0,"Cannot find any top module. Check for recursive instantiation") ;
        return 4 ;
    }
    VeriModule *top  = (VeriModule *) top_mod_array->GetFirst() ;
    topModule = top;
    const char *design_file = (argc > 1) ? argv[1] : "test.v" ;
    designClocksVector = findClocksInDesign(design_file);

    MapIter mi ;
    VeriModule *mod ;
    FOREACH_VERILOG_MODULE(mi, mod) {
        TraverseInstance(mod) ;
    }

    createFullDesignGraph(top);
    for (std::map<int,std::list<instStruct>>::iterator it=fullDesignGraph.begin(); it!=fullDesignGraph.end(); it++) {
        std::list<instStruct> instList = (*it).second;
        std::string instLevelList;
        for (std::list<instStruct>::iterator it=instList.begin(); it!=instList.end(); it++) {
            instLevelList +=" "+ (*it).instance_name+", ";
            VeriModule *modInst = (*it).module;

            std::list<std::string> hierPaths = getAllInstHierPaths(modInst);
            // for ( std::list<std::string>::iterator iit=hierPaths.begin(); iit!=hierPaths.end(); iit++)
            //     printf("Module=%s instance hier path = %s\n",modInst->Name(),/*(*it).instance_name.c_str(),*/(*iit).c_str());
        }
    }

    findAllClocksHierPaths();
    printClksSummary();

    return 0 ;
}

void TraverseInstance(VeriModule *module)
{
    if (!module) return ;
    Array *module_items = module->GetModuleItems() ;
    if (!module_items) return ;

    module->Info("Found module '%s', will visit its items", module->Name()) ;
    modStruct moduleToAdd;
    moduleToAdd.name = module->Name();
    moduleToAdd.ports = module->GetPorts();
    moduleToAdd.module = module;
    Array items_to_look(*module_items) ;
    unsigned i ;
    VeriModuleItem *item ;
    FOREACH_ARRAY_ITEM(&items_to_look, i, item) {
        if (!item) continue ;
        if (item->IsInstantiation()) {
            instStruct *addInst = new instStruct;
            VeriModule *instModule= item->GetInstantiatedModule();
            if(!instModule)
                continue;
            //printf("Found instance = %s\n",instModule->Name());
            Array *instModPorts = instModule->GetPorts();
            addInst->module = instModule;
            unsigned ii ;
            VeriModuleItem *port ;
            FOREACH_ARRAY_ITEM(instModPorts, ii, port) {
                char *pp_str = port->GetPrettyPrintedString() ;
                unsigned str_len = Strings::len(pp_str) ;
                if (str_len && pp_str && (pp_str[str_len-1] == '\n'))
                    pp_str[str_len-1] = '\0' ;
                Strings::free(pp_str) ; pp_str = 0 ;
                continue ;
            }

            Array *instConnects =  instModule->GetItems();
            unsigned iic ;
            VeriExpression *connect ;
            FOREACH_ARRAY_ITEM(instConnects, iic, connect) {
                char *pp_str = connect->GetPrettyPrintedString() ;
                unsigned str_len = Strings::len(pp_str) ;
                if (str_len && pp_str && (pp_str[str_len-1] == '\n')) pp_str[str_len-1] = '\0' ;
                Strings::free(pp_str) ; pp_str = 0 ;
                continue ;
            }

            char *pp_str = item->GetPrettyPrintedString() ;
            unsigned str_len = Strings::len(pp_str) ;
            if (str_len && pp_str && (pp_str[str_len-1] == '\n')) {
                pp_str[str_len-1] = '\0' ;
            } else continue;
            item->Info("      => Found instance %s", pp_str) ;
            std::string stdPPString =std::string(pp_str);
            std::string splitPPString = stdPPString.substr(stdPPString.find('.') + 1);
            addInst->connections = extractInstPortConnections(splitPPString);
            addInst->instance_name = extractInstName(std::string(pp_str),instModule->Name());
            moduleToAdd.instances.Insert(addInst);
            Strings::free(pp_str) ; pp_str = 0 ;
            continue ;
        }
    }
    designModulesVector.push_back(moduleToAdd);
    designModules.Insert(&moduleToAdd);
}


#include <sstream>
#include <string.h>

std::map<std::string,std::string> extractInstPortConnections( std::string str/*, Array *portList*/) {

    Map portSignalMap(STRING_HASH,1024);
    std::map<std::string,std::string> retMap;
    size_t end = str.find("))", 0);

    std::string content = str.substr(0, end );
    std::stringstream ss(content);
    std::string pair;

    while (std::getline(ss, pair, ',')) {
        pair.erase(0, pair.find_first_not_of(" \t"));
        pair.erase(pair.find_last_not_of(" \t") + 1);
        size_t parenPos = pair.find('(');
        if (parenPos != std::string::npos) {
            std::string port = pair.substr(0, parenPos);
            std::string signal = pair.substr(parenPos + 1);
            if (!port.empty() && port.front() == '.') {
                port.erase(0,1);
            }
            if (!signal.empty() && signal.back() == ')') {
                signal.pop_back();
            }
            char * portStr = new char[port.size()+1];
            strcpy(portStr,port.c_str());
            char * signalStr = new char[signal.size()+1];
            strcpy(signalStr,signal.c_str());
            retMap[portStr] = signalStr;
            portSignalMap.Insert(portStr,signalStr);
        }
    }

    //    MapIter mi ;
    //    char *key ;
    //    char *value ;
    //    FOREACH_MAP_ITEM(&portSignalMap,mi,&key,&value) {
    //                if (key && value) {
    //                   // printf("Map key=%s , value=%s\n",key,value);
    //                }
    //           }
    return  retMap;
}

char *extractInstName( std::string str, std::string module_name) {

    size_t start = str.find(module_name.c_str());
    size_t end = str.find("(", 0);

    std::string content = str.substr(start, end );
    content.erase(0, content.find_first_not_of(module_name.c_str()));
    content.erase(0, content.find_first_not_of(" \t"));
    content.erase(content.find_last_not_of(" \t") + 1);
    char * instName = new char[content.size()+1];
    strcpy(instName,content.c_str());
    if(!instName)
        strcpy(instName,"");
    return instName;
}

void createFullDesignGraph(VeriModule *top)
{
    if(!top) return;

    instStruct topInst;
    std::list<instStruct> topInstList;
    topInst.module = top;
    topInst.instance_name = top->Name();
    std::list<instStruct> instList;
    for (std::vector<modStruct>::iterator it=designModulesVector.begin(); it!=designModulesVector.end(); it++) {
        if(Strings::compare(top->Name(), (*it).name.c_str())) {
            topInst.connections["topCon"] = "Instance";
            Array topInstArray = (*it).instances;
            unsigned i;
            instStruct *inst;
            FOREACH_ARRAY_ITEM(&topInstArray, i, inst) {
                if(inst)
                    topInstList.push_back(*inst);
            }
            break;
        }
        // printf("designModulesVector inserted module = %s\n",(*it).name.c_str());
    }
    instList.push_back(topInst);
    fullDesignGraph.insert(std::pair<int,std::list<instStruct>>(0,instList));
    fullDesignGraph.insert(std::pair<int,std::list<instStruct>>(1,topInstList));
    //Start from 2 because previous two lines!!!
    int level = 2;
    recTraverseInstances(level,topInstList);

    for (std::map<int,std::list<instStruct>>::iterator it=fullDesignGraph.begin(); it!=fullDesignGraph.end(); it++)
    {
        std::list<instStruct> instList = (*it).second;
        for (std::list<instStruct>::iterator itL=instList.begin(); itL!=instList.end(); itL++)
        {
            insertInModuleToGraphLevel((*itL).module,(*it).first);
        }
    }

}

void recTraverseInstances(int level,std::list<instStruct> instList)
{
    for ( std::list<instStruct>::iterator it=instList.begin(); it!=instList.end(); it++) {
        VeriModule *instModule = (*it).module;
        //printf(">>>>>>> in MODULE %s\n",instModule->Name());

        static bool increaseLevel = false;
        bool found = false;
        for (std::vector<modStruct>::iterator it=designModulesVector.begin(); it!=designModulesVector.end(); it++) {
            if(Strings::compare(instModule->Name(), (*it).name.c_str())) {
                Array instModArray = (*it).instances;
                if(instModArray.Size()) {
                    unsigned i;
                    instStruct *inst;
                    std::list<instStruct>instInstArray;
                    FOREACH_ARRAY_ITEM(&instModArray, i, inst) {
                        instInstArray.push_back(*inst);
                    }
                    fullDesignGraph.insert(std::pair<int,std::list<instStruct>>(level,instInstArray));
                    if(!increaseLevel) {
                        level++;
                        increaseLevel = true;
                    }
                    recTraverseInstances(level,instInstArray);
                }
                else {
                    --level;
                    return;
                }
                found = true;
            }
            if(found) {
                break;
            }
        }
    }
}

std::string stringFlattenMap(std::map<std::string,std::string> mapToFlat) {

    std::string flattenStringMap;
    std::string keyFlattenStr;
    std::string valFlattenStr;
    for(std::map<std::string,std::string>::iterator it=mapToFlat.begin(); it!=mapToFlat.end();it++) {
        keyFlattenStr+=(*it).first;
        valFlattenStr+=(*it).second;
    }
    flattenStringMap = keyFlattenStr+"." +valFlattenStr;
    return  flattenStringMap;
}

void insertInModuleToGraphLevel(VeriModule *module,int level)
{
    if(!module) return;
    //printf(">>> Insert MODULE =%s\n",module->Name());
    if(moduleToGraphLevel.find( module ) != moduleToGraphLevel.end())
    {
        moduleToGraphLevel.at(module).push_back(level);
    } else {
        std::list<int> moduleLevels;
        moduleLevels.push_back(level);
        moduleToGraphLevel.insert(std::pair<VeriModule *,std::list<int>>(module,moduleLevels));
    }
    return;
}

std::list<std::string> getAllInstHierPaths(VeriModule *module)
{
    if(!module)
        return std::list<std::string>();
    // printf(">>>>>>>>>>> getAllInstHierPaths MODULE =%s\n",module->Name());
    std::list<std::string> retList;
    moduleToGraphLevelCopy = moduleToGraphLevel;
    fullDesignGraphCopy = fullDesignGraph;
    std::list<int> tmpInstList =  moduleToGraphLevel.at(module);
    for(int i=0;i<(int)tmpInstList.size();++i) {
        if(moduleToGraphLevel.find( module ) != moduleToGraphLevel.end()) {
            std::string foundPath = getInstHierPath(module,true);
            retList.push_back(foundPath);
            if(instModulePaths.find( module ) != instModulePaths.end())
            {
                instModulePaths.at(module).push_back(foundPath);
            } else {
                std::vector<std::string> pathsList;
                pathsList.push_back(foundPath);
                instModulePaths.insert(std::pair<VeriModule *,std::vector<std::string>>(module,pathsList));
            }
        }
    }
    return  retList;
}

std::string getInstHierPath(VeriModule *module,bool resetPath=false)
{
    if(!module)
        return std::string();
    std::string path;
    static std::string sameRetPath;
    // printf(">>> getInstHierPath MODULE =%s\n",module->Name());
    if(module ==topModule) {
        path = std::string(topModule->Name()) +"/";
        return  path;
    }
    std::list<std::string> retList;
    if(moduleToGraphLevel.find( module ) == moduleToGraphLevel.end())
        return std::string();

    static std::list<std::string> instNamesListChecked;
    std::list<int> instanceLevels = moduleToGraphLevelCopy.at(module);
    std::list<instStruct> instanceTopList = fullDesignGraph.at(0);
    instStruct instanceTop = instanceTopList.front();
    static std::vector<instStruct>vecStrPath;
    if(resetPath)
        vecStrPath.clear();
    vecStrPath.push_back(instanceTop);
    bool foundInst = false;
    static int numberLevelToCheck = -1;
    int checkedLevel =0;
    for (std::list<int>::iterator it=instanceLevels.begin(); it!=instanceLevels.end(); it++)
    {
        std::list<instStruct> instanceList/* = fullDesignGraph.at(*it)*/;
        instanceList = fullDesignGraphCopy.at(*it);
        std::list<instStruct> instanceListFull = fullDesignGraph.at(*it);
        checkedLevel = (*it);

        std::list<std::string> instNamesListA;
        if(0==(*it))
            continue;
        instNamesListA =  convertInsListToStringList(instanceListFull);
        if(instanceList.empty())
            continue;
        instStruct insStrA = instanceList.front();  //HERE IS PROBLEM!!!!!!!!!!!!
        if(instListContainModule(instanceList,module))
            insStrA = getInstFromInstList(instanceList,module);
        if(instanceList.size()>1)
            instanceList.pop_front();
        for (std::vector<modStruct>::iterator iit=designModulesVector.begin(); iit!=designModulesVector.end(); iit++)
        {
            modStruct modStr = (*iit);
            std::list<instStruct> modStrInstanceListToCompare;
            Array modInst = modStr.instances;
            std::list<std::string> instNamesListB;
            unsigned i ;
            instStruct *modIns ;
            FOREACH_ARRAY_ITEM(&modInst, i, modIns) {
                if(modIns) {
                    instNamesListB.push_back(modIns->instance_name);
                }
            }
            if((instNamesListA == instNamesListB) /*&& (instNamesListChecked!=instNamesListB)*/) {
                if(instNamesListChecked.empty()) {
                    instNamesListChecked = instNamesListB;
                }
                VeriModule *foundModule = modStr.module;
                if(foundModule) {
                    // printf(">>> found MODULE =%s\n",foundModule->Name());
                    vecStrPath.push_back(insStrA);
                    path +=insStrA.instance_name+"/" + getInstHierPath(foundModule,false);
                    if(sameRetPath.empty())
                        sameRetPath = path;
                    foundInst = true;
                    if(foundModule ==topModule)
                        break;
                }
                fullDesignGraphCopy[checkedLevel] = instanceList;
            }
        }

        instanceLevels.pop_front();
        if(foundInst)
            break;

    }
    return  path;
}

std::list<std::string> convertInsListToStringList(std::list<instStruct> instanceList)
{
    std::list<std::string> retList;
    for (std::list<instStruct>::iterator it=instanceList.begin(); it!=instanceList.end(); ++it)
    {
        instStruct insStr = (*it);
        if( it!=instanceList.end())
            retList.push_back((*it).instance_name);
    }
    return  retList;
}

bool instListContainModule(std::list<instStruct> instanceList,VeriModule *mod)
{
    for (std::list<instStruct>::iterator it=instanceList.begin(); it!=instanceList.end(); ++it)
    {
        instStruct retInst = (*it);
        if(retInst.module == mod)
            return  true;
    }
    return  false;
}

instStruct getInstFromInstList(std::list<instStruct> instanceList,VeriModule *mod)
{
    instStruct retInst;
    for (std::list<instStruct>::iterator it=instanceList.begin(); it!=instanceList.end(); ++it)
    {
        retInst = (*it);
        if(retInst.module == mod)
            return  retInst;
    }
    return  retInst;
}

#include <algorithm>

bool checkInstPathIsInsert( VeriModule *mod,std::string path)
{
    if(!mod)
        return false;
    std::map<VeriModule *,std::vector<std::string>>::iterator it = instModulePaths.find(mod);
    if (it != instModulePaths.end()) {
        std::vector<std::string> modPathsList = instModulePaths.at(mod);
        if(std::find(modPathsList.begin(), modPathsList.end(), path) != modPathsList.end())
            return  true;
    }
    return false;
}

void findAllClocksHierPaths()
{
    for (std::vector<clkStruct>::iterator it=designClocksVector.begin(); it!=designClocksVector.end(); it++)
    {
        clkStruct clk = (*it);
        VeriModule *module = clk.module;
        if(!module)
            return;
        if(instModulePaths.find( module ) == instModulePaths.end())
            continue;
        std::vector<std::string> instModPaths = instModulePaths.at(module);
        if(!instModPaths.empty()) {
            std::vector<std::string> paths;
            for (std::vector<std::string>::iterator iit=instModPaths.begin(); iit!=instModPaths.end(); iit++) {
                std::string insPath = (*iit);
                std::string revInsPath = reversInstPath(insPath);
                revInsPath+=/* "/"+*/clk.port;
                paths.push_back(revInsPath);
            }
            clockPaths.insert(std::pair<clkStruct, std::vector<std::string>>(clk,paths));
        }
    }

    return ;
}

#include "support_funcs.h"

std::string reversInstPath(std::string path) {
    std::string reversedPath;
    char * cString = const_cast<char*>(path.c_str());
    std::vector<char *> splitPath =  splitString(cString,'/');
    std::reverse(splitPath.begin(), splitPath.end());
    for (std::vector<char *>::iterator it = splitPath.begin() ; it != splitPath.end(); ++it) {
        reversedPath+= std::string(*it) + "/";
    }
    return  reversedPath;
}



void printClksSummary() {

    std::map<clkStruct,std::vector<std::string>> derivedClocks;
    std::map<clkStruct,std::vector<std::string>> masterClocks;
    bool printOne = true;
    for (std::map<clkStruct,std::vector<std::string>>::iterator it=clockPaths.begin(); it!=clockPaths.end(); it++) {
        clkStruct clkStr = (*it).first;
        if(clkStr.isGenClk)
            derivedClocks.insert(*it);
        if(clkStr.isMasterClk)
            masterClocks.insert(*it);
    }
    printf("\n ##############################################################################\n");
    printf(" #\tDesign summary:\n");
    printf(" ##############################################################################\n");
    printf(" #\tTop module: \t\t\t\t%s\n",topModule->Name());
    printf(" ##############################################################################\n\n");

    printf(" ##############################################################################\n");
    printf(" #\tMaster clocks:\n");
    printf(" #-----------------------------------------------------------------------------\n");
    for (std::map<clkStruct,std::vector<std::string>>::iterator it=masterClocks.begin(); it!=masterClocks.end(); it++) {
        clkStruct clkStr = (*it).first;
        std::vector<std::string> clkPathList = (*it).second;
        printf(" #\tClock name:\t\t\t\t %s\n",clkStr.name.c_str());
        printf(" #\tClocktree paths:");
        for (std::vector<std::string>::iterator iit=clkPathList.begin(); iit!=clkPathList.end(); iit++) {
            printOne? printf(" \t\t\t %s\n",(*iit).c_str()) : printf(" #\t\t\t\t\t\t %s\n",(*iit).c_str());
            printOne =false;
        }
        printf(" #-----------------------------------------------------------------------------\n");
        printOne =true;
    }

    printf(" #\tDerived/Generated clocks:\n");
    printf(" #-----------------------------------------------------------------------------\n");
    printOne =true;
    for (std::map<clkStruct,std::vector<std::string>>::iterator it=derivedClocks.begin(); it!=derivedClocks.end(); it++) {
        clkStruct clkStr = (*it).first;
        std::vector<std::string> clkPathList = (*it).second;
        printf(" #\tClock name:\t\t\t\t %s\n",clkStr.name.c_str());
        printf(" #\tClocktree paths:");
        for (std::vector<std::string>::iterator iit=clkPathList.begin(); iit!=clkPathList.end(); iit++) {
            printOne? printf(" \t\t\t %s\n",(*iit).c_str()) : printf(" #\t\t\t\t\t\t %s\n",(*iit).c_str());
            printOne =false;
        }
        printf(" #-----------------------------------------------------------------------------\n");
        printOne =true;
    }
    printf(" ##############################################################################\n\n");
    printf(" ##############################################################################\n");
    printf(" #\tEnd of Design summary!\n");
    printf(" ##############################################################################\n");

}

/*
void * operator new(size_t n) {
   // std::out << "allocate: " << n << " bytes\n";
    printf("allocate: %d bytes\n",n);
    return malloc(n);
}*/

