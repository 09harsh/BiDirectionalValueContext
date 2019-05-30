#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <vector>
#include <utility>
#include <bits/stdc++.h>
#include "lhsRhsPass.cpp"
namespace
{
    class LFCPA : public LhsRhsFinder
    {
    private:
        using contextId = int;
        using forwardEntryValue = std::map<Value*, std::set<Value*>>;
        using forwardExitValue = std::map<Value*, std::set<Value*>>;
        using forwardDataFlowValue = std::map<Value*, std::set<Value*>>;

        using backwardEntryValue = std::set<Value*>;
        using backwardExitValue = std::set<Value*>;
        using backwardDataFlowValue = std::set<Value*>;

        using callSite = BasicBlock*;
        using blockId = BasicBlock*;
        using insId = Instruction*;

        using callers = std::set<std::tuple<Function*, callSite, contextId>>;
        using callee = std::map<std::pair<callSite, insId>, contextId>;

        using lhs = std::pair<Value*, int>;
        using rhs = std::vector<std::pair<Value*, int>>;

        std::map<std::tuple<Function*, forwardEntryValue, backwardEntryValue>, std::tuple<contextId, forwardExitValue, backwardExitValue>> transitionTable;
        std::map<contextId, std::pair<callers, callee>> transitionGraph;
        std::deque<std::tuple<Function*, blockId, contextId>> forwardWorklist, backwardWorklist;
        std::map<Value*, std::set<Value*>> forwardBI, forwardTop, forwardBottom;
        std::set<Value*> backwardBI, backwardTop, backwardBottom;
        std::map<std::tuple<contextId, Function*, insId>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;
        std::map<std::pair<contextId, Function*>, std::pair<forwardEntryValue, backwardEntryValue>> inValues;
        std::map<Function*, std::pair<Instruction*, BasicBlock*>> funcTolastIB;
        std::set<std::tuple<int, Function*, Instruction*>> liveTests;

    public:
        void initContext(Function*, std::map<Value*, std::set<Value*>>, std::set<Value*>);
        void doAnalysisForward();
        void doAnalysisBackward();
        std::map<Value*, std::set<Value*>> merge(std::map<Value*, std::set<Value*>>, std::map<Value*, std::set<Value*>> );
        std::set<Value*> backwardMerge(std::set<Value*>, std::set<Value*>);
        std::set<Value*> backwardNormalFunction(Instruction*, Function*, int);
        std::map<Value*, std::set<Value*>> forwardNormalFlowFunction(Instruction*, Function*, int);
        std::map<Value*, std::set<Value*>> forwardLocalFlowFunction(Instruction*, Function*, int);
        std::set<Value*> backwardLocalFlowFunction(Instruction*, Function*, int);
        void printWorklist(std::deque<std::tuple<Function*, BasicBlock*, int>>);
        void printTransitionGraph();
        void printINandOUT();
        void run(Function*);
        void livenessTestResult();
        std::set<Value*> inInserter(std::set<Value*>, std::vector<std::pair<Value*, int>>, std::map<Value*, std::set<Value*>>);
    };
    LFCPA lfcpaObj;
    static int context = 0;

//-------------------- function to initialise context --------------//
    void LFCPA::initContext(Function *F, std::map<Value*, std::set<Value*>> forwardEntryValue, std::set<Value*> backwardEntryValue)
    {
//        errs() << "initContext for contextID=" << context << " and function=" << F->getName() <<'\n';
        inValues[std::make_pair(context, F)] = std::make_pair(forwardEntryValue, backwardEntryValue);
        //registering into transition table
        transitionTable[std::make_tuple(F, forwardEntryValue, backwardEntryValue)] = std::make_tuple(context, forwardTop, backwardTop);
        //updating transition graph
        transitionGraph[context];

        std::vector<std::tuple<Function*, BasicBlock*, int>> tempWorklist;
        for(Function::iterator bb=F->begin(), e=F->end(); e!=bb;++bb)
        {
            BasicBlock *BB = &(*bb);
            backwardWorklist.push_back(std::make_tuple(F, BB, context));
            tempWorklist.push_back(std::make_tuple(F, BB, context));

            //initializing IN and OUT with 'top' value
            for(BasicBlock::iterator i=bb->begin(), e=bb->end(); i!=e; ++i)
            {
                Instruction* I = &(*i);
                IN[std::make_tuple(context, F, I)] = std::make_pair(forwardTop, backwardTop);
                OUT[std::make_tuple(context, F, I)] = std::make_pair(forwardTop, backwardTop);
            }
        }

        // filling forwardWorklist
        for(int i=tempWorklist.size()-1; i>=0; i--)
        {
            forwardWorklist.push_front(tempWorklist[i]);
        }

        Instruction* start = &(*F->begin()->begin());
        Instruction* end = &(*std::get<1>(tempWorklist[tempWorklist.size()-1])->getTerminator());

        //setting IN and OUT of that function
        IN[std::make_tuple(context, F, start)].first = forwardEntryValue;
        OUT[std::make_tuple(context, F, end)].second = backwardEntryValue;
        funcTolastIB[F].first = end;
        funcTolastIB[F].second = std::get<1>(tempWorklist[tempWorklist.size()-1]);
        tempWorklist.clear();
    }


//-------------------- foward/backward flow analysis ---------------//
    void LFCPA::doAnalysisForward()
    {
//        errs() << "doAnalysisForward start" << '\n';
        while(!forwardWorklist.empty())
        {
            int contextId;
            Function* currentFunction;
            BasicBlock* currentBlock;
            std::tie(currentFunction, currentBlock, contextId) = forwardWorklist.front();
            forwardWorklist.pop_front();

            // an entry node
            if(currentBlock != &(*currentFunction->begin()))
            {
                Instruction* firstInc = &(*currentBlock->begin());
                IN[std::make_tuple(contextId, currentFunction, firstInc)].first = forwardTop;
                for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it)
                {
                    BasicBlock* pred = *it;
                    Instruction* pred_ins = &(*pred->rbegin());

                    std::map<Value*, std::set<Value*>> currentIN, predOUT;
                    currentIN = IN[std::make_tuple(contextId, currentFunction, firstInc)].first;
                    predOUT = OUT[std::make_tuple(contextId, currentFunction, pred_ins)].first;
                    IN[std::make_tuple(contextId, currentFunction, firstInc)].first = merge(currentIN, predOUT);
                }
            }
            for(auto insBB=currentBlock->begin();insBB!=currentBlock->end();insBB++)
            {
                Instruction* currentIns = &(*insBB);
                std::map<Value*, std::set<Value*>> prevOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;
                if(insBB != currentBlock->begin())
                {
                    auto tempIns = insBB;
                    tempIns--;
                    IN[std::make_tuple(contextId, currentFunction, currentIns)].first = OUT[std::make_tuple(contextId, currentFunction, &(*tempIns))].first;
                }
                //function call
    //						errs() << currentIns->getOpcode() << '\n';
                if(currentIns->getOpcode() == 55)
                {
                    int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
                    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
                    if(calledFunction->getName() == "_Z6isLiveIPiEvRT_")
                    {
                        OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                    }
                    else if(calledFunction->getName() == "_Z12isPointingToIPiS0_EvRT_RT0_")
                    {
                        OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                    }
                    else
                    {
                        //check existance of function in transition table
                        std::map<Value*, std::set<Value*>> fIN = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                        std::set<Value*> bOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].second;

                        //does not exist
                        int newContext;
                        if(transitionTable.find(std::make_tuple(calledFunction, fIN, bOUT)) == transitionTable.end())
                        {
                            newContext = ++context;
                            initContext(calledFunction, fIN, bOUT);
                            //setting up the edges of new context
                            transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, contextId));
                            //setting up the edges of calling context
                            transitionGraph[contextId].second[std::make_pair(currentBlock, currentIns)] = newContext;
                            break;
                        }
                        else //exists
                        {
                            newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                            std::map<Value*, std::set<Value*>> outFlow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                            OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = merge(prevOUT, outFlow);
                        }

                        //computing localFlowFunction
                        OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = forwardLocalFlowFunction(currentIns, currentFunction, contextId);

                        //setting up the edges of new context
                        transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, contextId));
                        //setting up the edges of calling context
                        transitionGraph[contextId].second[std::make_pair(currentBlock, currentIns)] = newContext;

                    }
                }
                else
                {
                    OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = forwardNormalFlowFunction(currentIns, currentFunction, contextId);
                }
                Instruction* lastIns = currentBlock->getTerminator();
                std::map<Value*, std::set<Value*>> newOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;

                if(currentIns==lastIns)
                {
                    //if OUT changes
                    if(prevOUT != newOUT)
                    {
//                        errs() << " OUT changes putting succ in forward and backward worklist" << '\n';
                        for(BasicBlock* Succ : successors(currentBlock))
                        {
                            forwardWorklist.push_front(std::make_tuple(currentFunction, Succ, contextId));
                            backwardWorklist.push_back(std::make_tuple(currentFunction, Succ, contextId));
                        }
                    }

                    //if current block is return block
                    if(currentBlock == &(*funcTolastIB[currentFunction].second))
                    {
                        std::map<Value*, std::set<Value*>> fIN = inValues[std::make_pair(contextId, currentFunction)].first;
                        std::set<Value*> bOUT = inValues[std::make_pair(contextId, currentFunction)].second;
                        //setting outflow
                        std::get<1>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newOUT;
//                        errs() << " (forward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                        for(auto callers : transitionGraph[contextId].first)
                        {
                            std::deque<std::tuple<Function*, BasicBlock*, int>> tempWorklist;
                            tempWorklist.push_back(callers);
                            forwardWorklist.push_front(callers);
                            backwardWorklist.push_back(callers);
                            tempWorklist.clear();
                        }
                    }
                }
            }
        }
//        errs() << "doAnalysisForward end" << '\n';
    }

    void LFCPA::doAnalysisBackward()
    {
//        errs() << "doAnalysisBackward strt" << '\n';
        while(!backwardWorklist.empty())
        {
            int currentContext;
            Function* currentFunction;
            BasicBlock* currentBlock;
            std::tie(currentFunction, currentBlock, currentContext) = backwardWorklist.back();
            backwardWorklist.pop_back();

            //if not entry node
            if(currentBlock != &(*funcTolastIB[currentFunction].second))
            {
                Instruction* lastIns = &(*currentBlock->getTerminator());
                OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second = backwardTop;
                for(BasicBlock* Succ : successors(currentBlock))
                {
                    Instruction* succIns = &(*Succ->begin());
                    std::set<Value*> prevOUT, succIN;
                    prevOUT = OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second;
                    succIN = IN[std::make_tuple(currentContext, currentFunction, succIns)].second;
                    OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second = backwardMerge(prevOUT, succIN);
                }
            }
            for(auto insBB=currentBlock->rbegin();insBB!=currentBlock->rend();insBB++)
            {
                Instruction* currentIns = &(*insBB);
                std::set<Value*> prevIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].second;
                if(insBB != currentBlock->rbegin())
                {
                    auto tempIns = insBB;
                    tempIns--;
                    OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second = IN[std::make_tuple(currentContext, currentFunction, &(*tempIns))].second;
                }
                //function call
                if(currentIns->getOpcode() == 55)
                {
                    // errs() <<"-----------------------\nCAll Stmt, printing IN and OUT :\nIN : ";
//					        for(auto in : IN[std::make_tuple(currentContext, calledFunction, currentIns)].second)
//					            errs() << in.first->getName() << ", ";
//					        errs() << "\nOUT : ";
//					        for(auto in : OUT[std::make_tuple(currentContext, calledFunction, currentIns)].second)
//					            errs() << in.first->getName() << ", ";
//					        errs() <<"\n--------------";
                    int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
                    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
                    if(calledFunction->getName() == "_Z6isLiveIPiEvRT_")
                    {
                        liveTests.insert(std::make_tuple(currentContext, currentFunction, currentIns));
                        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;
                    }
                    else if(calledFunction->getName() == "_Z12isPointingToIPiS0_EvRT_RT0_")
                    {
                        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;
                    }
                    else
                    {
                        //check for existance in transiation table
                        std::map<Value*, std::set<Value*>> fIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].first;
                        std::set<Value*> bOUT = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;

                        //does not exist
                        int newContext;
                        if(transitionTable.find(std::make_tuple(calledFunction, fIN, bOUT)) == transitionTable.end())
                        {
                            newContext = ++context;
                            initContext(calledFunction, fIN, bOUT);
                            //setting up the context for new context
                            transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, currentContext));
                            //setting up the edges for calling context
                            transitionGraph[currentContext].second[std::make_pair(currentBlock, currentIns)] = newContext;
                            break;
                        }
                        else
                        {
                            newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                            std::set<Value*> inflow = std::get<2>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
//                                    std::set<Value*> tempAns;
//                                    set_intersection(prevIN.begin(), prevIN.end(), inflow.begin(), inflow.end(), inserter(tempAns, tempAns.begin()));
//                                    IN[std::make_tuple(currentContext, calledFunction, currentIns)].second = inflow;
                            for(auto inflowValues : inflow)
                                IN[std::make_tuple(currentContext, currentFunction, currentIns)].second.insert(inflowValues);

                        }
                        //setting localFlowFunction
                        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = backwardLocalFlowFunction(currentIns, currentFunction, currentContext);

                        //setting up the context for new context
                        transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, currentContext));
                        //setting up the edges for calling context
                        transitionGraph[currentContext].second[std::make_pair(currentBlock, currentIns)] = newContext;

                    }
                }
                else
                {
                    IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = backwardNormalFunction(currentIns, currentFunction, currentContext);
                }

                Instruction* firstIns = &(*currentBlock->begin());
                std::set<Value*> newIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].second;

                if(currentIns == firstIns)
                {
                    //if in changes
                    if(prevIN != newIN)
                    {
//                        errs() << " IN changes putting pred in forward and backward worklist" << '\n';
                        for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it)
                        {
                            forwardWorklist.push_front(std::make_tuple(currentFunction, *it, currentContext));
                            backwardWorklist.push_back(std::make_tuple(currentFunction, *it, currentContext));
                        }
                    }
                    //if current block is exit block
                    if(currentBlock == &(*currentFunction->begin()))
                    {
                        std::map<Value*, std::set<Value*>> fIN = inValues[std::make_pair(currentContext, currentFunction)].first;

                        std::set<Value*> bOUT = inValues[std::make_pair(currentContext, currentFunction)].second;
                        //setting outflow

                        std::get<2>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newIN;
//                        errs() << " (backward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                        for(auto callers : transitionGraph[currentContext].first)
                        {
                            std::deque<std::tuple<Function*, BasicBlock*, int>> tempWorklist;
                            tempWorklist.push_back(callers);
                            forwardWorklist.push_front(callers);
                            backwardWorklist.push_back(callers);
                            tempWorklist.clear();
                        }
                    }
                }
            }
        }

//        errs() << "doAnalysisBackward end" << '\n';
    }


//-------------------- forwardFlow utility functions ----------------//
    std::map<Value*, std::set<Value*>> LFCPA::forwardLocalFlowFunction(Instruction* ins, Function* function, int contextId)
    {
//			    errs() << "forwardNormalFlwFunction called" << '\n';
        /*
            Perform normal flow operation
            Compute GEN, KILL
            and return OUT = (IN - KILL) U GEN;
        */
        return OUT[std::make_tuple(contextId, function, ins)].first;
    }

    std::map<Value*, std::set<Value*>> LFCPA::forwardNormalFlowFunction(Instruction* ins, Function* function, int contextId)
    {
//			    errs() << "forwardNormalFlwFunction called" << '\n';
        /*
            Perform normal flow operation
            Compute GEN, KILL
            and return OUT = (IN - KILL) U GEN;
        */
        return OUT[std::make_tuple(contextId, function, ins)].first;
    }

    std::map<Value*, std::set<Value*>> LFCPA::merge(std::map<Value*, std::set<Value*>> a1, std::map<Value*, std::set<Value*>> a2)
    {
        for(auto it : a2)
        {
            Value* key = a2.first;
            std::set<Value*> value = a2.second;
            for(auto itValues : value)
            {
                a1[key].insert(itValues);
            }
        }

        return a1;
    }


//-------------------- backwardFlow utility functions ---------------//
    std::set<Value*> LFCPA::backwardMerge(std::set<Value*> prevOUT, std::set<Value*> succIN)
    {
        // prevOUT merge succIN
        for(Value* valueInSuccIN : succIN)
            prevOUT.insert(valueInSuccIN);
        return prevOUT;
    }

    std::set<Value*> LFCPA::backwardNormalFunction(Instruction* ins, Function* function, int contextId)
    {
        /*
            Perform normal flow operation
            Compute GEN, KILL
            and return IN = (OUT - KILL) U GEN;
        */
        std::set<Value*> INofInst, OUTofInst;
        OUTofInst = OUT[std::make_tuple(contextId, function, ins)].second;
        std::map<Value*, std::set<Value*>> forwardOUT = OUT[std::make_tuple(contextId, function, ins)].first;

        for(auto outContents : OUTofInst)
            INofInst.insert(outContents);

        //lhs = rhs
        if(isa<StoreInst>(ins))
        {
            std::pair<Value* ,int> LHS = getLHS(ins);
            std::vector<std::pair<Value* ,int>> RHS = getRHS(ins);
            int lhsIndir = LHS.second;
            // x = RHS
            if(lhsIndir==0)
            {
                // if live
                if(OUTofInst.count(LHS.first))
                {
                    INofInst.erase(INofInst.find(LHS.first));
                    INofInst = inInserter(INofInst, RHS, forwardOUT);
                }
            }
            // *x = RHS
            else
            {
                std::queue<Value*> q;
                Value* nullValue = NULL;
                q.push(LHS.first);
                q.push(nullValue);
                while(lhsIndir>0 and !q.empty())
                {
                    Value* lhsTemp = q.front();
                    q.pop();
                    if(lhsTemp == nullValue)
                    {
                        lhsIndir--;
                        q.push(nullValue);
                    }
                    else
                    {
                        INofInst.insert(lhsTemp);
                        for(auto pointees : forwardOUT[lhsTemp])
                            q.push(pointees);
                    }
                }
                bool takeRHS = false;
                while(!q.empty())
                {
                    Value* lhsTemp = q.front();
                    q.pop();
                    //live and must pointer
                    if(lhsTemp!=nullValue and OUTofInst.count(lhsTemp) and forwardOUT[lhsTemp].size()==1)
                    {
                        INofInst.erase(INofInst.find(lhsTemp));
                        takeRHS = true;
                    }
                }
                if(takeRHS)
                {
                    INofInst = inInserter(INofInst, RHS, forwardOUT);
                }
            }
        }
        //use(x)
        else if(isa<ReturnInst>(ins) or isa<ICmpInst>(ins))
        {
            std::vector<std::pair<Value*, int>> use = getUSE(ins);
            INofInst = inInserter(INofInst, use, forwardOUT);
        }
//				ins->print(errs());
//				errs() << " : " << INofInst.size() << " " << OUTofInst.size() << "\n";
        return INofInst;
    }

    std::set<Value*> LFCPA::backwardLocalFlowFunction(Instruction* ins, Function* function, int contextId)
    {
        std::set<Value*> INofInst = IN[std::make_tuple(contextId, function, ins)].second;
        std::vector<std::pair<Value*, int>> argList = getArg(ins);
        std::map<Value*, std::set<Value*>> forwardOUT = OUT[std::make_tuple(contextId, function, ins)].first;
        INofInst = inInserter(INofInst, argList, forwardOUT);
        return INofInst;
    }


//-------------------- starter function --------------------------//
    void LFCPA::run(Function* F)
    {
//        errs() << " main() function start" << "\n";
        context++;
        initContext(F, forwardBI, backwardBI);

        while(!forwardWorklist.empty() or !backwardWorklist.empty())
        {
            doAnalysisBackward();
            doAnalysisForward();
        }
//        printTransitionGraph();
//        printINandOUT();
        livenessTestResult();
//        errs() << " main() function end" << "\n";
    }


//--------------------print functions ---------------------------//
    void LFCPA::printINandOUT()
    {
        for(auto InOutIt : IN)
        {
            int contextId;
            Function* function;
            Instruction* ins;
            errs() << "-----------------------------------------\n";
            std::tie(contextId, function, ins) = InOutIt.first;
            errs() << "( " << contextId << ", "
                   << function->getName() << ", ";
            ins->print(errs());
            errs() << " )\n In : ";
            for(auto inIt : IN[std::make_tuple(contextId, function, ins)].second)
                errs() << inIt->getName() << ", ";
            errs() << "\n OUT : ";
            for(auto inIt : OUT[std::make_tuple(contextId, function, ins)].second)
                errs() << inIt->getName() << ", ";
            errs() << "\n";
        }
    }

    void LFCPA::printTransitionGraph()
    {
        for(auto context : transitionGraph)
        {
            errs() << context.first << " : \n";
            errs() << "Callers : ( ";
            for(auto callers : context.second.first)
            {
                errs() << std::get<2>(callers) << " ";
            }
            errs() << ")\nCallee : ( ";
            for(auto callee : context.second.second)
            {
                errs() << callee.second << " ";
            }
            errs() <<")\n";
        }
    }

    void LFCPA::printWorklist(std::deque<std::tuple<Function*, BasicBlock*, int>> worklist)
    {
        for(auto w : worklist)
        {
            Function* wFunction;
            BasicBlock* wBB;
            int wContext;
            std::tie(wFunction, wBB, wContext) = w;
            if(wBB != NULL)
                errs() << wFunction->getName() << " " << wBB->getName() << " " << wContext << '\n';
            else
                errs() << wFunction->getName() << " " << "NULL" << " " << wContext << '\n';
        }
    }


//-------------------- insertes value of type *x --------------//
    std::set<Value*> LFCPA:: inInserter(std::set<Value*> currentIN, std::vector<std::pair<Value*, int>> list, std::map<Value*, std::set<Value*>> forwardOUT)
    {
        std::set<Value*> INofInst = currentIN;
        for(auto listValues : list)
        {
            int listValuesIndir = listValues.second;
            std::queue<Value*> q;
            Value* nullValue = NULL;
            q.push(listValues.first);
            q.push(nullValue);
            while(listValuesIndir>=0 and !q.empty())
            {
                Value* listValuesTemp = q.front();
                q.pop();
                if(listValuesTemp == nullValue)
                {
                    q.push(nullValue);
                    listValuesIndir--;
                }
                else if(listValuesTemp->getType()->getContainedType(0)->isPointerTy())
                {
                    INofInst.insert(listValuesTemp);
                    for(auto pointees : forwardOUT[listValuesTemp])
                        q.push(pointees);
                }
            }
        }
        return INofInst;
    }


//-------------------- testing functions ---------------------//
    void LFCPA::livenessTestResult()
    {
        errs() <<"\n\t#-------------------- Liveness Test ------------------------#\n\t\n";
        int testNumber = 0;
        bool result;
        for(auto testCases : liveTests)
        {
            int context;
            Function* func;
            Instruction* ins;
            std::tie(context, func, ins) = testCases;
            result = IN[testCases].second.count(ins->getOperand(0));
            errs() <<"\t\ttest#"<<testNumber<<" ( " << func->getName() <<", " << context << ", " <<ins->getOperand(0)->getName() <<" ) : ";
            if(result)
                errs() << "passed ";
            else
                errs() << "failed ";
//            ins->print(errs());
            errs() << "\n";
            testNumber++;
        }
        errs() <<"\t#-----------------------------------------------------------#\n";
    }


//-------------------- functionPass ----------------------------//
	class BValueContext : public FunctionPass
	{
    public:
        static char ID;
        BValueContext() : FunctionPass(ID){}
        virtual bool runOnFunction(Function &F)
        {
            lfcpaObj.metaDataSetter(&F);
            if(F.getName() == "main")
            {
                lfcpaObj.run(&F);
            }
            return false;
        }
	};
}//end namespace

char BValueContext :: ID = 0;
static RegisterPass<BValueContext> X(
	"bvc",		// the option name
	"Bi-Directional Value Context",	// option description
	false,
	false					
);