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
	std::deque<std::tuple<Function*, blockId, contextId, bool>> forwardWorklist, backwardWorklist;
	std::vector<bool> forwardBI, backwardBI, forwardTop, forwardBottom, backwardTop, backwardBottom;
	std::map<std::tuple<contextId, Function*, insId>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;
	std::map<std::pair<contextId, Function*>, std::pair<forwardEntryValue, backwardEntryValue>> inValues;
	std::map<Function*, Instruction*> funcTolastIns;

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
					initContext(&F, forwardBI, backwardBI, 1);
					
					while(!forwardWorklist.empty() or !backwardWorklist.empty())
					{
						doAnalysisForward();
						doAnalysisBackward();
					}
					errs() << " main() function end" << "\n";
				}
				return false;
			}

			//function to initialize a new context
			void initContext(Function *F, std::vector<bool> forwardEntryValue, std::vector<bool> backwardEntryValue, bool dir)
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

				std::vector<std::tuple<Function*, BasicBlock*, int, bool>> tempWorklist;
				BasicBlock* nullBlock = NULL;
				/*
				    bool 1=> entry
				    bool 0 => exit
				*/
				backwardWorklist.push_back(std::make_tuple(F, nullBlock, context, 0));
				for(Function::iterator bb=F->begin(), e=F->end(); e!=bb;++bb)
				{
					BasicBlock *BB = &(*bb);
					backwardWorklist.push_back(std::make_tuple(F, BB, context, 1));
					tempWorklist.push_back(std::make_tuple(F, BB, context, 1));
					
					//initializing IN and OUT with 'top' value
					for(BasicBlock::iterator i=bb->begin(), e=bb->end(); i!=e; ++i)
					{
						Instruction* I = &(*i);
						IN[std::make_tuple(context, F, I)] = std::make_pair(forwardTop, backwardTop);
						OUT[std::make_tuple(context, F, I)] = std::make_pair(forwardTop, backwardTop);
					}
				}
				backwardWorklist.push_back(std::make_tuple(F, nullBlock, context, 1));

				// filling forwardWorklist
				forwardWorklist.push_front(std::make_tuple(F, nullBlock, context, 0));
				for(int i=tempWorklist.size()-1; i>=0; i--)
				{
					forwardWorklist.push_front(tempWorklist[i]);
				}
				forwardWorklist.push_front(std::make_tuple(F, nullBlock, context,1));

				Instruction* start = &(*F->begin()->begin());
				Instruction* end = &(*std::get<1>(tempWorklist[tempWorklist.size()-1])->rbegin());
				if(dir)
				    IN[std::make_tuple(context, F, start)].first = forwardEntryValue;
				else
				    OUT[std::make_tuple(context, F, end)].second = backwardEntryValue;
				funcTolastIns[F] = end;
				errs() << "\nPrinting forward" << '\n';
				printWorklist(forwardWorklist);
				errs() << "\nPrinting backward" << '\n';
				printWorklist(backwardWorklist);
                errs() << "initContext end" << '\n';
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
					bool entry;
					std::tie(currentFunction, currentBlock, contextId, entry) = forwardWorklist.front();
					forwardWorklist.pop_front();

					// an entry node 
					if(currentBlock==NULL and entry==true)
					{
						std::tie(currentFunction, currentBlock, contextId, entry) = forwardWorklist.front();
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
                                std::vector<bool> bIN = IN[std::make_tuple(contextId, currentFunction, currentIns)].second;

                                //does not exist
                                int newContext;
                                if(transitionTable.find(std::make_tuple(calledFunction, fIN, bIN)) == transitionTable.end())
                                {
                                    newContext = ++context;
                                    initContext(calledFunction, fIN, bIN, 1);
                                }
                                else //exists
                                {
                                    newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bIN)]);
                                    std::vector<bool> outFlow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fIN, bIN)]);
                                    OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = merge(prevOUT, outFlow);
                                }


                                //setting up the edges of new context
                                transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, contextId));

                                //setting up the edges of calling context
                                transitionGraph[contextId].second[std::make_pair(currentBlock, currentIns)] = newContext;
                                printTransitionGraph();
						    }
						}
						else
						{
							OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = forwardNormalFlowFunction(currentIns, currentFunction, contextId);
						}
						Instruction* lastIns = currentBlock->getTerminator();
						std::vector<bool> newOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;

						//if OUT changes
						if(currentIns==lastIns and prevOUT != newOUT)
						{
						    errs() << " OUT changes putting succ in forward and backward worklist" << '\n';
						    for(BasicBlock* Succ : successors(currentBlock))
						    {
						        forwardWorklist.push_front(std::make_tuple(currentFunction, Succ, contextId, 1));
						        backwardWorklist.push_back(std::make_tuple(currentFunction, Succ, contextId, 1));
						    }
						}
					}
					//if next block is return block
					while(!forwardWorklist.empty() and std::get<1>(forwardWorklist.front())==NULL and std::get<3>(forwardWorklist.front())==false)
					{
					    Function* function = std::get<0>(forwardWorklist.front());
					    int funcContext = std::get<2>(forwardWorklist.front());
					    forwardWorklist.pop_front();
					    std::vector<bool> fIN = inValues[std::make_pair(funcContext, function)].first;
					    std::vector<bool> bIN = inValues[std::make_pair(funcContext, function)].second;
					    Instruction* lastIns = funcTolastIns[function];
					    //setting outflow
					    std::get<1>(transitionTable[std::make_tuple(function,fIN, bIN)]) = OUT[std::make_tuple(funcContext, function, lastIns)].first;
					    std::get<2>(transitionTable[std::make_tuple(function,fIN, bIN)]) = OUT[std::make_tuple(funcContext, function, lastIns)].second;
					    errs() << " (forward) Leaving function " << function->getName() << " putting its callers in forward and backward worklist" << '\n';
					    for(auto callers : transitionGraph[funcContext].first)
					    {
					        Function* function;
					        BasicBlock* bb;
					        int context;
					        std::tie(function, bb, context) = callers;
                            std::deque<std::tuple<Function*, BasicBlock*, int, bool>> tempWorklist;
                            tempWorklist.push_back(std::make_tuple(function, bb, context, 1));
					        forwardWorklist.push_front(std::make_tuple(function, bb, context, 1));
					        backwardWorklist.push_back(std::make_tuple(function, bb, context, 1));
					        printWorklist(tempWorklist);
					        tempWorklist.clear();
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
			        bool entry;
			        std::tie(currentFunction, currentBlock, currentContext, entry) = backwardWorklist.back();
					backwardWorklist.pop_back();

					//if entry node
					if(currentBlock == NULL)
					{
					    std::tie(currentFunction, currentBlock, currentContext, entry) = backwardWorklist.back();
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
                                std::vector<bool> fOUT = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].first;
                                std::vector<bool> bOUT = OUT[std::make_tuple(currentContext, currentFunction, currentIns)].second;

                                //does not exist
                                int newContext;
                                if(transitionTable.find(std::make_tuple(calledFunction, fOUT, bOUT)) == transitionTable.end())
                                {
                                    newContext = ++context;
                                    initContext(calledFunction, fOUT, bOUT, 0);
                                }
                                else
                                {
                                    newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fOUT, bOUT)]);
                                    std::vector<bool> inflow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fOUT, bOUT)]);
                                    IN[std::make_tuple(currentContext, calledFunction, currentIns)].second = backwardMerge(prevIN, inflow);
                                }
                                //setting up the context for new context
                                transitionGraph[newContext].first.insert(std::make_tuple(currentFunction, currentBlock, currentContext));
                                //setting up the edges for calling context
                                transitionGraph[currentContext].second[std::make_pair(currentBlock, currentIns)] = newContext;
                                printTransitionGraph();
						    }
					    }
					    else
					    {
					        IN[std::make_tuple(currentContext, currentFunction, currentIns)].second = backwardNormalFunction(currentIns, currentFunction, currentContext);
					    }

					    Instruction* firstIns = &(*currentBlock->begin());
					    std::vector<bool> newIN = IN[std::make_tuple(currentContext, currentFunction, currentIns)].second;
					    //if in changes
					    if(currentIns==firstIns and prevIN != newIN)
					    {
					        errs() << " IN changes putting pred in forward and backward worklist" << '\n';
					        for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it)
					        {
						        forwardWorklist.push_front(std::make_tuple(currentFunction, *it, currentContext, 1));
						        backwardWorklist.push_back(std::make_tuple(currentFunction, *it, currentContext, 1));
					        }
					    }
					}
					//if next block is exit block
					while(!backwardWorklist.empty() and std::get<1>(backwardWorklist.back())==NULL and std::get<3>(backwardWorklist.back())==false)
					{
					    Function* function;
					    int funcContext;
					    std::tie(function, std::ignore, funcContext, std::ignore) = backwardWorklist.back();
					    backwardWorklist.pop_back();
					    std::vector<bool> fOUT = inValues[std::make_pair(funcContext, function)].first;
					    std::vector<bool> bOUT = inValues[std::make_pair(funcContext, function)].second;
					    Instruction* firstIns = &(*function->begin()->begin());
					    //setting outflow
					    std::get<1>(transitionTable[std::make_tuple(function, fOUT, bOUT)]) = IN[std::make_tuple(funcContext, function, firstIns)].first;
					    std::get<2>(transitionTable[std::make_tuple(function, fOUT, bOUT)]) = IN[std::make_tuple(funcContext, function, firstIns)].second;
                        errs() << " (backward) Leaving function " << function->getName() << " putting its callers in forward and backward worklist" << '\n';
					    for(auto callers : transitionGraph[funcContext].first)
					    {
					        Function* function;
					        BasicBlock* bb;
					        int context;
					        std::tie(function, bb, context) = callers;
                            std::deque<std::tuple<Function*, BasicBlock*, int, bool>> tempWorklist;
                            tempWorklist.push_back(std::make_tuple(function, bb, context, 0));
					        forwardWorklist.push_front(std::make_tuple(function, bb, context, 0));
					        backwardWorklist.push_back(std::make_tuple(function, bb, context, 0));
					        printWorklist(tempWorklist);
					        tempWorklist.clear();
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

			std::vector<bool> forwardNormalFlowFunction(Instruction* ins, Function* currentFunction, int contextId)
			{
//			    errs() << "forwardNormalFlwFunction called" << '\n';
				/*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
				return OUT[std::make_tuple(contextId, currentFunction, ins)].first;
			}

			void performChecking(std::string fName)
			{
			    errs() << "\nCheck under progress for function = " << fName << '\n';
			    errs() << "***********************************\n";
			    errs() << " \nPrinting forwardWorklist : " << '\n';
			    printWorklist(forwardWorklist);
			    errs () << "Size of forwardWorklist = " << forwardWorklist.size() << '\n';
			    errs() << " \nPrinting backwardWorklist : " << '\n';
			    printWorklist(backwardWorklist);
			    errs () << "Size of backwardWorklist = " << backwardWorklist.size() << '\n';
			    for(auto callerContext : transitionGraph)
			    {
			        errs() << callerContext.first << " -> ( ";
			        for(auto calledContext : callerContext.second.second)
			            errs() << calledContext.second << " ";
			        errs() << ")\n";
			    }
			    errs() << "***********************************\n";
			}

            void printWorklist(std::deque<std::tuple<Function*, BasicBlock*, int, bool>> worklist)
            {
                for(auto w : worklist)
                {
                    Function* wFunction;
                    BasicBlock* wBB;
                    int wContext;
                    bool entry;
                    std::tie(wFunction, wBB, wContext, entry) = w;
                    if(wBB != NULL)
                        errs() << wFunction->getName() << " " << wBB->getName() << " " << wContext << " " << entry << '\n';
                    else
                        errs() << wFunction->getName() << " " << "NULL" << " " << wContext << " " << entry << '\n';
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