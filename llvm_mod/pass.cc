#include <llvm/Config/config.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/Support/DebugLoc.h>
//#include <llvm/Analysis/DebugInfo.h>

#include <llvm/Support/CommandLine.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/DataLayout.h>

#include <cxxabi.h>

#include <map>
#include <sstream>
#include <vector>

using namespace llvm;

namespace {
  bool IsAtomic(AtomicOrdering o) {
    return o == Monotonic || o == Acquire || o == Release || 
        o == AcquireRelease || o == SequentiallyConsistent;
  }

  struct MemoryInterceptPass : public ModulePass {
    static char ID;
    MemoryInterceptPass() : ModulePass(ID) {
      PassRegistry &Registry = *PassRegistry::getPassRegistry();
      initializeDataLayoutPass(Registry);
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DataLayout>();
    }

    int next;

    Module* M;
    Type *UIntPtr, *Void, *Int8Ptr, *Int64, *Int32;

    DataLayout* TD;

    Constant *LoadFn, *StoreFn, *CmpXChgFn, *FenceFn, *AtomicRMWFn;

    /*
    std::string GetTypeName(Type* type) {
      if (isa<PointerType>(type)) {
        auto pointed = static_cast<SequentialType*>(type)->getElementType();
        return GetTypeName(internal) + "*";
      } else {
        std::string s;
        raw_string_ostream os(s);
        type->print(os);
        os.flush();
        return s;
      }
    }
    */

    static std::string Simplify(StringRef str) {
      char const *mangled = str.str().c_str();
      int status;
      char *demangled = abi::__cxa_demangle(mangled, 0, 0, &status);

      std::stringstream ss;
      if (status == 0) {
        char *in = demangled;
        int depth = 0;
        bool dots = true;
        for (; *in; in++) {
          if (*in == '<' || *in == '(') {
            if (depth == 0) {
              ss << *in;
              dots = false;
            }
            depth++;
          } else if (*in == '>' || *in == ')') {
            depth--;
            if (depth == 0) {
              ss << *in;
            }
          } else {
            if (depth == 0) {
              ss << *in;
            } else if (!dots) {
              ss << "...";
              dots = true;
            }
          }
        }
        return ss.str();
      } else {
        return str;
      }
    }

    static GlobalVariable *createPrivateGlobalForString(
        Module* M, StringRef Str) {
      Constant *StrConst = ConstantDataArray::getString(M->getContext(), 
          Simplify(Str));
      return new GlobalVariable(*M, StrConst->getType(), true,
          GlobalValue::PrivateLinkage, StrConst, "");
    }

    int GetByteSize(Type* type) {
      return TD->getTypeAllocSize(type);
    }

    Value* CreateCastToInt64(Value* value, Instruction* insertBefore) {
      if (isa<PointerType>(value->getType())) {
        return CastInst::CreatePointerCast(value, Int64, "", insertBefore);
      } else {
        return CastInst::CreateZExtOrBitCast(value, Int64, "", insertBefore);
      }
    }

    Instruction* CreateCastFromInt64(Value* value, Type* type) {
      if (isa<PointerType>(type)) {
        return CastInst::Create(Instruction::IntToPtr, value, type);
      } else {
        return CastInst::CreateTruncOrBitCast(value, type, "");
      }
    }

    Constant* GetTrace(Module* M, Instruction* instruction) {
      DebugLoc loc = instruction->getDebugLoc();
      //int result = -1;
      bool first = true;
      std::stringstream ss;
      ss << "[";
      // Fixme: crashes on ben
      /*
      while (!loc.isUnknown()) {
        if (!first) {
          ss << ", ";
        } else {
          first = false;
        }

        ss << "{";
        ss << "'function': \"" << Simplify(getDISubprogram(loc.getScope(M->getContext())).getLinkageName()) << "\", ";
        ss << "'file': '" << DIScope(loc.getScope(M->getContext())).getFilename().str() << "', ";
        ss << "'line': " << loc.getLine() << ", 'column': " << loc.getCol();
        ss << "}";

        //result = loc.getLine();
        loc = DebugLoc::getFromDILocation(loc.getInlinedAt(M->getContext()));
      }
      */
      ss << "]";

      GlobalVariable* name = createPrivateGlobalForString(M, ss.str());
      return ConstantExpr::getPointerCast(name, Int8Ptr);
    }

    virtual bool runOnModule(Module &module) {
      M = &module;
      UIntPtr = Type::getInt32PtrTy(M->getContext());
      Type *Int32 = Type::getInt32Ty(M->getContext());
      Void = Type::getVoidTy(M->getContext());

      TD = &getAnalysis<DataLayout>();

      Int32 = Type::getInt32Ty(M->getContext());
      Int64 = Type::getInt64Ty(M->getContext());
      Int8Ptr = Type::getInt8PtrTy(M->getContext());

      next = 1234;

      LoadFn = M->getOrInsertFunction("InterceptLoad", 
          Int64, Int8Ptr, Int32, Int32, Int8Ptr, NULL);
      StoreFn = M->getOrInsertFunction("InterceptStore", 
          Void, Int8Ptr, Int64, Int32, Int32, Int8Ptr, NULL);
      CmpXChgFn = M->getOrInsertFunction("InterceptCmpXChg",
          Int64, Int8Ptr, Int64, Int64, Int32, Int8Ptr, NULL);
      AtomicRMWFn = M->getOrInsertFunction("InterceptAtomicRMW",
          Int64, Int8Ptr, Int64, Int32, Int32, Int8Ptr, NULL);
      FenceFn = M->getOrInsertFunction("InterceptFence", Void, NULL);

      for (Module::iterator F = M->begin(), E = M->end(); F != E; ++F) {
        GlobalVariable* name = createPrivateGlobalForString(M, F->getName());
        Constant* namePointer = ConstantExpr::getPointerCast(name, Int8Ptr);
        for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
          for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
            if (isa<LoadInst>(BI)) {
              LoadInst& IN = static_cast<LoadInst&>(*BI);
              std::vector<Value*> args(4);

              args[0] = CastInst::CreateZExtOrBitCast(IN.getPointerOperand(), Int8Ptr,
                  "", BI);
              args[1] = ConstantInt::get(Int32, GetByteSize(IN.getType()));
              args[2] = ConstantInt::get(Int32, IsAtomic(IN.getOrdering()));
              args[3] = GetTrace(M, BI);
              Instruction* callLoad = CallInst::Create(
                  LoadFn,
                  args, "");


              BI->getParent()->getInstList().insert(BI, callLoad);
              ReplaceInstWithInst(BI->getParent()->getInstList(), BI, CreateCastFromInt64(callLoad, BI->getType()));

            } else if (isa<StoreInst>(BI)) {
              StoreInst& IN = static_cast<StoreInst&>(*BI);
              std::vector<Value*> args(5);

              args[0] = CastInst::CreateZExtOrBitCast(IN.getPointerOperand(), Int8Ptr,
                  "", BI);
              args[1] = CreateCastToInt64(IN.getValueOperand(),
                  BI);
              args[2] = ConstantInt::get(Int32, GetByteSize(IN.getValueOperand()->getType()));
              args[3] = ConstantInt::get(Int32, IsAtomic(IN.getOrdering()));
              args[4] = GetTrace(M, BI);
              Instruction* callStore = CallInst::Create(
                  StoreFn,
                  args, "");

              ReplaceInstWithInst(BI->getParent()->getInstList(), BI, callStore);

            } else if (isa<AtomicCmpXchgInst>(BI)) {
              AtomicCmpXchgInst& IN = static_cast<AtomicCmpXchgInst&>(*BI);
              std::vector<Value*> args(5);

              args[0] = CastInst::CreateZExtOrBitCast(IN.getPointerOperand(), Int8Ptr,
                  "", BI);
              args[1] = CreateCastToInt64(IN.getCompareOperand(),
                  BI);
              args[2] = CreateCastToInt64(IN.getNewValOperand(),
                  BI);
              args[3] = ConstantInt::get(Int32, GetByteSize(IN.getCompareOperand()->getType()));
              args[4] = GetTrace(M, BI);
              Instruction* callCmpXChg = CallInst::Create(
                  CmpXChgFn,
                  args, "");

              BI->getParent()->getInstList().insert(BI, callCmpXChg);
              ReplaceInstWithInst(BI->getParent()->getInstList(), BI, CreateCastFromInt64(callCmpXChg, BI->getType()));
            } else if (isa<FenceInst>(BI)) {
              std::vector<Value*> args(0);

              Instruction* callFence = CallInst::Create(
                  FenceFn,
                  args, "");

              ReplaceInstWithInst(BI->getParent()->getInstList(), BI, callFence);

            } else if (isa<AtomicRMWInst>(BI)) {
              AtomicRMWInst& IN = static_cast<AtomicRMWInst&>(*BI);

              switch (IN.getOperation()) {
                case AtomicRMWInst::Xchg:
                case AtomicRMWInst::Add:
                case AtomicRMWInst::Sub:
                  break;
                default:
                  errs() << "Found an unsupported atomic RMW...: " << IN.getOperation() << "\n";
                  assert(0);
              }

              std::vector<Value*> args(5);
              args[0] = CastInst::CreateZExtOrBitCast(IN.getPointerOperand(), Int8Ptr, "", BI);
              args[1] = CreateCastToInt64(IN.getValOperand(), BI);
              args[2] = ConstantInt::get(Int32, IN.getOperation());
              args[3] = ConstantInt::get(Int32, GetByteSize(IN.getValOperand()->getType()));
              args[4] = GetTrace(M, BI);
              Instruction* callCmpXChg = CallInst::Create(AtomicRMWFn, args, "");

              BI->getParent()->getInstList().insert(BI, callCmpXChg);
              ReplaceInstWithInst(BI->getParent()->getInstList(), BI, CreateCastFromInt64(callCmpXChg, BI->getType()));
            } else if (isa<MemSetInst>(BI)) {
              MemSetInst& IN = static_cast<MemSetInst&>(*BI);
            } else if (isa<MemCpyInst>(BI) || isa<MemMoveInst>(BI)) {
              MemCpyInst& IN = static_cast<MemCpyInst&>(*BI);
            }
          }
        }
      }

      for (Module::iterator F = M->begin(), E = M->end(); F != E; ++F) {
        if (!F->isDeclaration() && F->hasLinkOnceLinkage()) {
          // Make local copies of link-once functions so we don't end up
          // intercepting Codex code.
          F->setName(F->getName() + "_copy_for_codex");
        } else if (F->getName() == "_Znwm") {
          F->setName("InterceptNew");
        } else if (F->getName() == "_ZdlPv") {
          F->setName("InterceptDelete");
        } else if (F->getName() == "llvm.memcpy.p0i8.p0i8.i32") {
          F->setName("InterceptMemcpy");
        } else if (F->getName() == "llvm.memset.p0i8.i64") {
          F->setName("InterceptMemset");
        }
      }

      return false;
    }
  };
}

char MemoryInterceptPass::ID = 0;
static RegisterPass<MemoryInterceptPass> X("intercept", 
    "Intercept memory accesses");

