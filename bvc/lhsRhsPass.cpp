using namespace llvm;
class LhsRhsFinder
{
    protected:
        using lhs = std::pair<Value*, long long int >;
	    using rhs = std::vector<std::pair<Value*, long long int >>;
        std::map<Instruction*, std::pair<lhs, rhs>> lhsRhsMap;
        std::stack<std::vector<std::pair<Value*, long long int >>> st;
    	std::map<Instruction*, rhs> useMap;
    	 std::map<std::tuple<Function*, BasicBlock*, Instruction*>, std::set<std::tuple<Function*, BasicBlock*, Instruction*>>> predIns, succIns;
    	std::map<std::pair<BasicBlock*, Function*>, std::map<Instruction*, long long int >> instCounter;
    	std::map<Instruction*, rhs> funcToArgMap;

    public:
        void metaDataSetter(Function* F);
        void setPredSucc(Function* F);
        void computePredSucc(Instruction* I, BasicBlock* BB, Function* F);
        std::pair<Value*, long long int > getLHS(Instruction* ins);
        std::vector<std::pair<Value*, long long int >> getRHS(Instruction*);
        std::vector<std::pair<Value*, long long int >> getUSE(Instruction*);
        std::vector<std::pair<Value*, long long int >> getArg(Instruction*);
        std::set<std::tuple<Function*, BasicBlock*, Instruction*>> getPredecessor(Function*, BasicBlock*, Instruction*);
        std::set<std::tuple<Function*, BasicBlock*, Instruction*>> getSuccesor(Function*, BasicBlock*, Instruction*);
        void print();



};


//------------------ utility function to compute pred and succ ----------//
void LhsRhsFinder::computePredSucc(Instruction* I, BasicBlock* BB, Function* F)
{
    long long int  counter = instCounter[std::make_pair(BB, F)][I];
    if(counter == 0)
    {//first instruction of basic block
        for(auto it = pred_begin(BB), et = pred_end(BB); it != et; ++it)
        {
            BasicBlock* pred = *it;
            Function* parent = pred->getParent();
//            if(instCounter[std::make_pair(pred, parent)] != instCounter.end())
//            {
                Instruction* pIns = instCounter[std::make_pair(pred, parent)].rbegin()->first;
                predIns[std::make_tuple(F, BB, I)].insert(std::make_tuple(parent, pred, pIns));
                succIns[std::make_tuple(parent, pred, pIns)].insert(std::make_tuple(F, BB, I));
//            }
        }
    }
    else
    {
        Instruction* pIns = instCounter[std::make_pair(BB, F)].rbegin()->first;
        predIns[std::make_tuple(F, BB, I)].insert(std::make_tuple(F, BB, pIns));
        succIns[std::make_tuple(F,BB,pIns)].insert(std::make_tuple(F,BB,I));
    }

}

std::set<std::tuple<Function*, BasicBlock*, Instruction*>> LhsRhsFinder::getPredecessor(Function* f, BasicBlock* bb, Instruction* i)
{   //returns pred vector
    return predIns[std::make_tuple(f, bb, i)];
}

std::set<std::tuple<Function*, BasicBlock*, Instruction*>> LhsRhsFinder::getSuccesor(Function* f, BasicBlock* bb, Instruction* i)
{   //returns succ vector
    return succIns[std::make_tuple(f, bb, i)];
}


//-------------------- different API ------------------------//
std::pair<Value*, long long int > LhsRhsFinder::getLHS(Instruction* ins)
{
    return lhsRhsMap[ins].first;
}

std::vector<std::pair<Value*, long long int >> LhsRhsFinder::getRHS(Instruction* ins)
{
    return lhsRhsMap[ins].second;
}

std::vector<std::pair<Value*, long long int >> LhsRhsFinder::getUSE(Instruction* ins)
{
    return useMap[ins];
}

std::vector<std::pair<Value*, long long int >> LhsRhsFinder::getArg(Instruction* ins)
{
    return funcToArgMap[ins];
}
//-------------------------------------------------------------------------//

/*
Computes LHS=RHS pair,
Computes USE variables
Computes predecessor and succors
*/
void LhsRhsFinder::metaDataSetter(Function* F)
{
    for(Function::iterator bb=F->begin(), e=F->end(); e!=bb;++bb)
    {
        BasicBlock* BB = &(*bb);
        long long int  counter = 0;
        for(BasicBlock::iterator i=bb->begin(), e=bb->end(); i!=e; ++i)
        {
            Instruction* I = &(*i);
            if(isa<StoreInst>(I))
            {
            //lhs = rhs
                bool flag = true;
                std::pair<Value*, long long int > LHS;
                std::vector<std::pair<Value*, long long int >> RHS;
                if(I->getOperand(0)->getName().size()>0 and isa<GlobalVariable>(I->getOperand(0)))
                {
                    RHS.push_back(std::make_pair(I->getOperand(0), -1));
                    if(st.empty())
                    {
                        LHS = std::make_pair(I->getOperand(1), 0);
                    }
                    else
                    {
                        LHS = st.top()[0];
                        LHS.second++;
                        st.pop();
                    }
                }
                else if(st.empty())
                {
                    flag = false;
                }
                else if(st.size() == 1)
                {
                    LHS = std::make_pair(I->getOperand(1), 0);
                    RHS = st.top();
                    st.pop();
                }
                else
                {
                    LHS = st.top()[0];
                    LHS.second++;
                    st.pop();
                    RHS = st.top();
                    st.pop();
                }
                if(flag)
                {
                    lhsRhsMap[I] = std::make_pair(LHS, RHS);

                //pred and succ calculation
                    instCounter[std::make_pair(BB, F)][I] = counter;
                    counter++;
                }

            }
            else if(isa<LoadInst>(I))
            {
                if(st.empty() or isa<GlobalVariable>(I->getOperand(0)))
                {
                    std::vector<std::pair<Value*, long long int >> v;
                    v.push_back(std::make_pair(i->getOperand(0), 0));
                    st.push(v);
                }
                else
                {
                    st.top()[0].second++;
                }
            }
            else if(isa<ReturnInst>(I) or isa<ICmpInst>(I))
            {

                if(!st.empty())
                {

                    while(!st.empty())
                    {

                        useMap[I].push_back(st.top()[0]);
                        st.pop();
                    }
                    instCounter[std::make_pair(BB, F)][I] = counter;
                    counter++;
                }
            }
            else if(isa<CallInst>(I))
            {
                funcToArgMap[I];
                while(!st.empty())
                {
                    funcToArgMap[I].push_back(st.top()[0]);
                    st.pop();
                }
                instCounter[std::make_pair(BB, F)][I] = counter;
                counter++;
            }
            else if(st.size() >= 2)
            {

                std::vector<std::pair<Value*, long long int >> v = st.top();
                st.pop();
                for(auto it : st.top())
                {
                    v.push_back(it);
                }
                st.pop();
                st.push(v);
            }

        }
    }
}

void LhsRhsFinder::setPredSucc(Function* F)
{
    for(Function::iterator bb=F->begin(), e=F->end(); e!=bb;++bb)
    {
        BasicBlock* BB = &(*bb);
        for(BasicBlock::iterator i=bb->begin(), e=bb->end(); i!=e; ++i)
        {
            Instruction* I = &(*i);
            if(instCounter[std::make_pair(BB, F)].find(I) != instCounter[std::make_pair(BB, F)].end())
                computePredSucc(I, BB, F);
        }
    }
}



//-------------------- prlong long int  function --------------------------//
void LhsRhsFinder::print()
{
    for(auto func: funcToArgMap)
    {
        func.first->print(errs());
        errs() << " : ";
        for(auto it : func.second)
        {
            errs() << "( " << it.first->getName() <<", " << it.second << " ),";
        }
        errs()<<"\n";
    }
    errs() << "-----------------\n";

}
