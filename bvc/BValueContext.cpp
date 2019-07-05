#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include <bits/stdc++.h>
#include <cxxabi.h>
#include "lhsRhsPass.cpp"
namespace
{
    class LFCPA : public LhsRhsFinder
    {
    private:
        using contextId = long long int ;
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

        using lhs = std::pair<Value*, long long int >;
        using rhs = std::vector<std::pair<Value*, long long int >>;

        std::map<std::tuple<Function*, forwardEntryValue, backwardEntryValue>, std::tuple<contextId, forwardExitValue, backwardExitValue>> transitionTable;
        std::map<contextId, std::pair<callers, callee>> transitionGraph;
        std::deque<std::tuple<Function*, blockId, contextId>> forwardWorklist, backwardWorklist;
        std::map<Value*, std::set<Value*>> forwardBI, forwardTop, forwardBottom;
        std::set<Value*> backwardBI, backwardTop, backwardBottom;
        std::map<std::tuple<contextId, Function*, insId>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;
        std::map<std::pair<contextId, Function*>, std::pair<forwardEntryValue, backwardEntryValue>> inValues;
        std::map<Function*, std::pair<Instruction*, BasicBlock*>> funcTolastIB;
        std::set<std::tuple<long long int , Function*, Instruction*>> liveTests;
        std::set<std::tuple<long long int , Function*, Instruction*>> pointsToTests;
        std::map<Value*, std::set<Value*>> totalPointers;
        long long int  before, after;

    public:
        void initContext(Function*, std::map<Value*, std::set<Value*>>, std::set<Value*>);
        void doAnalysisForward();
        void doAnalysisBackward();
        std::map<Value*, std::set<Value*>> merge(std::map<Value*, std::set<Value*>>, std::map<Value*, std::set<Value*>> );
        std::set<Value*> backwardMerge(std::set<Value*>, std::set<Value*>);
        std::set<Value*> backwardNormalFunction(Instruction*, Function*, long long int );
        std::map<Value*, std::set<Value*>> forwardNormalFlowFunction(Instruction*, Function*, long long int );
        std::map<Value*, std::set<Value*>> forwardLocalFlowFunction(Instruction*, Function*, long long int );
        std::set<Value*> backwardLocalFlowFunction(Instruction*, Function*, long long int );
        void printWorklist(std::deque<std::tuple<Function*, BasicBlock*, long long int >>);
        void printTransitionGraph();
        void printTransitionTable();
        void printINandOUT();
        void printForwardINandOUT();
        void run(Function*);
        void livenessTestResult();
        void pointsToTestResult();
        std::set<Value*> inInserter(std::set<Value*>, std::vector<std::pair<Value*, long long int >>, std::map<Value*, std::set<Value*>>);
        std::string demangle(const char*);
        std::string getOriginalName(Function*);
        void removeExtraPointerInfo();
        bool isAcceptable(Instruction*);
        void totalPointsToPairs();
        long long int findTotalPointer();
    };
    LFCPA lfcpaObj;
    static long long int  context = 0;


//-------------------- function to initialise context --------------//
    void LFCPA::initContext(Function *F, std::map<Value*, std::set<Value*>> forwardEntryValue, std::set<Value*> backwardEntryValue)
    {
        inValues[std::make_pair(context, F)] = std::make_pair(forwardEntryValue, backwardEntryValue);
        //registering into transition table
        transitionTable[std::make_tuple(F, forwardEntryValue, backwardEntryValue)] = std::make_tuple(context, forwardTop, backwardTop);
        //updating transition graph
        transitionGraph[context];

        std::vector<std::tuple<Function*, BasicBlock*, long long int >> tempWorklist;
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
        for(long long int  i=tempWorklist.size()-1; i>=0; i--)
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
        while(!forwardWorklist.empty())
        {
            long long int  contextId;
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
                    long long int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
                    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
                    std::string originalName = getOriginalName(calledFunction);
                    if(originalName == "isLive")
                    {
                        OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                    }
                    else if(originalName == "isPointingTo")
                    {
                        pointsToTests.insert(std::make_tuple(contextId, currentFunction, currentIns));
                        OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                    }
                    else
                    {
                        //check existance of function in transition table
                        std::map<Value*, std::set<Value*>> fIN = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                        std::set<Value*> bOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].second;

                        //does not exist
                        long long int  newContext;
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
                        std::map<Value*, std::set<Value*>> oldInflow = std::get<1>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]);
                        if(oldInflow != newOUT)
                        {
                            //setting outflow
                            std::get<1>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newOUT;
    //                        errs() << " (forward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                            for(auto callers : transitionGraph[contextId].first)
                            {
                                forwardWorklist.push_front(callers);
                                backwardWorklist.push_back(callers);
                            }
                        }
                    }
                }
            }
        }
    }

    void LFCPA::doAnalysisBackward()
    {
        while(!backwardWorklist.empty())
        {
            long long int  currentContext;
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
                //function call isa<CallInst>(currentIns)
                if(currentIns->getOpcode() == 55)
                {
                    long long int  numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
                    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
                    std::string originalName = getOriginalName(calledFunction);

                    if(originalName == "isLive")
                    {
                        liveTests.insert(std::make_tuple(currentContext, currentFunction, currentIns));
                        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;
                    }
                    else if(originalName == "isPointingTo")
                    {
                        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;
                    }
                    else
                    {

                        //check for existance in transiation table
                        std::map<Value*, std::set<Value*>> fIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].first;
                        std::set<Value*> bOUT = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;

                        //does not exist
                        long long int  newContext;
                        if(transitionTable.find(std::make_tuple(calledFunction, fIN, bOUT)) == transitionTable.end())
                        {
//                            errs() << currentFunction->getName() << " " << calledFunction->getName() <<" new\n";
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
                            IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = backwardMerge(prevIN, inflow);
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
                        std::set<Value*> oldInflow = std::get<2>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]);

                        if(oldInflow != newIN)
                        {
                            //setting outflow
                            std::get<2>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newIN;
    //                        errs() << " (backward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                            for(auto callers : transitionGraph[currentContext].first)
                            {
                                forwardWorklist.push_front(callers);
                                backwardWorklist.push_back(callers);
                            }
                        }
                    }
                }
            }
        }
    }


//-------------------- forwardFlow utility functions ----------------//
    std::map<Value*, std::set<Value*>> LFCPA::forwardLocalFlowFunction(Instruction* ins, Function* function, long long int  contextId)
    {
//			    errs() << "forwardNormalFlwFunction called" << '\n';
        /*
            Perform normal flow operation
            Compute GEN, KILL
            and return OUT = (IN - KILL) U GEN;
        */
        return OUT[std::make_tuple(contextId, function, ins)].first;
    }

    std::map<Value*, std::set<Value*>> LFCPA::forwardNormalFlowFunction(Instruction* ins, Function* function, long long int  contextId)
    {
//			    errs() << "forwardNormalFlwFunction called" << '\n';
        /*
            Perform normal flow operation
            Compute GEN, KILL
            and return OUT = (IN - KILL) U GEN;
        */
        std::map<Value*, std::set<Value*>> INofInst, OUTofInst;
        INofInst = IN[std::make_tuple(contextId, function, ins)].first;
        OUTofInst = INofInst;
        std::set<Value*> backwardOUT = OUT[std::make_tuple(contextId, function, ins)].second;
        if(isa<StoreInst>(ins) and getRHS(ins).size()==1)
        {
            std::pair<Value*, long long int > LHS = getLHS(ins);
            std::pair<Value*, long long int > RHS = getRHS(ins)[0];
            std::set<Value*> rhsSet;
            //getting rhs part
            long long int  rhsIndir = RHS.second;
            Value* rhsValue = RHS.first;
            if(rhsIndir == -1)
            {
                rhsSet.insert(rhsValue);
            }
            else
            {
                std::queue<Value*> q;
                Value* nullValue = NULL;
                q.push(rhsValue);
                q.push(nullValue);
                while(rhsIndir>=0 and !q.empty())
                {
                    Value* rhsTemp = q.front();
                    q.pop();
                    if(rhsTemp == nullValue)
                    {
                        q.push(nullValue);
                        rhsIndir--;
                    }
                    else if(rhsTemp->getType()->getContainedType(0)->isPointerTy())
                    {
                        for(auto pointees : INofInst[rhsTemp])
                            q.push(pointees);
                    }
                }
                while(!q.empty())
                {
                    Value* rhsValue = q.front();
                    q.pop();
                    if(rhsValue != nullValue)
                        rhsSet.insert(rhsValue);
                }
            }
            //setting lhs
            long long int  lhsIndir = LHS.second;
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
                    q.push(nullValue);
                    lhsIndir--;
                }
                else if(lhsTemp->getType()->getContainedType(0)->isPointerTy())
                {
                    for(auto pointees : INofInst[lhsTemp])
                        q.push(pointees);
                }
            }
            while(!q.empty())
            {
                Value* lhsValue = q.front();
                q.pop();
                if(lhsValue!=nullValue )
                {
                    OUTofInst[lhsValue] = rhsSet;
                }
            }
        }
        return OUTofInst;
    }

    std::map<Value*, std::set<Value*>> LFCPA::merge(std::map<Value*, std::set<Value*>> a1, std::map<Value*, std::set<Value*>> a2)
    {
        for(auto it : a2)
        {
            Value* key = it.first;
            std::set<Value*> value = it.second;
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

    std::set<Value*> LFCPA::backwardNormalFunction(Instruction* ins, Function* function, long long int contextId)
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
            std::pair<Value* ,long long int > LHS = getLHS(ins);
            std::vector<std::pair<Value* ,long long int >> RHS = getRHS(ins);
            long long int  lhsIndir = LHS.second;
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
            std::vector<std::pair<Value*, long long int >> use = getUSE(ins);
            INofInst = inInserter(INofInst, use, forwardOUT);
        }
//				ins->print(errs());
//				errs() << " : " << INofInst.size() << " " << OUTofInst.size() << "\n";
        return INofInst;
    }

    std::set<Value*> LFCPA::backwardLocalFlowFunction(Instruction* ins, Function* function, long long int  contextId)
    {
        std::set<Value*> INofInst = IN[std::make_tuple(contextId, function, ins)].second;
        std::vector<std::pair<Value*, long long int >> argList = getArg(ins);
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
        before = after = 0;

        totalPointers.clear();
        before = findTotalPointer();

        removeExtraPointerInfo();

        totalPointers.clear();
        after = findTotalPointer();

        // printTransitionGraph();
        pointsToTestResult();
        livenessTestResult();
        totalPointsToPairs();
        
    }


//--------------------prlong long int functions ---------------------------//
    void LFCPA::printINandOUT()
    {
        for(auto InOutIt : IN)
        {
            long long int  contextId;
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

    void LFCPA::printForwardINandOUT()
    {
        for(auto InOutIt : IN)
        {
            long long int  contextId;
            Function* function;
            Instruction* ins;
            errs() << "-----------------------------------------\n";
            std::tie(contextId, function, ins) = InOutIt.first;
            errs() << "( " << contextId << ", "
                   << function->getName() << ", ";
            ins->print(errs());
            errs() << " )\n In : ";
            for(auto inIt : IN[std::make_tuple(contextId, function, ins)].first)
            {
                errs() <<"{ ";
                errs() << inIt.first->getName() << "->(";
                for(auto it : inIt.second)
                    errs() << it->getName() << ", ";
                errs() <<" }";
            }
            errs() << "\n OUT : ";
            for(auto inIt : OUT[std::make_tuple(contextId, function, ins)].first)
            {
                errs() <<"{ ";
                errs() << inIt.first->getName() << "->(";
                for(auto it : inIt.second)
                    errs() << it->getName() << ", ";
                errs() <<" }";
            }
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

    void LFCPA::printWorklist(std::deque<std::tuple<Function*, BasicBlock*, long long int >> worklist)
    {
        for(auto w : worklist)
        {
            Function* wFunction;
            BasicBlock* wBB;
            long long int  wContext;
            std::tie(wFunction, wBB, wContext) = w;
            if(wBB != NULL)
                errs() << wFunction->getName() << " " << wBB->getName() << " " << wContext << '\n';
            else
                errs() << wFunction->getName() << " " << "NULL" << " " << wContext << '\n';
        }
    }

    void LFCPA::printTransitionTable()
    {
        errs() << "\n\t#----------------- Transition Table -----------------#\n";
        for(auto it : transitionTable)
        {
            Function* func;
            std::map<Value*, std::set<Value*>> fEntry, fExit;
            std::set<Value*> bEntry, bExit;
            long long int  context;
            std::tie(func, fEntry, bEntry) = it.first;
            std::tie(context, fExit, bExit) = it.second;
            errs() << "\t\tfunctionName\t: " << getOriginalName(func) << "\n";
            errs() << "\t\tcontext\t: " << context << "\n";
            errs() << "\t\tbackwardEntry\t: ";
            for(auto bEntryValues : bEntry)
                errs() << bEntryValues->getName() << ", ";
            errs() << "\n";
            errs() << "\t\tbackwardExit\t: ";
            for(auto bEntryValues : bExit)
                errs() << bEntryValues->getName() << ", ";
            errs() << "\n\t\tforwardEntry\t: ";
            for(auto fEntryValue : fEntry)
            {
                Value* key = fEntryValue.first;
                std::set<Value*> value = fEntryValue.second;
                errs() << key->getName() << "-> (";
                for(auto itValue : value)
                    errs() << itValue->getName() << ", ";
                errs() << "), ";
            }
            errs() << "\n\t\tforwardExit\t: ";
            for(auto fEntryValue : fExit)
            {
                Value* key = fEntryValue.first;
                std::set<Value*> value = fEntryValue.second;
                errs() << key->getName() << "-> (";
                for(auto itValue : value)
                    errs() << itValue->getName() << ", ";
                errs() << "), ";
            }
            errs() << "\n";
            errs() << "\t*************************************************\n";

        }
        errs() << "\t#-----------------------------------------------#\n";
    }


//-------------------- other utility functions --------------//
    std::set<Value*> LFCPA:: inInserter(std::set<Value*> currentIN, std::vector<std::pair<Value*, long long int >> list, std::map<Value*, std::set<Value*>> forwardOUT)
    {
        std::set<Value*> INofInst = currentIN;
        for(auto listValues : list)
        {
            long long int  listValuesIndir = listValues.second;
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

    inline std::string LFCPA::demangle(const char* name)
    {
        int status = -1;

        std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
        return (status == 0) ? res.get() : std::string(name);
    }

    std::string LFCPA::getOriginalName(Function* calledFunction)
    {
        std::string s1 = demangle(calledFunction->getName().str().c_str());
        size_t found = s1.find('<');
        size_t found1 = s1.find(' ');
        if(found!=std::string::npos and found1!=std::string::npos)
            s1 = std::string(s1.begin()+found1+1, s1.begin()+found);
        return s1;
    }

    bool LFCPA::isAcceptable(Instruction* ins)
    {
        bool result = false;
        if(isa<CallInst>(ins))
            result = true;
        if(lhsRhsMap.count(ins))
            result = true;
        if(useMap.count(ins))
            result = true;
        return result;
    }

    void LFCPA::removeExtraPointerInfo()
    {
        std::vector<std::tuple<long long int , Function*, Instruction*>> deleter;
        for(auto it : IN)
        {
            Instruction* ins = std::get<2>(it.first);
            if(isAcceptable(ins))
            {
                std::set<Value*> bIN, bOUT;
                std::map<Value*, std::set<Value*>> fIN, fOUT;
                fIN = it.second.first;
                fOUT = OUT[it.first].first;
                bIN = it.second.second;
                bOUT = OUT[it.first].second;
                //setting IN
                for(auto fINIt : fIN)
                {
                    Value* key = fINIt.first;
                    if(bIN.count(key) == 0)
                        IN[it.first].first.erase(key);
                }
    //            //setting OUT
                for(auto fOUTIt : fOUT)
                {
                    Value* key = fOUTIt.first;
                    if(bOUT.count(key) == 0)
                        OUT[it.first].first.erase(key);
                }
            }
            else
            {
                deleter.push_back(it.first);
            }
        }
        for(auto deleteKey : deleter)
        {
            IN.erase(deleteKey);
            OUT.erase(deleteKey);
        }
    }

//-------------------- testing functions ---------------------//
    void LFCPA::livenessTestResult()
    {
        if(liveTests.size() > 0)
        {
            errs() <<"\n\t#-------------------- Liveness Test ------------------------#\n\t\n";
            long long int testNumber = 0;
            bool result;
            for(auto testCases : liveTests)
            {
                long long int context;
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
    }

    void LFCPA::pointsToTestResult()
    {
        if(pointsToTests.size() > 0)
        {
            errs() <<"\n\t#-------------------- PointsTo Test ------------------------#\n\t\n";
            long long int testNumber = 0;
            bool result;
            for(auto testCases : pointsToTests)
            {
                long long int context;
                Function* func;
                Instruction* ins;
                std::tie(context, func, ins) = testCases;
                result = OUT[testCases].first[ins->getOperand(0)].count(ins->getOperand(1));
                errs() <<"\t\ttest#"<<testNumber<<" ( " << func->getName() <<", " << context << ", " <<ins->getOperand(0)->getName() <<"->" << ins->getOperand(1)->getName() <<" ) : ";
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
    }


    long long int LFCPA::findTotalPointer()
    {
        long long ans=0;
        for(auto it : IN)
        {
            std::map<Value*, std::set<Value*>> pointsTopairs = it.second.first;
            for(auto mapIt : pointsTopairs)
            {
                Value* key = mapIt.first;
                std::set<Value*> value = mapIt.second;
//                errs() << key->getName() << " " << value.size() << "\n";
                totalPointers[key] = value;
            }
        }
        for(auto it : totalPointers)
        {
            ans = ans + it.second.size();
        }
        return ans;
    }

    void LFCPA::totalPointsToPairs()
    {
        errs() <<"\n\t#-------- Total Number Of PointsTo Pairs---------------#\n\n";
        errs() << "\t\tBefore\t:\t" << before;
        errs() << "\n\t\tAfter\t:\t" << after;
        errs() <<"\n\t#-----------------------------------------------------------#\n";
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
//            lfcpaObj.setPredSucc(&F);
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