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
	using callers = std::vector<std::tuple<Function*, callSite, contextId>>;
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
				    errs() << "Main block start" << '\n';
				    context++;
					initContext(&F, forwardBI, backwardBI);
					
					while(!forwardWorklist.empty())
					{
						// doAnalysisBackward();
						doAnalysisForward();
					}
					errs() << "Main block end" << '\n';
				}
				return false;
			}

			//function to initialize a new context
			void initContext(Function *F, std::vector<bool> forwardEntryValue, std::vector<bool> backwardEntryValue)
			{
			    errs() << "initContext for context" << context << " and function " << F->getName() <<'\n';
                inValues[std::make_pair(context, F)] = std::make_pair(forwardEntryValue, backwardEntryValue);
				//registering into transition table
				transitionTable[std::make_tuple(F, forwardEntryValue, backwardEntryValue)] = std::make_tuple(context, forwardTop, backwardTop);
				//updating transition graph
				transitionGraph[context];

				std::vector<std::tuple<Function*, BasicBlock*, int>> tempWorklist;
				BasicBlock* nullBlock = NULL;
				backwardWorklist.push_back(std::make_tuple(F, nullBlock, context));
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
				backwardWorklist.push_back(std::make_tuple(F, nullBlock, context));

				// filling forwardWorklist
				forwardWorklist.push_front(std::make_tuple(F, nullBlock, context));
				for(int i=tempWorklist.size()-1; i>=0; i--)
				{
					forwardWorklist.push_front(tempWorklist[i]);
				}
				forwardWorklist.push_front(std::make_tuple(F, nullBlock, context));

				Instruction* start = &(*F->begin()->begin());
				Instruction* end = &(*std::get<1>(tempWorklist[tempWorklist.size()-1])->rbegin());
				IN[std::make_tuple(context, F, start)].first = forwardEntryValue;
				OUT[std::make_tuple(context, F, end)].second = backwardEntryValue;
				funcTolastIns[F] = end;
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
					std::tie(currentFunction, currentBlock, contextId) = forwardWorklist.front();
					forwardWorklist.pop_front();

					// an entry node 
					if(currentBlock == NULL)
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
						    errs() << " Function call " << '\n';
						    int numberOfArg = currentIns->operands().end() - currentIns->operands().begin() - 1;
						    Function* calledFunction = cast<CallInst>(currentIns)->getCalledFunction();
						    if(calledFunction->getName() == "_Z5checkv")
						    {
						        performChecking();  //checker code
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
                                    initContext(calledFunction, fIN, bIN);
                                }
                                else //exists
                                {
                                    newContext = std::get<0>(transitionTable[std::make_tuple(calledFunction, fIN, bIN)]);
                                    std::vector<bool> outFlow = std::get<1>(transitionTable[std::make_tuple(calledFunction, fIN, bIN)]);
                                    OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = merge(prevOUT, outFlow);
                                }

                                //setting up the edges of new context
                                transitionGraph[context].first.push_back(std::make_tuple(currentFunction, currentBlock, contextId));

                                //setting up the edges of calling context
                                transitionGraph[contextId].second[std::make_pair(currentBlock, currentIns)] = context;
						    }
						}
						else
						{
						    errs() << " normal ins call " << '\n';
							OUT[std::make_tuple(contextId, currentFunction, currentIns)].first = forwardNormalFlowFunction(currentIns, currentFunction, contextId);
						}
						Instruction* lastIns = currentBlock->getTerminator();
						std::vector<bool> newOUT = OUT[std::make_tuple(contextId, currentFunction, currentIns)].first;

						//if OUT changes
						if(currentIns==lastIns and prevOUT != newOUT)
						{
						    for(BasicBlock* Succ : successors(currentBlock))
						    {
						        forwardWorklist.push_front(std::make_tuple(currentFunction, Succ, contextId));
						        backwardWorklist.push_back(std::make_tuple(currentFunction, Succ, contextId));
						    }
						}
					}
					//if next block is return block
					while(!forwardWorklist.empty() and std::get<1>(forwardWorklist.front()) == NULL)
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
					    for(auto callers : transitionGraph[funcContext].first)
					    {
					        forwardWorklist.push_front(callers);
					        backwardWorklist.push_back(callers);
					    }

					}
				}
				errs() << "doAnalysisForward end" << '\n';
			}

			std::vector<bool> merge(std::vector<bool> a1, std::vector<bool> a2)
			{
				// a1 merge a2
				return a1;
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

			void performChecking()
			{
			    errs() << '\t' << "Check under progress" << '\n';
			    errs() << "***********************************\n";
			    errs() << "Context\tcallers\tcallee\n";
			    for(auto i : transitionGraph)
			        errs() << i.first << "-> " << i.second.first.size() << " -> " << i.second.second.size() << "\n";
			    errs() << forwardWorklist.size() << '\n';
			    errs() << "***********************************\n";
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