#ifndef __SQ_LLVM_H__
#define __SQ_LLVM_H__

#ifdef SQ_JIT_LLVM


#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/AsmPrinter.h"

#ifdef _MSC_VER
#pragma comment(lib, "LLVMAnalysis.lib")
#pragma comment(lib, "LLVMArchive.lib")
#pragma comment(lib, "LLVMCodeGen.lib")
#pragma comment(lib, "LLVMAsmParser.lib")
#pragma comment(lib, "LLVMAsmPrinter.lib")
#pragma comment(lib, "LLVMBitReader.lib")
#pragma comment(lib, "LLVMBitWriter.lib")
#pragma comment(lib, "LLVMCodeGen.lib")
#pragma comment(lib, "LLVMCore.lib")
#pragma comment(lib, "LLVMDebugInfo.lib")
#pragma comment(lib, "LLVMExecutionEngine.lib")
#pragma comment(lib, "LLVMInstCombine.lib")
#pragma comment(lib, "LLVMInstrumentation.lib")
#pragma comment(lib, "LLVMInterpreter.lib")
#pragma comment(lib, "LLVMipa.lib")
#pragma comment(lib, "LLVMipo.lib")
#pragma comment(lib, "LLVMIRReader.lib")
#pragma comment(lib, "LLVMJIT.lib")
#pragma comment(lib, "LLVMLinker.lib")
#pragma comment(lib, "LLVMMC.lib")
#pragma comment(lib, "LLVMMCDisassembler.lib")
#pragma comment(lib, "LLVMMCJIT.lib")
#pragma comment(lib, "LLVMMCParser.lib")
#pragma comment(lib, "LLVMObjCARCOpts.lib")
#pragma comment(lib, "LLVMObject.lib")
#pragma comment(lib, "LLVMOption.lib")
#pragma comment(lib, "LLVMRuntimeDyld.lib")
#pragma comment(lib, "LLVMScalarOpts.lib")
#pragma comment(lib, "LLVMSelectionDAG.lib")
#pragma comment(lib, "LLVMSupport.lib")
#pragma comment(lib, "LLVMTableGen.lib")
#pragma comment(lib, "LLVMTarget.lib")
#pragma comment(lib, "LLVMTransformUtils.lib")
#pragma comment(lib, "LLVMVectorize.lib")
#pragma comment(lib, "profile_rt.lib")

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")

#pragma comment(lib, "LLVMX86AsmParser.lib")
#pragma comment(lib, "LLVMX86AsmPrinter.lib")
#pragma comment(lib, "LLVMX86CodeGen.lib")
#pragma comment(lib, "LLVMX86Desc.lib")
#pragma comment(lib, "LLVMX86Disassembler.lib")
#pragma comment(lib, "LLVMX86Info.lib")
#pragma comment(lib, "LLVMX86Utils.lib")

#pragma comment(lib, "LLVMARMAsmParser.lib")
#pragma comment(lib, "LLVMARMAsmPrinter.lib")
#pragma comment(lib, "LLVMARMCodeGen.lib")
#pragma comment(lib, "LLVMARMDesc.lib")
#pragma comment(lib, "LLVMARMDisassembler.lib")
#pragma comment(lib, "LLVMARMInfo.lib")

#endif

struct SQObjectPtr;
struct SQFunctionProto;
struct SQVM;
struct SQInstruction;

struct CallingContext
{
	SQVM* VMPtr;
	SQFunctionProto* FuncProtoPtr;
	SQInteger* TrapsPtr;
	SQObjectPtr* OutresPtr;
	SQInteger Suspend;
	SQInteger ReturnValue;
};

struct sqrt_exception
{
	sqrt_exception()
	{
		scprintf(_SC("exception_hook"));
	}
};

typedef bool (*sq_jit_func_type)(CallingContext*);

struct SqJitEngine
{
private:
	SqJitEngine(const SqJitEngine&);
	SqJitEngine& operator=(const SqJitEngine&);

	llvm::LLVMContext& m_llvmContext;
	llvm::Module m_llvmModule;
	llvm::IRBuilder<> m_llvmBuilder;
	llvm::FunctionPassManager m_llvmFuncPassMgr;
	llvm::ExecutionEngine* m_llvmExecEngine;

	llvm::Type* m_SQIntegerTy;
	llvm::PointerType* m_VoidPointerTy;	//A dummy type used for all sq structures.
	size_t m_FuncCounter;

	std::vector<llvm::Function*> m_InlineFunctions;
	
	enum INLINED_FUNCTIONS
	{
		INL_GET = 0,
		INL_SET,
		INL_ADD,
		INL_SUB,
		INL_MUL,
		INL_DIV,
		INL_MOD,
		INL_OP_CALL,
		INL_NEWSLOT,
		INL_NEWSLOTA,
		INL_DELETESLOT,
		INL_CLASS_OP,
		INL_TABLE_CREATE,
		INL_ARRAY_CREATE,
		INL_CLOSURE_OP,
		INL_RETURN,
		INL_OBJECT_NULL,
		INL_OBJ_PTR_ASSIGN,
		INL_INSTRUCTION_EXEC_HOOK,
		INL_LINE_HOOK,
		INL_NUM_FUCNTIONS,
	};

public:

	enum { MAX_FUNC_NAME = 256 };

public:

	SqJitEngine();
	llvm::LLVMContext& Context() { return m_llvmContext; }
	llvm::Module& Module() { return m_llvmModule; }
	llvm::ExecutionEngine* GetExecEngine() { return m_llvmExecEngine; }
	llvm::Type* GetSQPtrType() const { return m_VoidPointerTy; }

	llvm::Function* BuildFunction(const SQObjectPtr& name, SQFunctionProto* sqfuncproto);

private:
	void EmitInstructions(llvm::Function* f, SQFunctionProto* sqfuncproto);

	llvm::Value* OP_EMIT_MEMBER_ACCESSING(llvm::Value* srcObject, size_t memberOffsetInBytes, llvm::Type* dstType = NULL);
	llvm::Value* OP_EMIT_MEMBER_PTR_ACCESSING(llvm::Value* srcObject, size_t memberOffsetInBytes, llvm::Type* dstType = NULL);
	llvm::Value* OP_EMIT_ARRAY_INDEXING(llvm::Value* arrPtr, llvm::Value* index, size_t elementSize);
	void EmitSQObjectAssignment(llvm::Value* srcObjectPtr, llvm::Value* dstObjectPtr);

	llvm::Value* OP_STK(llvm::Value* vm, llvm::Value* stkBase, llvm::Value* a);
	llvm::Value* OP_TARGET(llvm::Value* vm, llvm::Value* stkBase, const SQInstruction& instr);
	void OP_SQOBJASSIGN(llvm::Value* src, llvm::Value* dst);
	void OP_SQOBJFROMINT(llvm::Value* srcInt, llvm::Value* dst);
	void OP_SQOBJFROMFLOAT(llvm::Value* srcFloat, llvm::Value* dst);
	void OP_SQOBJFROMUSERPOINTER(llvm::Value* srcUP, llvm::Value* dst);
	void OP_CALLFUNCTION(const std::string& name, llvm::Value* vmptr, llvm::Value* cloptr);
	void OP_SQOBJSWAP(llvm::Value* left, llvm::Value* right);

private:
	//TODO: Implement most of these functions in LLVM
	static bool SQ_LLVM_OP_CALL_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_GET_PROXY(CallingContext* callingctx, SQObjectPtr* pObj, SQObjectPtr* pKey, bool bRaw, SQInteger selfidx);
	static bool SQ_LLVM_RETURN_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1);
	static void SQ_LLVM_OBJECT_NULL_PROXY(CallingContext* callingctx, SQObjectPtr* pObj);
	static void SQ_LLVM_CLASS_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2);
	//	INL_TABLE_CREATE,
	//	INL_ARRAY_CREATE,
	static void SQ_LLVM_CLOSURE_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1);
	static void SQ_LLVM_NEWSLOTA(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_NEWSLOT(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_OBJECTPTR_ASSIGN(SQObjectPtr* src, SQObjectPtr* dst);

	static void SQ_LLVM_INSTRUCTION_EXEC_HOOK(CallingContext* callingctx, SQInteger nip);
	static void SQ_LLVM_LINE_HOOK(CallingContext* callingctx, SQInteger iarg1);

	static void SQ_LLVM_ADD(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
	static void SQ_LLVM_SUB(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
	static void SQ_LLVM_MUL(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
	static void SQ_LLVM_DIV(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
	static void SQ_LLVM_MOD(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
};


class llvm_sq_ostream : public llvm::raw_ostream
{
public:
	llvm_sq_ostream()
	{
	}

	virtual ~llvm_sq_ostream()
	{
	}
private:
	virtual void write_impl(const char *Ptr, size_t Size)
	{
		static std::vector<SQChar> buf(128);
		if(buf.size() <= Size)
		{
			buf.resize(std::max(Size, buf.size()) * 2);
		}
		std::copy(Ptr, Ptr + Size, buf.begin());
		buf[Size] = _SC('\0');
		scprintf(_SC("%s"), &buf[0]);
	}

	/// current_pos - Return the current position within the stream, not
	/// counting the bytes currently in the buffer.
	virtual uint64_t current_pos() const
	{
		return 0;
	}
};

#endif//SQ_JIT_LLVM

#endif//__SQ_LLVM_H__