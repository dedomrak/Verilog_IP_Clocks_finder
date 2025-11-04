#include "./containers/Array.h" // a dynamic array
#include "./containers/Set.h"   // a simple hash table
#include "./containers/Map.h"   // an associated hash table

#include "./util/Strings.h"        // Definition of class to easily create/concatenate char*s.
#include "./util/Message.h"        // Message handler
#include "./util/TextBasedDesignMod.h"  // Text-Based Design-Modification (TextBasedDesignMod) utility

// Verilog parser, main interface :
#include "./verilog/veri_file.h"     // Make Verilog reader available
#include "./verilog/VeriVisitor.h"    // Make the parse-tree visitor pattern available

// Verilog parse tree API :
#include "./verilog/VeriModule.h"     // Definition of a VeriModule
#include "./verilog/VeriModuleItem.h" // Definitions of all verilog module item tree nodes
#include "./verilog/VeriStatement.h"  // Definitions of all verilog statement tree nodes
#include "./verilog/VeriExpression.h" // Definitions of all verilog expression tree nodes
#include "./verilog/VeriId.h"         // Definitions of all identifier definition tree nodes

#include "./verilog/veri_tokens.h"    // Definition of port direction VERI_OUTPUT, etc ...
#include <vector>
#include "commonDefs.h"

#ifdef VERIFIC_NAMESPACE
namespace Verific {
#endif

class EventControlVisitor : public VeriVisitor
{
public :
    EventControlVisitor() : VeriVisitor(), _event_control_stmts() { } ;
    ~EventControlVisitor() {} ;

    virtual void VERI_VISIT(VeriInitialConstruct, node) {
        node.Info("skipping this initial block") ;
    }

    // Collect event control statements :
    virtual void VERI_VISIT(VeriEventControlStatement, node) {
        node.Info("visiting this event control statement") ;
        _event_control_stmts.InsertLast(&node) ;
    }

public :
    Array _event_control_stmts ; // array of VeriEventControlStatement *'s, collected by this visitor
} ;

class FindReferencedIds : public VeriVisitor
{
public :
    FindReferencedIds() : VeriVisitor(), _referenced_ids(POINTER_HASH) { } ;
    ~FindReferencedIds() {} ;

    virtual void VERI_VISIT(VeriIdRef, node) {
        VeriIdDef *id = node.GetId() ;
        _referenced_ids.Insert(id) ;
    }

public :
    Set _referenced_ids ;
} ;

#ifdef VERIFIC_NAMESPACE
} ;
#endif


#ifdef VERIFIC_NAMESPACE
using namespace Verific ;
#endif

extern VeriModule *topModule;

std::vector<clkStruct> findClocksInDesign(const char *file_name )
{

    // if (!veri_file::Analyze(file_name, veri_file::SYSTEM_VERILOG)) return 1 ;

    std::vector<clkStruct> retVector;
    Array transition_table ;
    Array instantiation_table ;
    Array module_table ;
    Array *tops = veri_file::GetTopModules() ;

    MapIter mi1 ;
    VeriModule *mod, *top ;
    unsigned i ;
    FOREACH_VERILOG_MODULE(mi1, mod) {
        if (!mod || mod->IsPackage() || mod->IsRootModule()) continue ;
        module_table.Insert(mod) ;
    }

    unsigned mc ;
    FOREACH_ARRAY_ITEM(&module_table, mc, mod) {
        unsigned found = 0 ;
        FOREACH_ARRAY_ITEM(tops, i, top) {
            if (Strings::compare(top->Name(), mod->Name())) {
                found = 1 ;
                break ;
            }
        }
        //        Array *ports = mod->GetPorts();
        //        if (ports) {
        //            VeriIdDef *port ;

        //            FOREACH_ARRAY_ITEM(ports, i, port) {
        //                printf("PORT:  %s\n",port->Name() );
        //            }
        //        }
    }

    VeriModule *module ;
    MapIter mi ;
    FOREACH_VERILOG_MODULE(mi, module) {
        if (!module) continue ;
        module->Info("Searching module %s for clocked statements...", module->Name()) ;
        EventControlVisitor event_control_visitor ;
        module->Accept( event_control_visitor ) ;

        clkStruct foundClk;
        VeriEventControlStatement *event_control_stmt ;
        unsigned i, j ;
        FOREACH_ARRAY_ITEM(&(event_control_visitor._event_control_stmts), i, event_control_stmt) {
            if (!event_control_stmt) continue ;
            Array *sens_list = event_control_stmt->GetAt() ;
            unsigned is_clocked = 0 ;
            VeriExpression *event_expr ;
            FOREACH_ARRAY_ITEM(sens_list, j, event_expr) {
                if (!event_expr) break ;
                if (event_expr->IsEdge(0/*any edge (pos or neg)*/)) {
                    is_clocked = 1 ;
                    break ;
                }
            }
            if (!is_clocked) continue ;
            FindReferencedIds condition_visitor ;

            VeriStatement *clocked_stmt = event_control_stmt->GetStmt() ;
            while (sens_list->Size() > condition_visitor._referenced_ids.Size()+1) {
                if (!clocked_stmt) break ;
                if (clocked_stmt->GetClassId() == ID_VERISEQBLOCK) {
                    VeriSeqBlock *block = (VeriSeqBlock*)clocked_stmt ;
                    Array *stmts = block->GetStatements() ;
                    VeriStatement *tmp_stmt ;
                    FOREACH_ARRAY_ITEM(stmts, j, tmp_stmt) {
                        if (!tmp_stmt) continue ;
                        if (tmp_stmt->GetClassId() == ID_VERICONDITIONALSTATEMENT) {
                            clocked_stmt = tmp_stmt ;
                            break ;
                        }
                    }
                }

                if (clocked_stmt->GetClassId() == ID_VERICONDITIONALSTATEMENT) {
                    VeriConditionalStatement *if_stmt = (VeriConditionalStatement*)clocked_stmt ;
                    VeriExpression *if_condition = if_stmt->GetIfExpr() ;
                    if_condition->Accept(condition_visitor) ;

                    clocked_stmt = if_stmt->GetElseStmt() ;
                } else {
                    clocked_stmt = 0 ;
                }
            }
            if (!clocked_stmt) {
                continue ;
            }

            VeriIdDef *clock = 0 ;
            FOREACH_ARRAY_ITEM(sens_list, j, event_expr) {
                if (!event_expr) continue ;
                VeriIdDef *sense_id = event_expr->FullId() ;
                if (!sense_id) {
                    continue ;
                }
                if (condition_visitor._referenced_ids.GetItem(sense_id)) {
                    event_control_stmt->Info("   signal %s is an asynchronous set/reset condition of this statement", sense_id->Name()) ;
                } else {
                    event_control_stmt->Info("   signal %s is a clock of this statement", sense_id->Name()) ;
                    if (clock) {
                        event_control_stmt->Error("multiple clocks found on this statement : %s and %s", sense_id->Name(), clock->Name()) ;
                    }
                    clock = sense_id ;
                    Array *ports = module->GetPorts();
                    if (ports) {
                        VeriIdDef *port ;
                        unsigned ii;

                        FOREACH_ARRAY_ITEM(ports, ii, port) {
                            //printf("PORT:  %s\n",port->Name() );
                            if (Strings::compare(port->Name(), clock->Name())) {
                                // printf("PORT:  %s is clock %s\n",port->Name(),clock->Name() );
                                foundClk.port = port->Name();
                                if(topModule == module) {
                                    foundClk.isMasterClk = true;
                                    break;
                                }
                                else {
                                    foundClk.isGenClk = true;
                                    break;
                                }

                            }
                            else
                                foundClk.isGenClk = true;
                        }
                    }
                    foundClk.name = clock->Name();
                    foundClk.module = module;
                    retVector.push_back(foundClk);
                }
            }

            if (!clock) {
                event_control_stmt->Error("cannot find a clock on this statement") ;
            }
        }

    }

    return retVector ;
}







