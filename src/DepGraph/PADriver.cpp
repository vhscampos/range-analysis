#define DEBUG_TYPE "PADriver"

//#include <sstream>
//#include <unistd.h>
//#include <ios>
//#include <fstream>
//#include <string>
//#include <iostream>

//#include "llvm/Pass.h"
//#include "llvm/Module.h"
//#include "llvm/Instructions.h"
//#include "llvm/InstrTypes.h"
//#include "llvm/Support/raw_ostream.h"
//#include "llvm/Support/CallSite.h"
//#include "llvm/Support/Debug.h"
//#include "llvm/ADT/Statistic.h"

//#include "PointerAnalysis.h"
//#include "PADriver.h"

//#include <sstream>
//#include <sys/time.h>
//#include <sys/resource.h>
//#include <string>
//#include <vector>
//#include <set>
//#include <map>
//#include <iomanip>
//#include <fstream>
//#include <cstdlib>

//#include "llvm/Function.h"
//#include "llvm/Constants.h"
//#include "llvm/Analysis/DebugInfo.h"
//#include <llvm/Support/CommandLine.h>

#include "PADriver.h"

using namespace llvm;

STATISTIC(PABaseCt,  "Counts number of base constraints");
STATISTIC(PAAddrCt,  "Counts number of address constraints");
STATISTIC(PALoadCt,  "Counts number of load constraints");
STATISTIC(PAStoreCt, "Counts number of store constraints");
STATISTIC(PANumVert, "Counts number of vertices");
STATISTIC(PAMerges,  "Counts number of merged vertices");
STATISTIC(PARemoves, "Counts number of calls to remove cycle");
STATISTIC(PAMemUsage, "kB of memory");


PADriver::PADriver() : ModulePass(ID) {
                pointerAnalysis = new PointerAnalysis();
                currInd = 0;
                nextMemoryBlock = 1;

        }

bool PADriver::runOnModule(Module &M) {
        //struct timeval startTime, endTime;
        //struct rusage ru;

        // Get Time before analysis
        //getrusage(RUSAGE_SELF, &ru);
        //startTime = ru.ru_utime;
        if (pointerAnalysis == 0) pointerAnalysis = new PointerAnalysis();

        // Collect information
        for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
                if (!F->isDeclaration()) {
                        addConstraints(*F);
                        matchFormalWithActualParameters(*F);
                        matchReturnValueWithReturnVariable(*F);
                }
        }

        // Run the analysis
        pointerAnalysis->solve(false);
        double vmUsage, residentSet;
        process_mem_usage(vmUsage, residentSet);
        PAMemUsage = vmUsage;

        // Get some statistics
        PAMerges = pointerAnalysis->getNumOfMertgedVertices();
        PARemoves = pointerAnalysis->getNumCallsRemove();
        PANumVert = pointerAnalysis->getNumVertices();

        // Get Time after analysis
        //getrusage(RUSAGE_SELF, &ru);
        //endTime = ru.ru_utime;

        // Print time spent with analysis
        //double tS = startTime.tv_sec + (double)startTime.tv_usec / 1000000;
        //double tE = endTime.tv_sec + (double)endTime.tv_usec / 1000000;
        //double deltaTime = tE - tS;
        //std::stringstream ss(std::stringstream::in | std::stringstream::out);
        //ss << std::fixed << std::setprecision(4) << deltaTime;
        //std::string deltaTimeStr;
        //ss >> deltaTimeStr;
        //errs() << deltaTimeStr << " Time to perform the pointer analysis\n";

        return false;
}

// ============================= //

// Get the int ID of a new memory space
int PADriver::getNewMem(std::string name) {

        // Get a new ID for the memory
        int n = ++currInd;
        nameMap[n] = name;
        //PAMemUsage += sizeof(std::pair<int,std::string>);

        //errs() << "MemVal (" << v << " - " << n << "): " << nameMap[n] << "\n";
        //errs() << "Name of mem: " << nameMap[n] << "\n";

        return n;
}

// ============================= //

void PADriver::print(raw_ostream& O, const Module* M) const {
        std::stringstream dotFileSS;
        DEBUG( pointerAnalysis->print() );
        pointerAnalysis->printDot(dotFileSS, M->getModuleIdentifier(), nameMap);
        O << dotFileSS.str();
}

// ============================= //

std::string PADriver::intToStr(int v) {
        std::stringstream ss;
        ss << v;
        return ss.str();
}

// ============================= //

void PADriver::process_mem_usage(double& vm_usage, double& resident_set) {
        using std::ios_base;
        using std::ifstream;
        using std::string;

        vm_usage     = 0.0;
        resident_set = 0.0;

        // 'file' stat seems to give the most reliable results
        //
        ifstream stat_stream("/proc/self/stat",ios_base::in);

        // dummy vars for leading entries in stat that we don't care about
        //
        string pid, comm, state, ppid, pgrp, session, tty_nr;
        string tpgid, flags, minflt, cminflt, majflt, cmajflt;
        string utime, stime, cutime, cstime, priority, nice;
        string O, itrealvalue, starttime;

        // the two fields we want
        //
        unsigned long vsize;
        long rss;

        stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
                >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                >> utime >> stime >> cutime >> cstime >> priority >> nice
                >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

        stat_stream.close();

        long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
        vm_usage     = vsize / 1024.0;
        resident_set = rss * page_size_kb;
}

// ============================= //

void PADriver::addConstraints(Function &F) {
        for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
                for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
                        if (isa<PHINode>(I)) {
                                PHINode *Phi = dyn_cast<PHINode>(I);
                                const Type *Ty = Phi->getType();

                                if (Ty->isPointerTy()) {
                                        unsigned n = Phi->getNumIncomingValues();
                                        std::vector<Value*> values;

                                        for (unsigned i = 0; i < n; i++) {
                                                Value *v = Phi->getIncomingValue(i);

                                                values.push_back(v);
                                        }

                                        phiValues[I] = values;
                                }
                        }
                }
        }

        for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
                for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
                        if (isa<CallInst>(I)) {
                                CallInst *CI = dyn_cast<CallInst>(I);

                                if (CI) {
                                        Function *FF = CI->getCalledFunction();

                                        if (FF && (FF->getName() == "malloc" || FF->getName() == "realloc" ||
                                                                FF->getName() == "calloc")) {
                                                std::vector<int> mems;

                                                if (!memoryBlock.count(I)) {
                                                        mems.push_back(getNewMemoryBlock());
                                                        memoryBlock[I] = mems;
                                                } else {
                                                        mems = memoryBlock[I];
                                                }

                                                int a = Value2Int(I);
                                                pointerAnalysis->addAddr(a, mems[0]);
                                                PAAddrCt++;
                                        }
                                }
                        }

                        // Handle special operations
                        switch (I->getOpcode()) {
                                case Instruction::Alloca:
                                        {
                                                handleAlloca(I);

                                                break;
                                        }
                                case Instruction::GetElementPtr:
                                        {
                                                GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I);
                                                Value *v = GEPI->getPointerOperand();
                                                const PointerType *PoTy = cast<PointerType>(GEPI->getPointerOperandType());
                                                const Type *Ty = PoTy->getElementType();

                                                if (Ty->isStructTy()) {
                                                        if (phiValues.count(v)) {
                                                                std::vector<Value*> values = phiValues[v];

                                                                for (unsigned i = 0; i < values.size(); i++) {
                                                                        Value* vv = values[i];

                                                                        if (memoryBlocks.count(vv)) {
                                                                                for (unsigned j = 0; j < memoryBlocks[vv].size(); j++) {
                                                                                        int i = 0;
                                                                                        unsigned pos = 0;
                                                                                        bool hasConstantOp = true;

                                                                                        for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                if (i == 1) {
                                                                                                        if (isa<ConstantInt>(*it))
                                                                                                                pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                        else
                                                                                                                hasConstantOp = false;
                                                                                                }

                                                                                                i++;
                                                                                        }
                                                                                        if (hasConstantOp) {
                                                                                                std::vector<int> mems = memoryBlocks[vv][j];
                                                                                                int a = Value2Int(I);
                                                                                                if (pos < mems.size()) {
                                                                                                        pointerAnalysis->addAddr(a, mems[pos]);
                                                                                                        PAAddrCt++;
                                                                                                }
                                                                                        }
                                                                                }
                                                                        } else {
                                                                                if (memoryBlock.count(vv)) {
                                                                                        if (isa<BitCastInst>(vv)) {
                                                                                                BitCastInst *BC = dyn_cast<BitCastInst>(vv);

                                                                                                Value *v2 = BC->getOperand(0);

                                                                                                if (memoryBlock.count(v2)) {
                                                                                                        int i = 0;
                                                                                                        unsigned pos = 0;
                                                                                                        bool hasConstantOp = true;

                                                                                                        for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                                if (i == 1) {
                                                                                                                        if (isa<ConstantInt>(*it))
                                                                                                                                pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                                        else
                                                                                                                                hasConstantOp = false;
                                                                                                                }

                                                                                                                i++;
                                                                                                        }

                                                                                                        if (hasConstantOp) {
                                                                                                                std::vector<int> mems = memoryBlock[v2];
                                                                                                                int parent = mems[0];
                                                                                                                if (memoryBlock2.count(parent)) {
                                                                                                                        std::vector<int> mems2 = memoryBlock2[parent];

                                                                                                                        int a = Value2Int(I);
                                                                                                                        if (pos < mems2.size()) {
                                                                                                                                pointerAnalysis->addAddr(a, mems2[pos]);
                                                                                                                                PAAddrCt++;
                                                                                                                        }
                                                                                                                }
                                                                                                        }
                                                                                                }
                                                                                        } else {
                                                                                                int i = 0;
                                                                                                unsigned pos = 0;
                                                                                                bool hasConstantOp = true;

                                                                                                for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                        if (i == 1) {
                                                                                                                if (isa<ConstantInt>(*it))
                                                                                                                        pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                                else
                                                                                                                        hasConstantOp = false;
                                                                                                        }

                                                                                                        i++;
                                                                                                }

                                                                                                if (hasConstantOp) {
                                                                                                        std::vector<int> mems = memoryBlock[vv];
                                                                                                        int a = Value2Int(I);
                                                                                                        //pointerAnalysis->addBase(a, mems[pos]);
                                                                                                        if (pos < mems.size()) {
                                                                                                                pointerAnalysis->addAddr(a, mems[pos]);
                                                                                                                PAAddrCt++;
                                                                                                        }
                                                                                                }
                                                                                        }
                                                                                } else {
                                                                                        GetElementPtrInst *GEPI2 = dyn_cast<GetElementPtrInst>(vv);

                                                                                        if (!GEPI2)
                                                                                                goto saida;

                                                                                        Value *v2 = GEPI2->getPointerOperand();

                                                                                        if (memoryBlock.count(v2)) {
                                                                                                int i = 0;
                                                                                                unsigned pos = 0;
                                                                                                bool hasConstantOp = true;

                                                                                                for (User::op_iterator it = GEPI2->idx_begin(), e = GEPI2->idx_end(); it != e; ++it) {
                                                                                                        if (i == 1) {
                                                                                                                if (isa<ConstantInt>(*it))
                                                                                                                        pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                                else
                                                                                                                        hasConstantOp = false;
                                                                                                        }

                                                                                                        i++;
                                                                                                }

                                                                                                if (hasConstantOp) {
                                                                                                        std::vector<int> mems = memoryBlock[v2];
                                                                                                        if (pos < mems.size()) {
                                                                                                                int parent = mems[pos];

                                                                                                                i = 0;
                                                                                                                unsigned pos2 = 0;

                                                                                                                for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                                        if (i == 1)
                                                                                                                                pos2 = cast<ConstantInt>(*it)->getZExtValue();

                                                                                                                        i++;
                                                                                                                }

                                                                                                                if (memoryBlock2.count(parent)) {
                                                                                                                        std::vector<int> mems2 = memoryBlock2[parent];
                                                                                                                        int a = Value2Int(I);

                                                                                                                        if (pos2 < mems2.size()) {
                                                                                                                                pointerAnalysis->addAddr(a, mems2[pos2]);
                                                                                                                                PAAddrCt++;
                                                                                                                                memoryBlock[v] = mems2;
                                                                                                                        }
                                                                                                                }

                                                                                                        }
                                                                                                }
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                        } else {
                                                                if (memoryBlock.count(v)) {
                                                                        if (isa<BitCastInst>(v)) {
                                                                                BitCastInst *BC = dyn_cast<BitCastInst>(v);

                                                                                Value *v2 = BC->getOperand(0);

                                                                                if (memoryBlock.count(v2)) {
                                                                                        int i = 0;
                                                                                        unsigned pos = 0;
                                                                                        bool hasConstantOp = true;

                                                                                        for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                if (i == 1) {
                                                                                                        if (isa<ConstantInt>(*it))
                                                                                                                pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                        else
                                                                                                                hasConstantOp = false;
                                                                                                }

                                                                                                i++;
                                                                                        }

                                                                                        if (hasConstantOp) {
                                                                                                std::vector<int> mems = memoryBlock[v2];
                                                                                                int parent = mems[0];
                                                                                                if (memoryBlock2.count(parent)) {
                                                                                                        std::vector<int> mems2 = memoryBlock2[parent];

                                                                                                        int a = Value2Int(I);
                                                                                                        if (pos < mems2.size()) {
                                                                                                                pointerAnalysis->addAddr(a, mems2[pos]);
                                                                                                                PAAddrCt++;
                                                                                                        }
                                                                                                }
                                                                                        }
                                                                                }
                                                                        } else {
                                                                                int i = 0;
                                                                                unsigned pos = 0;
                                                                                bool hasConstantOp = true;

                                                                                for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                        if (i == 1) {
                                                                                                if (isa<ConstantInt>(*it))
                                                                                                        pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                else
                                                                                                        hasConstantOp = false;
                                                                                        }

                                                                                        i++;
                                                                                }

                                                                                if (hasConstantOp) {
                                                                                        std::vector<int> mems = memoryBlock[v];
                                                                                        int a = Value2Int(I);
                                                                                        //pointerAnalysis->addBase(a, mems[pos]);
                                                                                        if (pos < mems.size()) {
                                                                                                pointerAnalysis->addAddr(a, mems[pos]);
                                                                                                PAAddrCt++;
                                                                                        }
                                                                                }
                                                                        }
                                                                } else {
                                                                        GetElementPtrInst *GEPI2 = dyn_cast<GetElementPtrInst>(v);

                                                                        if (!GEPI2)
                                                                                goto saida;

                                                                        Value *v2 = GEPI2->getPointerOperand();

                                                                        if (memoryBlock.count(v2)) {
                                                                                int i = 0;
                                                                                unsigned pos = 0;
                                                                                bool hasConstantOp = true;

                                                                                for (User::op_iterator it = GEPI2->idx_begin(), e = GEPI2->idx_end(); it != e; ++it) {
                                                                                        if (i == 1) {
                                                                                                if (isa<ConstantInt>(*it))
                                                                                                        pos = cast<ConstantInt>(*it)->getZExtValue();
                                                                                                else
                                                                                                        hasConstantOp = false;
                                                                                        }

                                                                                        i++;
                                                                                }

                                                                                if (hasConstantOp) {
                                                                                        std::vector<int> mems = memoryBlock[v2];
                                                                                        if (pos < mems.size()) {
                                                                                                int parent = mems[pos];

                                                                                                i = 0;
                                                                                                unsigned pos2 = 0;

                                                                                                for (User::op_iterator it = GEPI->idx_begin(), e = GEPI->idx_end(); it != e; ++it) {
                                                                                                        if (i == 1)
                                                                                                                pos2 = cast<ConstantInt>(*it)->getZExtValue();

                                                                                                        i++;
                                                                                                }

                                                                                                if (memoryBlock2.count(parent)) {
                                                                                                        std::vector<int> mems2 = memoryBlock2[parent];
                                                                                                        int a = Value2Int(I);

                                                                                                        if (pos2 < mems2.size()) {
                                                                                                                pointerAnalysis->addAddr(a, mems2[pos2]);
                                                                                                                PAAddrCt++;
                                                                                                                memoryBlock[v] = mems2;
                                                                                                        }
                                                                                                }
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                        }
                                                } else {
                                                        int a = Value2Int(I);
                                                        int b = Value2Int(v);
                                                        pointerAnalysis->addBase(a, b);
                                                        PABaseCt++;
                                                }

saida:
                                                break;
                                        }
                                case Instruction::BitCast:
                                        {
                                                Value *src = I->getOperand(0);
                                                Value *dst = I;

                                                const Type *srcTy = src->getType();
                                                const Type *dstTy = dst->getType();

                                                if (srcTy->isPointerTy()) {
                                                        if (dstTy->isPointerTy()) {
                                                                const PointerType *PoTy = cast<PointerType>(dstTy);
                                                                const Type *Ty = PoTy->getElementType();

                                                                if (Ty->isStructTy()) {
                                                                        if (memoryBlock.count(src)) {
                                                                                std::vector<int> mems = memoryBlock[src];
                                                                                int parent = mems[0];

                                                                                handleNestedStructs(Ty, parent);
                                                                                memoryBlock[I] = mems;
                                                                        }
                                                                }
                                                        }

                                                        int a = Value2Int(I);
                                                        int b = Value2Int(src);
                                                        pointerAnalysis->addBase(a, b);
                                                        PABaseCt++;
                                                }

                                                break;
                                        }
                                case Instruction::Store:
                                        {
                                                // *ptr = v
                                                StoreInst *SI = dyn_cast<StoreInst>(I);
                                                Value *v = SI->getValueOperand();
                                                Value *ptr = SI->getPointerOperand();

                                                if (v->getType()->isPointerTy()) {
                                                        int a = Value2Int(ptr);
                                                        int b = Value2Int(v);

                                                        pointerAnalysis->addStore(a, b);
                                                        PAStoreCt++;
                                                }

                                                break;
                                        }
                                case Instruction::Load:
                                        {
                                                // I = *ptr
                                                LoadInst *LI = dyn_cast<LoadInst>(I);
                                                Value *ptr = LI->getPointerOperand();

                                                int a = Value2Int(I);
                                                int b = Value2Int(ptr);
                                                pointerAnalysis->addLoad(a, b);
                                                PALoadCt++;

                                                break;
                                        }
                                case Instruction::AtomicRMW:
                                        {
                                                // I = *ptr
                                                AtomicRMWInst *LI = dyn_cast<AtomicRMWInst>(I);
                                                Value *ptr = LI->getPointerOperand();

                                                int a = Value2Int(I);
                                                int b = Value2Int(ptr);
                                                pointerAnalysis->addLoad(a, b);
                                                PALoadCt++;

                                                break;
                                        }
                                case Instruction::AtomicCmpXchg:
                                        {
                                                // I = *ptr
                                                AtomicCmpXchgInst *LI = dyn_cast<AtomicCmpXchgInst>(I);
                                                Value *ptr = LI->getPointerOperand();

                                                int a = Value2Int(I);
                                                int b = Value2Int(ptr);
                                                pointerAnalysis->addLoad(a, b);
                                                PALoadCt++;

                                                break;
                                        }
                                case Instruction::PHI:
                                        {
                                                PHINode *Phi = dyn_cast<PHINode>(I);
                                                const Type *Ty = Phi->getType();

                                                if (Ty->isPointerTy()) {
                                                        unsigned n = Phi->getNumIncomingValues();
                                                        std::vector<Value*> values;

                                                        for (unsigned i = 0; i < n; i++) {
                                                                Value *v = Phi->getIncomingValue(i);
                                                                int a = Value2Int(I);
                                                                int b = Value2Int(v);
                                                                pointerAnalysis->addBase(a, b);
                                                                PABaseCt++;

                                                                values.push_back(v);

                                                                if (phiValues.count(v)) {
                                                                        if (memoryBlocks.count(v)) {
                                                                                memoryBlocks[I] = std::vector<std::vector<int> >();
                                                                                memoryBlocks[I].insert(memoryBlocks[I].end(), memoryBlocks[v].begin(), memoryBlocks[v].end());
                                                                        }
                                                                } else {
                                                                        if (memoryBlock.count(v)) {
                                                                                memoryBlocks[I] = std::vector<std::vector<int> >();

                                                                                if (isa<BitCastInst>(v)) {
                                                                                        BitCastInst *BC = dyn_cast<BitCastInst>(v);

                                                                                        Value *v2 = BC->getOperand(0);

                                                                                        if (memoryBlock.count(v2)) {
                                                                                                std::vector<int> mems = memoryBlock[v2];
                                                                                                int parent = mems[0];
                                                                                                std::vector<int> mems2 = memoryBlock2[parent];

                                                                                                memoryBlocks[I].push_back(mems2);
                                                                                        }
                                                                                } else
                                                                                        memoryBlocks[I].push_back(memoryBlock[v]);
                                                                        }
                                                                }
                                                        }
                                                }

                                                break;
                                        }
                        }
                }
        }
}

// ============================= //

void PADriver::matchFormalWithActualParameters(Function &F) {
        if (F.arg_empty() || F.use_empty()) return;

        for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E; ++UI) {
                User *U = *UI;

                if (isa<BlockAddress>(U)) continue;
                if (!isa<CallInst>(U) && !isa<InvokeInst>(U)) return;

                CallSite CS(cast<Instruction>(U));
                if (!CS.isCallee(UI))
                        return;

                CallSite::arg_iterator actualArgIter = CS.arg_begin();
                Function::arg_iterator formalArgIter = F.arg_begin();
                int size = F.arg_size();

                for (int i = 0; i < size; ++i, ++actualArgIter, ++formalArgIter) {
                        Value *actualArg = *actualArgIter;
                        Value *formalArg = formalArgIter;

                        int a = Value2Int(formalArg);
                        int b = Value2Int(actualArg);
                        pointerAnalysis->addBase(a, b);
                        PABaseCt++;
                }
        }
}

// ============================= //

void PADriver::matchReturnValueWithReturnVariable(Function &F) {
        if (F.getReturnType()->isVoidTy() || F.mayBeOverridden()) return;

        std::set<Value*> retVals;

        for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
                if (ReturnInst *RI = dyn_cast<ReturnInst>(BB->getTerminator())) {
                        Value *v = RI->getOperand(0);

                        retVals.insert(v);
                }
        }

        for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E; ++UI) {
                CallSite CS(*UI);
                Instruction *Call = CS.getInstruction();

                if (!Call || !CS.isCallee(UI)) continue;

                if (Call->use_empty()) continue;

                for (std::set<Value*>::iterator it = retVals.begin(), E = retVals.end(); it != E; ++it) {

                        int a = Value2Int(CS.getCalledFunction());
                        int b = Value2Int(*it);
                        pointerAnalysis->addBase(a, b);
                        PABaseCt++;
                }
        }
}

// ============================= //

void PADriver::handleAlloca(Instruction *I) {
        AllocaInst *AI = dyn_cast<AllocaInst>(I);
        const Type *Ty = AI->getAllocatedType();

        std::vector<int> mems;
        unsigned numElems = 1;
        bool isStruct = false;

        if (Ty->isStructTy()) { // Handle structs
                const StructType *StTy = dyn_cast<StructType>(Ty);
                numElems = StTy->getNumElements();
                isStruct = true;
        }

        if (!memoryBlock.count(I)) {
                for (unsigned i = 0; i < numElems; i++) {
                        mems.push_back(getNewMemoryBlock());

                        if (isStruct) {
                                const StructType *StTy = dyn_cast<StructType>(Ty);

                                if (StTy->getElementType(i)->isStructTy())
                                        handleNestedStructs(StTy->getElementType(i), mems[i]);
                        }
                }

                memoryBlock[I] = mems;
        } else {
                mems = memoryBlock[I];
        }

        for (unsigned i = 0; i < mems.size(); i++) {
                int a = Value2Int(I);
                pointerAnalysis->addAddr(a, mems[i]);
                PAAddrCt++;
        }
}

// ============================= //2

void PADriver::handleNestedStructs(const Type *Ty, int parent) {
        const StructType *StTy = dyn_cast<StructType>(Ty);
        unsigned numElems = StTy->getNumElements();
        std::vector<int> mems;

        for (unsigned i = 0; i < numElems; i++) {
                mems.push_back(getNewMemoryBlock());

                if (StTy->getElementType(i)->isStructTy())
                        handleNestedStructs(StTy->getElementType(i), mems[i]);
        }

        memoryBlock2[parent] = mems;

        for (unsigned i = 0; i < mems.size(); i++) {
                pointerAnalysis->addAddr(parent, mems[i]);
                PAAddrCt++;
        }
}

// ============================= //

int PADriver::getNewMemoryBlock() {
        return nextMemoryBlock++;
}

// ============================= //

int PADriver::getNewInt() {
        return getNewMemoryBlock();
}

// ============================= //

int PADriver::Value2Int(Value *v) {

        int n;

        if (value2int.count(v))
                return value2int[v];

        n = getNewInt();
        value2int[v] = n;
        int2value[n] = v;
//      errs() << "int " << n << "; value " << v << "\n";

        // Also get a name for it
        if (v->hasName()) {
                nameMap[n] = v->getName();
        }
        else if (isa<Constant>(v)) {
                nameMap[n] = "constant";
                //nameMap[n] += intToStr(n);
        }
        else {
                nameMap[n] = "unknown";
        }

        return n;
}

// ============================= //

// Get a (possibly new) int ID associated with
// the given Value
//int PADriver::Value2Int(Value* v) {
        //// If we already have this value,
        //// just return it!
        //if (valMap.count(v))
                //return valMap[v];

        //// If it's a new value, get new ID
        //// and save it
        //int n = ++currInd;
        //valMap[v] = n;

        //// Count the memory usage
        ////PAMemUsage += sizeof(std::pair<Value*,int>);

        //// Also get a name for it
        //if (v->hasName()) {
                //nameMap[n] = v->getName();
        //}
        //else if (isa<Constant>(v)) {
                //nameMap[n] = "constant";
                ////nameMap[n] += intToStr(n);
        //}
        //else {
                //nameMap[n] = "unknown";
        //}

        //return n;
//}

// ============================= //
/*
Value* AddrLeaks::Int2Value(int x)
{
        return int2value[x];
}
*/
// ========================================= //

// Register the pass to the LLVM framework
char PADriver::ID = 0;
//static RegisterPass<PADriver> X("pa", "Pointer Analysis Driver Pass");
static RegisterPass<PADriver> X("pa", "Pointer Analysis Driver Pass", true, true);
