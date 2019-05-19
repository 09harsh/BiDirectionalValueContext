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
using namespace llvm;

namespace
{
	using contextId = int;
	using forwardEntryValue = std::vector<bool>;
	using backwardEntryValue = std::vector<bool>;
	using forwardExitValue = std::vector<bool>;
	using backwardExitValue = std::vector<bool>;
	using forwardDataFlowValue = std::vector<bool>;
	using backwardDataFlowValue = std::vector<bool>;
	using callSite = BasicBlock*;
	using callers = std::set<std::tuple<Function*, callSite, contextId>>;
	using blockId = BasicBlock*;
	using insId = Instruction*;
	using callee = std::map<std::pair<callSite, insId>, contextId>;
	
	//declaring globals
	
	static int context = 0;
	std::map<std::tuple<Function*, forwardEntryValue, backwardEntryValue>, std::tuple<contextId, forwardExitValue, backwardExitValue>> transitionTable;
	std::map<contextId, std::pair<callers, callee>> transitionGraph;
	std::deque<std::tuple<Function*, blockId, contextId>> forwardWorklist, backwardWorklist;
	std::vector<bool> forwardBI, backwardBI, forwardTop, forwardBottom, backwardTop, backwardBottom;
	std::map<std::tuple<contextId, Function*, insId>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;
	std::map<std::pair<contextId, Function*>, std::pair<forwardEntryValue, backwardEntryValue>> inValues;
	std::map<Function*, std::pair<Instruction*, BasicBlock*>> funcTolastIB;

	//functionPass
	class BValueContext : public FunctionPass
	{
		public:
			static char ID;
			BValueContext() : FunctionPass(ID){}
			virtual bool runOnFunction(Function &F)
			{
				if(F.getName() == "main")
				{
				    errs() << " main() function start" << "\n";
				    context++;
					initContext(&F, forwardBI, backwardBI);
					
					while(!forwardWorklist.empty() or !backwardWorklist.empty())
					{
						doAnalysisBackward();
						doAnalysisForward();
					}
					errs() << " main() function end" << "\n";
				}
				return false;
			}

			//function to initialize a new context
			void initContext(Function *F, std::vector<bool> forwardEntryValue, std::vector<bool> backwardEntryValue)
			{
			    /*
			        dir tells either forward initialization or backward
			        1 => forward
			        0 => backward
			    */
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


			//procedure to do forward analysis
			void doAnalysisForward()
			{
			    errs() << "doAnalysisForward start" << '\n';
				while(!forwardWorklist.empty())
				{
					int contextId;
					Function* currentFunction;
					BasicBlock* currentBlock;
					std::tie(currentFunction, currentBlock, contextId) = forwardWorklist.front();
					forwardWorklist.pop_front();

					// an entry node 
					if(currentBlock == &(*currentFunction->begin()))
					{
						std::tie(currentFunction, currentBlock, contextId) = forwardWorklist.front();
						forwardWorklist.pop_front();
					}
					else
					{
						Instruction* firstInc = &(*currentBlock->begin());
						IN[std::make_tuple(contextId, currentFunction, firstInc)].first = forwardTop;
						for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it)
						{
							BasicBlock* pred = *it;
							Instruction* pred_ins = &(*pred->rbegin());

							std::vector<bool> a1,a2;
							a1 = IN[std::make_tuple(contextId, currentFunction, firstInc)].first;
							a2 = OUT[std::make_tuple(contextId, currentFunction, pred_ins)].first;
							IN[std::make_tuple(contextId, currentFunction, firstInc)].first = merge(a1,a2);
						}
					}
					for(auto insBB=currentBlock->begin();insBB!=currentBlock->end();insBB++)
					{
						Instruction* currentIns = &(*insBB);
						std::vector<bool> prevOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;
						if(insBB != currentBlock->begin())
						{
							auto tempIns = insBB;
							tempIns--;
							IN[std::make_tuple(contextId, currentFunction, currentIns)].first = OUT[std::make_tuple(contextId, currentFunction, &(*tempIns))].first;
						}
						//function call
						if(currentIns->getOpcode() == 55)
						{
						    int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
						    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
						    if(calledFunction->getName() == "_Z5checkv")
						    {
						        performChecking(currentFunction->getName());  //checker code
						    }
						    else
						    {
                                //check existance of function in transition table
                                std::vector<bool> fIN = IN[std::make_tuple(contextId, currentFunction, currentIns)].first;
                                std::vector<bool> bOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].second;

                                //does not exist
                                int newContext;
                                if(transitionTable.find(std::make_tuple(calledFunction, fIN, bOUT)) == transitionTable.end())
                                {
                                    newContext = ++context;
                                    initContext(calledFunction, fIN, bOUT);
                                }
                                else //exists
                                {
                                    newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                                    std::vector<bool> outFlow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
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
						std::vector<bool> newOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;

						if(currentIns==lastIns)
						{
						    //if OUT changes
						    if(prevOUT != newOUT)
						    {
                                errs() << " OUT changes putting succ in forward and backward worklist" << '\n';
                                for(BasicBlock* Succ : successors(currentBlock))
                                {
                                    forwardWorklist.push_front(std::make_tuple(currentFunction, Succ, contextId));
                                    backwardWorklist.push_back(std::make_tuple(currentFunction, Succ, contextId));
                                }
						    }

						    //if current block is return block
                            if(currentBlock == &(*funcTolastIB[currentFunction].second))
                            {
                                std::vector<bool> fIN = inValues[std::make_pair(contextId, currentFunction)].first;
                                std::vector<bool> bOUT = inValues[std::make_pair(contextId, currentFunction)].second;
                                //setting outflow
                                std::get<1>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newOUT;
                                errs() << " (forward) Leaving function " << currentFunction->getName() << " putting its callers in forward and backward worklist" << '\n';
                                for(auto callers : transitionGraph[contextId].first)
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
				errs() << "doAnalysisForward end" << '\n';
			}

			void doAnalysisBackward()
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
					if(currentBlock == &(*funcTolastIB[currentFunction].second))
					{
					    std::tie(currentFunction, currentBlock, currentContext) = backwardWorklist.back();
					    backwardWorklist.pop_back();
					}
					else
					{
					    Instruction* lastIns = &(*currentBlock->getTerminator());
					    OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second = backwardTop;
					    for(BasicBlock* Succ : successors(currentBlock))
					    {
					        Instruction* succIns = &(*Succ->begin());
					        std::vector<bool> prevOUT, succIN;
					        prevOUT = OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second;
					        succIN = IN[std::make_tuple(currentContext, currentFunction, succIns)].second;
					        OUT[std::make_tuple(currentContext, currentFunction, lastIns)].second = backwardMerge(prevOUT, succIN);
					    }
					}
					for(auto insBB=currentBlock->rbegin();insBB!=currentBlock->rend();insBB++)
					{
					    Instruction* currentIns = &(*insBB);
					    std::vector<bool> prevIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].second;
					    if(insBB != currentBlock->rbegin())
					    {
					        auto tempIns = insBB;
					        tempIns--;
					        OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second = IN[std::make_tuple(currentContext, currentFunction, &(*tempIns))].second;
					    }
					    //function call
					    if(currentIns->getOpcode() == 55)
					    {
					        int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
						    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
						    if(calledFunction->getName() == "_Z5checkv")
						    {
						        performChecking(currentFunction->getName());  //checker code
						    }
						    else
						    {
						        //check for existance in transiation table
                                std::vector<bool> fIN = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].first;
                                std::vector<bool> bOUT = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;

                                //does not exist
                                int newContext;
                                if(transitionTable.find(std::make_tuple(calledFunction, fIN, bOUT)) == transitionTable.end())
                                {
                                    newContext = ++context;
                                    initContext(calledFunction, fIN, bOUT);
                                }
                                else
                                {
                                    newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                                    std::vector<bool> inflow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fIN, bOUT)]);
                                    IN[std::make_tuple(currentContext, calledFunction, currentIns)].second = backwardMerge(prevIN, inflow);
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
					    std::vector<bool> newIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].second;

					    if(currentIns == firstIns)
					    {
					        //if in changes
					        if(prevIN != newIN)
                            {
                                errs() << " IN changes putting pred in forward and backward worklist" << '\n';
                                for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it)
                                {
                                    forwardWorklist.push_front(std::make_tuple(currentFunction, *it, currentContext));
                                    backwardWorklist.push_back(std::make_tuple(currentFunction, *it, currentContext));
                                }
					        }
					        //if current block is exit block
					        if(currentBlock == &(*currentFunction->begin()))
					        {
                                std::vector<bool> fIN = inValues[std::make_pair(currentContext, currentFunction)].first;
                                std::vector<bool> bOUT = inValues[std::make_pair(currentContext, currentFunction)].second;
                                //setting outflow
                                std::get<2>(transitionTable[std::make_tuple(currentFunction, fIN, bOUT)]) = newIN;
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

			std::vector<bool> merge(std::vector<bool> a1, std::vector<bool> a2)
			{
				// a1 merge a2
				return a1;
			}

            std::vector<bool> backwardMerge(std::vector<bool> prevOUT, std::vector<bool> succIN)
			{
				// prevOUT merge succIN
				return prevOUT;
			}

            std::vector<bool> backwardNormalFunction(Instruction* ins, Function* function, int contextId)
            {
                /*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
				return IN[std::make_tuple(contextId, function, ins)].second;
            }

			std::vector<bool> forwardNormalFlowFunction(Instruction* ins, Function* function, int contextId)
			{
//			    errs() << "forwardNormalFlwFunction called" << '\n';
				/*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
				return OUT[std::make_tuple(contextId, function, ins)].first;
			}


			std::vector<bool> forwardLocalFlowFunction(Instruction* ins, Function* function, int contextId)
			{
//			    errs() << "forwardNormalFlwFunction called" << '\n';
				/*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
				return OUT[std::make_tuple(contextId, function, ins)].first;
			}

			std::vector<bool> backwardLocalFlowFunction(Instruction* ins, Function* function, int contextId)
			{
			    //errs() << "forwardNormalFlwFunction called" << '\n';
				/*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
			    return IN[std::make_tuple(contextId, function, ins)].second;
			}

			void performChecking(std::string fName)
			{
			    errs() << "\nCheck under progress for function = " << fName << '\n';
			    errs() << "***********************************\n";
			    errs() << " \nPrinting backwardWorklist : " << '\n';
			    printWorklist(backwardWorklist);
			    errs () << "Size of backwardWorklist = " << backwardWorklist.size() << '\n';
			    errs() << " \nPrinting forwardWorklist : " << '\n';
			    printWorklist(forwardWorklist);
			    errs () << "Size of forwardWorklist = " << forwardWorklist.size() << '\n';
			    printTransitionGraph();
			    errs() << "***********************************\n";
			}

            void printWorklist(std::deque<std::tuple<Function*, BasicBlock*, int>> worklist)
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

            void printTransitionGraph()
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

            void printINandOUT()
            {

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