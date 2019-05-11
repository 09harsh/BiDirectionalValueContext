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

namespace{

	using methodName = std::string;
	using contextId = int;
	using forwardEntryValue = std::vector<bool>;
	using backwardEntryValue = std::vector<bool>;
	using forwardExitValue = std::vector<bool>;
	using backwardExitValue = std::vector<bool>;
	using forwardDataFlowValue = std::vector<bool>;
	using backwardDataFlowValue = std::vector<bool>;
	using callSite = BasicBlock*;
	using callers = std::vector<std::pair<methodName, callSite>>;
	using blockId = BasicBlock*;
	using insId = Instruction*;
	using callee = std::map<callSite, contextId>;
	
	//declaring globals
	std::map<std::tuple<methodName, forwardEntryValue, backwardEntryValue>, std::tuple<contextId, forwardExitValue, backwardExitValue>> transitionTable;
	std::map<contextId, std::pair<callers, callee>> transitionGraph;
	std::deque<std::tuple<methodName, blockId, contextId>> forwardWorklist, backwardWorklist;
	std::vector<bool> forwardBI, backwardBI, forwardTop, forwardBottom, backwardTop, backwardBottom;
	std::map<std::tuple<contextId, methodName, insId>, std::pair<forwardDataFlowValue, backwardDataFlowValue>> IN, OUT;

	//functionPass
	class BValueContext : public FunctionPass{
		public:
			static char ID;
			BValueContext() : FunctionPass(ID){}
			virtual bool runOnFunction(Function &F) {
				errs()<<F.getName()<<'\n';
				if(F.getName() == "main")
				{
					initContext(F, forwardBI, backwardBI);
					
					while(!forwardWorklist.empty() or !backwardWorklist.empty())
					{
						// doAnalysisBackward();
						doAnalysisForward();
					}
					
				}
				return false;
			}

			//function to initialize a new context
			void initContext(Function &F, std::vector<bool> forwardEntryValue, std::vector<bool> backwardEntryValue)
			{
				static int contextId = 0;
				std::string mName = F.getName();

				//registering into transition table
				transitionTable[std::make_tuple(mName, forwardEntryValue, backwardEntryValue)] = std::make_tuple(++contextId, forwardTop, backwardTop);

				/*
					filling backwardWorklist
						0 :- entry
						1 :- exit
				*/
				std::vector<std::tuple<std::string, BasicBlock*, int>> tempWorklist;
				BasicBlock* nullBlock = NULL;
				backwardWorklist.push_back(std::make_tuple(mName, nullBlock, 1));
				for(Function::iterator bb=F.begin(), e=F.end(); e!=bb;++bb){
					BasicBlock *BB = &(*bb);
					backwardWorklist.push_back(std::make_tuple(mName, BB, contextId));
					tempWorklist.push_back(std::make_tuple(mName, BB, contextId));
					
					//initializing IN and OUT with 'top' value
					for(BasicBlock::iterator i=bb->begin(), e=bb->end(); i!=e; ++i){
						Instruction* I = &(*i);
						IN[std::make_tuple(contextId, mName, I)] = std::make_pair(forwardTop, backwardTop);
						OUT[std::make_tuple(contextId, mName, I)] = std::make_pair(forwardTop, backwardTop);
					}
				}
				backwardWorklist.push_back(std::make_tuple(mName, nullBlock, 0));

				// filling forwardWorklist
				forwardWorklist.push_front(std::make_tuple(mName, nullBlock, 1));
				for(int i=tempWorklist.size()-1; i>=0; i--){
					forwardWorklist.push_front(tempWorklist[i]);
				}
				forwardWorklist.push_front(std::make_tuple(mName, nullBlock, 0));

				Instruction* start = &(*F.begin()->begin());
				Instruction* end = &(*std::get<1>(tempWorklist[tempWorklist.size()-1])->rbegin());
				IN[std::make_tuple(contextId, mName, start)].first = forwardEntryValue;
				OUT[std::make_tuple(contextId, mName, end)].second = backwardEntryValue;

			}


			//procedure to do forward analysis
			void doAnalysisForward()
			{
				while(!forwardWorklist.empty()){
					int contextId;
					std::string methodName;
					BasicBlock* currentBlock;
					std::tie(methodName, currentBlock, contextId) = forwardWorklist.front();
					forwardWorklist.pop_front();

					// an entry node 
					if(currentBlock == NULL){
						std::tie(methodName, currentBlock, contextId) = forwardWorklist.front();
						forwardWorklist.pop_front();
					}
					else{
						Instruction* firstInc = &(*currentBlock->begin());
						IN[std::make_tuple(contextId, methodName, firstInc)].first = forwardTop;
						for(auto it = pred_begin(currentBlock), et = pred_end(currentBlock); it != et; ++it){
							BasicBlock* pred = *it;
							Instruction* pred_ins = &(*pred->rbegin());
							IN[std::make_tuple(contextId, methodName, firstInc)].first = merge(firstInc, pred_ins, methodName, contextId);
						}
					}
					for(auto insBB=currentBlock->begin();insBB!=currentBlock->end();insBB++){
						Instruction* currentIns = &(*insBB);
						//function call
						if(currentIns->getOpcode() == 54){

						}
						else{
							OUT[std::make_tuple(contextId, methodName, currentIns)].first = forwardNormalFlowFunction(currentIns, methodName, contextId);
						}
					}
					//if next block is return block
					if(std::get<1>(forwardWorklist.front()) == NULL)
					{
						forwardWorklist.pop_front();
					}
				}
			}

			std::vector<bool> merge(Instruction* ins, Instruction* pred, std::string methodName, int contextId)
			{
				//return IN[contextId, methodName, currentBlock] merge OUT[contextId, methodName, pred]
				return IN[std::make_tuple(contextId, methodName, ins)].first;
			}

			std::vector<bool> forwardNormalFlowFunction(Instruction* ins, std::string methodName, int contextId)
			{
				/*
					Perform normal flow operation
					Compute GEN, KILL
					and return OUT = (IN - KILL) U GEN;
				*/
				return OUT[std::make_tuple(contextId, methodName, ins)].first;
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