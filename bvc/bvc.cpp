static int context = 0;
class BiDirValueContext
{
protected:
    using forwardEntryValue = void*;
    using backwardEntryValue = void*;
    using forwardExitValue = void*;
    using backwardExitValue = void*;
    using contextId = int;
    std::map<std::tuple<Function*, forwardEntryValue, backwardEntryValue>, std::tuple<contextId, forwardExitValue, backwardExitValue>> transitionTable;

    using callers = std::set<std::tuple<Function*, BasicBlock*, contextId>>;
    using callee = std::map<std::pair<BasicBlock*, Instruction*>, contextId>;
    std::map<contextId, std::pair<callers, callee>> transitionGraph;

    std::deque<std::tuple<Function*, BasicBlock*, contextId>> forwardWorklist, backwardWorklist;

    void *forwardBI, *forwardTop, *forwardBottom, *backwardBI, *backwardTop, *backwardBottom;

    using forwardDataFlowValue = void*;
    using backwardDataFlowValue = void*;
    std::map<std::tuple<contextId, Function*, Instruction*>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;

    std::map<std::pair<contextId, Function*>, std::pair<forwardEntryValue, backwardEntryValue>> inValues;
    std::map<Function*, std::pair<Instruction*, BasicBlock*>> funcTolastIB;

public:
    void initContext(Function*, forwardEntryValue, backwardEntryValue);
    void doAnalysisBackward();
};


void BiDirValueContext::doAnalysisBackward()
{
    errs() << "doAnalysisBackward strt" << '\n';
    while(!backwardWorklist.empty())
    {
        int currentContext;
        Function* currentFunction;
        BasicBlock* currentBlock;
        std::tie(currentFunction, currentBlock, currentContext) = backwardWorklist.back();
        backwardWorklist.pop_back();

        //if entry node
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
                if(calledFunction->getName() == "_Z5checkv")
                {
                    performChecking(currentFunction->getName());  //checker code
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
                        std::set<Value*> tempAns;
                        set_intersection(prevIN.begin(), prevIN.end(), inflow.begin(), inflow.end(), inserter(tempAns, tempAns.begin()));
//                                    IN[std::make_tuple(currentContext, calledFunction, currentIns)].second = inflow;
                        for(auto inflowValues : inflow)
                            IN[std::make_tuple(currentContext, currentFunction, currentIns)].second.insert(inflowValues);
                        errs() << "--------------\nprinting inflow : ";
                        for(auto finValues : inflow)
                        {
                            errs() << finValues->getName() << " ";
                        }
                        errs() << "\n";
                        errs() << "--------------\ncalled func : ";
                        for(auto finValues : IN[std::make_tuple(currentContext, calledFunction, currentIns)].second)
                        {
                            errs() << finValues->getName() << " ";
                        }
                        errs() << "\n" << calledFunction->getName() <<" ";
                        currentIns->print(errs());
                        errs() << "\n";
                    }
                    //setting localFlowFunction
                    IN[std::make_tuple(currentContext, calledFunction, currentIns)].second = backwardLocalFlowFunction(currentIns, currentFunction, currentContext);

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
                    errs() << " IN hanges putting pred in forward and backward worklist" << '\n';
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
                    errs() << "--------------\nprinting outflow : ";
                    for(auto finValues : std::get<2>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]))
                    {
                        errs() << finValues->getName() << " ";
                    }
                    errs() << "\n";
                    errs() << " (backward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                    for(auto callers : transitionGraph[currentContext].first)
                    {
                        std::deque<std::tuple<Function*, BasicBlock*, int>> tempWorklist;
                        tempWorklist.push_back(callers);
                        forwardWorklist.push_front(callers);
                        backwardWorklist.push_back(callers);
                        printWorklist(tempWorklist);
                        tempWorklist.clear();
                    }
                }
            }
        }
    }

    errs() << "doAnalysisBackward end" << '\n';
}

// function to initialize contexts
void BiDirValueContext::initContext(Function*, void* forwardEntryValue, void* backwardEntryValue)
{
    errs() << "initContext for contextID=" << context << " and function=" << F->getName() <<'\n';
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