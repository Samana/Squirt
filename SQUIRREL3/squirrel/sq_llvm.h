#ifndef __SQ_LLVM_H__
#define __SQ_LLVM_H__

#ifdef SQ_JIT_LLVM

#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IRBuilder.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DataLayout.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/TargetSelect.h"

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
		INL_OP_CALL,
		INL_NEWSLOT,
		INL_NEWSLOTA,
		INL_DELETESLOT,
		INL_CLASS_OP,
		INL_TABLE_CREATE,
		INL_ARRAY_CREATE,
		INL_CLOSURE_OP,
		INL_RETURN,
		INL_OBJ_PTR_ASSIGN,
		INL_INSTRUCTION_EXEC_HOOK,
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
	llvm::Value* OP_EMIT_MEMBER_PTR_ACCESSING(llvm::Value* srcObject, size_t memberOffsetInBytes);
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
	static bool SQ_LLVM_OP_CALL_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_GET_PROXY(CallingContext* callingctx, SQObjectPtr* pObj, SQObjectPtr* pKey, bool bRaw, SQInteger selfidx);
	static bool SQ_LLVM_RETURN_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1);
	static void SQ_LLVM_CLASS_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2);
	//	INL_TABLE_CREATE,
	//	INL_ARRAY_CREATE,
	static void SQ_LLVM_CLOSURE_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1);
	static void SQ_LLVM_NEWSLOTA(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_NEWSLOT(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3);
	static void SQ_LLVM_OBJECTPTR_ASSIGN(SQObjectPtr* src, SQObjectPtr* dst);

	static void SQ_LLVM_INSTRUCTION_EXEC_HOOK(CallingContext* callingctx, SQInteger nip);

	static void SQ_LLVM_ADD(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2);
};

#endif//SQ_JIT_LLVM

#endif//__SQ_LLVM_H__