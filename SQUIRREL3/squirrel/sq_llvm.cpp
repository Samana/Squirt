#ifdef SQ_JIT_LLVM

#include "sqpcheader.h"
#include <math.h>
#include <stdlib.h>
#include <tchar.h>
#include <algorithm>
#include "sqopcodes.h"
#include "sqvm.h"
#include "squirrel.h"
#include "sq_llvm.h"
#include "sqobject.h"
#include "sqstring.h"
#include "sqopcodes.h"
#include "sqfuncproto.h"
#include "sqclosure.h"

#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

static TargetMachine* g_TM;

SqJitEngine::SqJitEngine()
	: m_llvmContext(llvm::getGlobalContext())
	, m_llvmModule("squirt", m_llvmContext)
	, m_llvmBuilder(m_llvmContext)
	, m_llvmFuncPassMgr(&m_llvmModule)
	, m_VoidPointerTy(NULL)
	, m_SQIntegerTy(NULL)
	, m_FuncCounter(0)
	, m_InlineFunctions(INL_NUM_FUCNTIONS, NULL)
{
	llvm::InitializeNativeTarget();

	EngineBuilder builder(&m_llvmModule);

	TargetMachine* tm = builder.selectTarget();
	tm->Options.PrintMachineCode = 1;
	m_llvmExecEngine = builder.create();

	g_TM = tm;

#ifdef _SQ64
	m_VoidPointerTy = Type::getInt64PtrTy(m_llvmContext);	//StructType::create(m_llvmContext, "SQPointer");
	m_SQIntegerTy = Type::getInt64Ty(m_llvmContext);
#else
	m_VoidPointerTy = Type::getInt32PtrTy(m_llvmContext);	
	m_SQIntegerTy = Type::getInt32Ty(m_llvmContext);
#endif

	std::vector<Type*> argTypes;
	FunctionType* funcType = NULL;
	llvm::Function* gloPtr;

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_llvmBuilder.getTrue()->getType());
		argTypes.push_back(m_SQIntegerTy);
		//CallingContext* callingctx, SQObjectPtr* pObj, SQObjectPtr* pKey, bool bRaw, SQInteger selfidx
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_GET_PROXY", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_GET_PROXY);
		m_InlineFunctions[INL_GET] = m_llvmModule.getFunction("SQ_LLVM_GET_PROXY");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getTrue()->getType(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_OP_CALL_PROXY", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_OP_CALL_PROXY);
		m_InlineFunctions[INL_OP_CALL] = m_llvmModule.getFunction("SQ_LLVM_OP_CALL_PROXY");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getTrue()->getType(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_RETURN_PROXY", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_RETURN_PROXY);
		m_InlineFunctions[INL_RETURN] = m_llvmModule.getFunction("SQ_LLVM_RETURN_PROXY");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_OBJECT_NULL_PROXY", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_OBJECT_NULL_PROXY);
		m_InlineFunctions[INL_OBJECT_NULL] = m_llvmModule.getFunction("SQ_LLVM_OBJECT_NULL_PROXY");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_CLASS_OP", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_CLASS_OP);
		m_InlineFunctions[INL_CLASS_OP] = m_llvmModule.getFunction("SQ_LLVM_CLASS_OP");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_CLOSURE_OP", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_CLOSURE_OP);
		m_InlineFunctions[INL_CLOSURE_OP] = m_llvmModule.getFunction("SQ_LLVM_CLOSURE_OP");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_NEWSLOTA", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_NEWSLOTA);
		m_InlineFunctions[INL_NEWSLOTA] = m_llvmModule.getFunction("SQ_LLVM_NEWSLOTA");
	}

	{
		//argTypes.clear();
		//argTypes.push_back(m_VoidPointerTy);
		//argTypes.push_back(m_SQIntegerTy);
		//argTypes.push_back(m_SQIntegerTy);
		//argTypes.push_back(m_SQIntegerTy);
		//argTypes.push_back(m_SQIntegerTy);
		//funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_NEWSLOT", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_NEWSLOT);
		m_InlineFunctions[INL_NEWSLOT] = m_llvmModule.getFunction("SQ_LLVM_NEWSLOT");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_OBJECTPTR_ASSIGN", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_OBJECTPTR_ASSIGN);
		m_InlineFunctions[INL_OBJ_PTR_ASSIGN] = m_llvmModule.getFunction("SQ_LLVM_OBJECTPTR_ASSIGN");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_INSTRUCTION_EXEC_HOOK", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_INSTRUCTION_EXEC_HOOK);
		m_InlineFunctions[INL_INSTRUCTION_EXEC_HOOK] = m_llvmModule.getFunction("SQ_LLVM_INSTRUCTION_EXEC_HOOK");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_ADD", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_ADD);
		m_InlineFunctions[INL_ADD] = m_llvmModule.getFunction("SQ_LLVM_ADD");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_SUB", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_SUB);
		m_InlineFunctions[INL_SUB] = m_llvmModule.getFunction("SQ_LLVM_SUB");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_MUL", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_MUL);
		m_InlineFunctions[INL_MUL] = m_llvmModule.getFunction("SQ_LLVM_MUL");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_DIV", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_DIV);
		m_InlineFunctions[INL_DIV] = m_llvmModule.getFunction("SQ_LLVM_DIV");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_VoidPointerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_MOD", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_MOD);
		m_InlineFunctions[INL_MOD] = m_llvmModule.getFunction("SQ_LLVM_MOD");
	}

	{
		argTypes.clear();
		argTypes.push_back(m_VoidPointerTy);
		argTypes.push_back(m_SQIntegerTy);
		funcType = FunctionType::get(m_llvmBuilder.getVoidTy(), argTypes, false);
		gloPtr = llvm::Function::Create(funcType, GlobalValue::ExternalLinkage, "SQ_LLVM_LINE_HOOK", &m_llvmModule);
		m_llvmExecEngine->addGlobalMapping(gloPtr, &SQ_LLVM_LINE_HOOK);
		m_InlineFunctions[INL_LINE_HOOK] = m_llvmModule.getFunction("SQ_LLVM_LINE_HOOK");
	}


	m_llvmFuncPassMgr.add(new DataLayout(*m_llvmExecEngine->getDataLayout()));
	m_llvmFuncPassMgr.add(createBasicAliasAnalysisPass());
	m_llvmFuncPassMgr.add(createInstructionCombiningPass());
	m_llvmFuncPassMgr.add(createInstructionSimplifierPass());
	m_llvmFuncPassMgr.add(createReassociatePass());
	m_llvmFuncPassMgr.add(createGVNPass());
	m_llvmFuncPassMgr.add(createCFGSimplificationPass());
	m_llvmFuncPassMgr.doInitialization();
}

Function* SqJitEngine::BuildFunction(
	const SQObjectPtr& name,
	SQFunctionProto* sqfuncproto)
{
	//
	//Prepare argument types list and create FunctionType.
	//
	std::vector<Type*> arrArgTypes(1);
	arrArgTypes[0] = PointerType::get(m_VoidPointerTy, 0);		//Pointer to VM
	FunctionType* ft = FunctionType::get(Type::getVoidTy(m_llvmContext), arrArgTypes, false);

	//
	//Give a proper name, create Function.
	//
	char nameBuf[MAX_FUNC_NAME];
	if(sq_isstring(name))
	{
		SQChar* text = _string(name)->_val;
#ifdef SQUNICODE 
		SQChar wbuf[MAX_FUNC_NAME];
		memset(wbuf, 0, sizeof(wbuf));
		_tcsncpy(wbuf, text, std::min<size_t>(MAX_FUNC_NAME, _string(name)->_len));
		wcstombs(nameBuf, wbuf, MAX_FUNC_NAME);
#else
		strncpy(nameBuf, text, std::min<size_t>(MAX_FUNC_NAME, _string(name)->_len));
#endif
	}
	else
	{
		sprintf(nameBuf, "_unnamed_func_%x", m_FuncCounter++);
	}
	Twine strName(nameBuf);
	Function* f = Function::Create(ft, GlobalValue::ExternalLinkage, strName, &m_llvmModule);
	
	EmitInstructions(f, sqfuncproto);

	bool bBroken = llvm::verifyFunction(*f);
	if(bBroken)
	{
		//FIXME: Leaks?
		return NULL;
	}
	else
	{
		llvm_sq_ostream ostm;
		scprintf(_SC("\n=============== BEFORE OPTIMIZATION ==============\n"));
		f->print(ostm);
		m_llvmFuncPassMgr.run(*f);
		scprintf(_SC("\n=============== AFTER OPTIMIZATION ==============\n"));
		f->print(ostm);

		/*
		std::string err;
		llvm_sq_ostream m3log;
		formatted_raw_ostream fm3log(m3log);
		//TD = new TargetData(m_llvmModule.getDataLayout());
		PassManager MPasses;
		MPasses.add(new llvm::DataLayout(*m_llvmExecEngine->getDataLayout()));
		g_TM->addPassesToEmitFile(MPasses, fm3log, TargetMachine::CGFT_AssemblyFile);
		MPasses.run(m_llvmModule);
		*/

		return f;
	}
}

#define _GUARD(x) ( x )
#define arg0 (_i_._arg0)
#define sarg0 ((SQInteger)*((signed char *)&_i_._arg0))
#define arg1 (_i_._arg1)
#define sarg1 (*((SQInt32 *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((SQInteger)*((signed char *)&_i_._arg3))

#ifdef _SQ64
#define INT_TO_VALUE( x ) (m_llvmBuilder.getInt64(x))
#else
#define INT_TO_VALUE( x ) (m_llvmBuilder.getInt32(x))
#endif

#define JIT_STK(a) vm->_stack._vals[vm->_stackbase+(a)]
#define JIT_TARGET vm->_stack._vals[vm->_stackbase+arg0]

bool SqJitEngine::SQ_LLVM_OP_CALL_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3)
{
	unsigned char arg0uchar = (unsigned char)iarg0;
	SQInteger isarg0 = ((SQInteger)*((signed char *)&arg0uchar));

	SQVM* vm = callingctx->VMPtr;
	SQVM::CallInfo* ci = vm->ci;
	SQObjectPtr clo = vm->_stack._vals[vm->_stackbase + iarg1];
	switch (sqobjtype(clo)) {
	case OT_CLOSURE:
		{
			if(!vm->StartCall(_closure(clo), iarg0, iarg3, vm->_stackbase + iarg2, false))
			{
				throw sqrt_exception();
			}

			sq_jit_func_type funcPtr = (sq_jit_func_type)_closure(clo)->_sharedstate->GetJitEngine().GetExecEngine()->getPointerToFunction(
				_closure(clo)->_function->_jitfunction);
			CallingContext context =
			{
				callingctx->VMPtr,
				_closure(clo)->_function,
				callingctx->TrapsPtr,
				callingctx->OutresPtr,
				false,
				true,
			};

			funcPtr(&context);

			if(context.Suspend)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
	case OT_NATIVECLOSURE:
		{
			bool suspend;
			if(!vm->CallNative(_nativeclosure(clo), iarg3, vm->_stackbase + iarg2, clo,suspend))
			{
				throw sqrt_exception();
			}
			if(suspend){
				vm->_suspended = SQTrue;
				vm->_suspended_target = isarg0; //sarg0;
				vm->_suspended_root = ci->_root;
				vm->_suspended_traps = *callingctx->TrapsPtr; //traps;
				*callingctx->OutresPtr = clo; //outres = clo;
				callingctx->Suspend = true;
				callingctx->ReturnValue = true;
				return true;
			}
			if(isarg0 != -1) {
				JIT_STK(iarg0) = clo;
			}
		}
		return false;
	case OT_CLASS:
		{
			SQObjectPtr inst;
			_GUARD(vm->CreateClassInstance(_class(clo),inst,clo));
			if(isarg0 != -1) {
				JIT_STK(iarg0) = inst;
			}
			SQInteger stkbase;
			switch(sqobjtype(clo)) {
				case OT_CLOSURE:
					stkbase = vm->_stackbase + iarg2;
					vm->_stack._vals[stkbase] = inst;
					if(!vm->StartCall(_closure(clo), -1, iarg3, stkbase, false))
					{
						throw sqrt_exception();
					}
					break;
				case OT_NATIVECLOSURE:
					bool suspend;
					stkbase = vm->_stackbase + iarg2;
					vm->_stack._vals[stkbase] = inst;
					if(!vm->CallNative(_nativeclosure(clo), iarg3, stkbase, clo,suspend))
					{
						throw sqrt_exception();
					}
					break;
				default: break; //shutup GCC 4.x
			}
		}
		break;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		{
			SQObjectPtr closure;
			if(_delegable(clo)->_delegate && _delegable(clo)->GetMetaMethod(vm, MT_CALL, closure)) 
			{
				vm->Push(clo);
				for (SQInteger i = 0; i < iarg3; i++)
				{
					vm->Push(JIT_STK(iarg2 + i));
				}
				if(!vm->CallMetaMethod(closure, MT_CALL, iarg3+1, clo)) throw sqrt_exception();
				if(isarg0 != -1) {
					JIT_STK(iarg0) = clo;
				}
				break;
			}
		}
	default:
		vm->Raise_Error(_SC("attempt to call '%s'"), GetTypeName(clo));
		throw sqrt_exception();
	}
	return false;
}

void SqJitEngine::SQ_LLVM_GET_PROXY(CallingContext* callingctx, SQObjectPtr* pObj, SQObjectPtr* pKey, bool bRaw, SQInteger selfidx)
{
	bool bRet = callingctx->VMPtr->Get(*pObj, *pKey, callingctx->VMPtr->temp_reg, bRaw, selfidx);
	callingctx->ReturnValue = bRet ? 1 : 0;
}

bool SqJitEngine::SQ_LLVM_RETURN_PROXY(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1)
{
	SQVM* vm = callingctx->VMPtr;

	if((vm->ci)->_generator)
	{
		(vm->ci)->_generator->Kill();
	}
	if(vm->Return(iarg0, iarg1, vm->temp_reg)){
		assert(*callingctx->TrapsPtr==0);
		_Swap(*callingctx->OutresPtr,vm->temp_reg);
		callingctx->ReturnValue = 1;
		return true;
	}
	return false;
}

void SqJitEngine::SQ_LLVM_OBJECT_NULL_PROXY(CallingContext* callingctx, SQObjectPtr* pObj)
{
	SQVM* vm = callingctx->VMPtr;
	SQObjectType tOldType = pObj->_type;
	SQObjectValue unOldVal = pObj->_unVal;
	pObj->_type = OT_NULL;
	pObj->_unVal.raw = (SQRawObjectVal)NULL;
	__Release(tOldType ,unOldVal);
}

void SqJitEngine::SQ_LLVM_CLASS_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2)
{
	SQVM* vm = callingctx->VMPtr;
	if(!vm->CLASS_OP(vm->_stack._vals[vm->_stackbase + iarg0], iarg1, iarg2))
	{
		throw sqrt_exception();
	}
}

void SqJitEngine::SQ_LLVM_CLOSURE_OP(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1)
{
	SQVM* vm = callingctx->VMPtr;
	SQClosure *c = vm->ci->_closure._unVal.pClosure;
	SQFunctionProto *fp = c->_function;
	if(!vm->CLOSURE_OP(vm->_stack._vals[vm->_stackbase + iarg0], fp->_functions[iarg1]._unVal.pFunctionProto))
	{
		throw sqrt_exception();
	}
}

void SqJitEngine::SQ_LLVM_NEWSLOTA(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3)
{
	SQVM* vm = callingctx->VMPtr;

	if(!vm->NewSlotA(
		vm->_stack._vals[vm->_stackbase+iarg1],
		vm->_stack._vals[vm->_stackbase+iarg2],
		vm->_stack._vals[vm->_stackbase+iarg3],
		(iarg0 & NEW_SLOT_ATTRIBUTES_FLAG) ? vm->_stack._vals[vm->_stackbase+(iarg2-1)] : SQObjectPtr(),
		(iarg0 & NEW_SLOT_STATIC_FLAG) ? true : false, false))
	{
		throw sqrt_exception();
	}
}

void SqJitEngine::SQ_LLVM_NEWSLOT(CallingContext* callingctx, SQInteger iarg0, SQInteger iarg1, SQInteger iarg2, SQInteger iarg3)
{
	SQVM* vm = callingctx->VMPtr;

	if(!vm->NewSlot(
		vm->_stack._vals[vm->_stackbase+iarg1],
		vm->_stack._vals[vm->_stackbase+iarg2],
		vm->_stack._vals[vm->_stackbase+iarg3],
		false))
	{
		throw sqrt_exception();
	}

	if(iarg0 != 0xFF)
	{
		vm->_stack._vals[vm->_stackbase + iarg0] = vm->_stack._vals[vm->_stackbase + iarg3];
	}
}

void SqJitEngine::SQ_LLVM_OBJECTPTR_ASSIGN(SQObjectPtr* src, SQObjectPtr* dst)
{
	*dst = *src;
}

void SqJitEngine::SQ_LLVM_INSTRUCTION_EXEC_HOOK(CallingContext* callingctx, SQInteger nip)
{
//	scprintf(_SC("IP = %d, stkbase = %d\n"), nip, callingctx->VMPtr->_stackbase);
}

void SqJitEngine::SQ_LLVM_LINE_HOOK(CallingContext* callingctx, SQInteger iarg1)
{
	if(callingctx->VMPtr->_debughook)
	{
		callingctx->VMPtr->CallDebugHook(_SC('l'), iarg1);
	}
}

void SqJitEngine::SQ_LLVM_ADD(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2)
{
	SQInteger tmask = sqobjtype(*o1)|sqobjtype(*o2);
	switch(tmask) {
		case OT_INTEGER: 
			{
				*dst = _integer(*o1) + _integer(*o2);
			}
			break;
		case (OT_FLOAT|OT_INTEGER): 
		case (OT_FLOAT): 
			{
				*dst = tofloat(*o1) + tofloat(*o2); 
			}
			break;
		default: 
			if(!callingctx->VMPtr->ARITH_OP('+', *dst, *o1, *o2))
			{
			};
			break;
	}
}

void SqJitEngine::SQ_LLVM_SUB(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2)
{
	SQInteger tmask = sqobjtype(*o1)|sqobjtype(*o2);
	switch(tmask) {
		case OT_INTEGER: 
			{
				*dst = _integer(*o1) - _integer(*o2);
			}
			break;
		case (OT_FLOAT|OT_INTEGER): 
		case (OT_FLOAT): 
			{
				*dst = tofloat(*o1) - tofloat(*o2); 
			}
			break;
		default: 
			if(!callingctx->VMPtr->ARITH_OP('-', *dst, *o1, *o2))
			{
			};
			break;
	}
}

void SqJitEngine::SQ_LLVM_MUL(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2)
{
	SQInteger tmask = sqobjtype(*o1)|sqobjtype(*o2);
	switch(tmask) {
		case OT_INTEGER: 
			{
				*dst = _integer(*o1) * _integer(*o2);
			}
			break;
		case (OT_FLOAT|OT_INTEGER): 
		case (OT_FLOAT): 
			{
				*dst = tofloat(*o1) * tofloat(*o2); 
			}
			break;
		default: 
			if(!callingctx->VMPtr->ARITH_OP('*', *dst, *o1, *o2))
			{
			};
			break;
	}
}

void SqJitEngine::SQ_LLVM_DIV(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2)
{
	SQInteger tmask = sqobjtype(*o1)|sqobjtype(*o2);
	switch(tmask) {
		case OT_INTEGER: 
			{
				*dst = _integer(*o1) / _integer(*o2);
			}
			break;
		case (OT_FLOAT|OT_INTEGER): 
		case (OT_FLOAT): 
			{
				*dst = tofloat(*o1) / tofloat(*o2); 
			}
			break;
		default: 
			if(!callingctx->VMPtr->ARITH_OP('/', *dst, *o1, *o2))
			{
			};
			break;
	}
}

void SqJitEngine::SQ_LLVM_MOD(CallingContext* callingctx, SQObjectPtr* dst, SQObjectPtr* o1, SQObjectPtr* o2)
{
	SQInteger tmask = sqobjtype(*o1)|sqobjtype(*o2);
	switch(tmask) {
		case OT_INTEGER: 
			{
				*dst = _integer(*o1) % _integer(*o2);
			}
			break;
		case (OT_FLOAT|OT_INTEGER): 
		case (OT_FLOAT): 
			{
				*dst = SQFloat(fmod(tofloat(*o1), tofloat(*o2))); 
			}
			break;
		default: 
			if(!callingctx->VMPtr->ARITH_OP('%', *dst, *o1, *o2))
			{
			};
			break;
	}
}

#undef STK
#undef TARGET

static inline void GetLineName(char* name, int nip)
{
	sprintf(name, "line_%d", nip);
}

static inline std::string GetBlockName(const char* basic_name, int nip)
{
	char tmp[64];
	std::string ret = basic_name;
	sprintf(tmp, "%d", nip);
	ret += tmp;
	return ret;
}

void SqJitEngine::EmitInstructions(llvm::Function* f, SQFunctionProto* sqfuncproto)
{
	BasicBlock* bb_entry = BasicBlock::Create(m_llvmContext, "entry", f);
	BasicBlock* bb_leave = BasicBlock::Create(m_llvmContext, "leave", f);
	
	m_llvmBuilder.SetInsertPoint(bb_entry);

	Value* val_context = m_llvmBuilder.CreateCast(llvm::Instruction::BitCast, f->arg_begin(), m_VoidPointerTy);
	Value* val_vmptr = OP_EMIT_MEMBER_ACCESSING(val_context, offsetof(CallingContext, VMPtr));
	Value* val_funcprotoptr = OP_EMIT_MEMBER_ACCESSING(val_context, offsetof(CallingContext, FuncProtoPtr));

	Value* val_ssptr = OP_EMIT_MEMBER_ACCESSING(val_vmptr, offsetof(SQVM, _sharedstate));
	Value* val_stkbase = OP_EMIT_MEMBER_ACCESSING(val_vmptr, offsetof(SQVM, _stackbase), m_SQIntegerTy);

	Value* val_literalarr = OP_EMIT_MEMBER_ACCESSING(val_funcprotoptr, offsetof(SQFunctionProto, _literals));
	std::vector<Value*> val_literals(sqfuncproto->_nliterals);
	for(SQInteger i=0; i<sqfuncproto->_nliterals; i++)
	{
		val_literals[i] = OP_EMIT_ARRAY_INDEXING(val_literalarr, INT_TO_VALUE(i), sizeof(SQObjectPtr));
	}

	Value* val_tmp_reg = OP_EMIT_MEMBER_PTR_ACCESSING(val_vmptr, offsetof(SQVM, temp_reg));

	SQInstruction* _ip_ = sqfuncproto->_instructions;

	std::vector<BasicBlock*> jmp_blocks(sqfuncproto->_ninstructions, nullptr);

	for(SQInteger nip = 0; nip < sqfuncproto->_ninstructions; nip++)
	{
		const SQInstruction &_i_ = _ip_[nip];

		switch(_i_.op)
		{
		case _OP_JMP:
			{
				char name[64];
				GetLineName(name, nip);
				jmp_blocks[nip] = BasicBlock::Create(m_llvmContext, name, f);
				continue;
			}
		default:
			continue;
		};
	}

#define NOT_IMPL_YET { assert(false && "NOT IMPL"); }

	for(SQInteger nip = 0; nip < sqfuncproto->_ninstructions; nip++)
	{
		const SQInstruction &_i_ = _ip_[nip];

#ifdef _DEBUG
		{
			std::vector<Value*> args;
			args.push_back(val_context);
			args.push_back(m_llvmBuilder.getInt32(nip));
			m_llvmBuilder.CreateCall(m_InlineFunctions[INL_INSTRUCTION_EXEC_HOOK], args);
		}
#endif
		if(jmp_blocks[nip] != NULL)
		{
			m_llvmBuilder.CreateBr(jmp_blocks[nip]);
		}

		switch(_i_.op)
		{
		case _OP_LINE:
			{
				//if (_debughook) CallDebugHook(_SC('l'),arg1); 
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg1));
				Value* val_func_call = m_InlineFunctions[INL_LINE_HOOK];
				m_llvmBuilder.CreateCall(val_func_call, args, "");
				continue;
			}
		case _OP_LOAD:
			{
				Value* val_src_objptr = val_literals[_i_._arg1];
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);
				//TARGET = ci->_literals[arg1]; 
				continue;
			}
		case _OP_LOADINT: 
			{
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
#ifndef _SQ64
				Value* val_srcint = INT_TO_VALUE((SQInteger)_i_._arg1);
				//TARGET = (SQInteger)arg1; continue;
#else
				Value* val_srcint = m_llvmBuilder.getInt64((SQInteger)(SQUnsignedInteger32)_i_._arg1);
				//TARGET = (SQInteger)((SQUnsignedInteger32)arg1); continue;
#endif
				OP_SQOBJFROMINT(val_srcint, val_target_objptr);
				continue;
			}
		case _OP_LOADFLOAT:
			{
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_srcflt = ConstantFP::get(m_llvmContext, APFloat(*((SQFloat*)&_i_._arg1)));
				OP_SQOBJFROMFLOAT(val_srcflt, val_target_objptr);
				//TARGET = *((SQFloat *)&arg1);
				continue;
			}
		case _OP_DLOAD:
			{
				Value* val_src_objptr = val_literals[_i_._arg1];
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);
				
				val_src_objptr = val_literals[_i_._arg3];
				Value* val_arg2 = INT_TO_VALUE((SQInteger)_i_._arg2);
				val_target_objptr = OP_STK(val_vmptr, val_stkbase, val_arg2);
				OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);

				//TARGET = ci->_literals[arg1]; 
				//STK(arg2) = ci->_literals[arg3];
				continue;
			}
		case _OP_TAILCALL:
			{
				//SQObjectPtr &t = STK(arg1);
				//if (sqobjtype(t) == OT_CLOSURE 
				//	&& (!_closure(t)->_function->_bgenerator)){
				//	SQObjectPtr clo = t;
				//	if(_openouters) CloseOuters(&(_stack._vals[_stackbase]));
				//	for (SQInteger i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
				//	_GUARD(StartCall(_closure(clo), ci->_target, arg3, _stackbase, true));
				//	continue;
				//}
				NOT_IMPL_YET
				continue;
			}
		case _OP_CALL: 
			{
				//Value* clo = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg1));
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));
				Value* val_func_call = m_InlineFunctions[INL_OP_CALL];
				Value* val_if_return = m_llvmBuilder.CreateCall(val_func_call, args, "");
				
				BasicBlock* bb_goon = BasicBlock::Create(m_llvmContext, GetBlockName("goon", nip), f);
				m_llvmBuilder.CreateCondBr(val_if_return, bb_leave, bb_goon);

				m_llvmBuilder.SetInsertPoint(bb_goon);

				continue;
			}
		case _OP_PREPCALL:
			{
				Value* val_key = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg1));
				Value* val_arg2 = INT_TO_VALUE(_i_._arg2);
				Value* val_o = OP_STK(val_vmptr, val_stkbase, val_arg2);

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_o);
				args.push_back(val_key);
				args.push_back(m_llvmBuilder.getFalse());	//False
				args.push_back(val_arg2);

				Value* val_func_call = m_InlineFunctions[INL_GET];
				Value* val_get = m_llvmBuilder.CreateCall(val_func_call, args, "");

				Value* val_stkarg3 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg3));
				OP_SQOBJASSIGN(val_o, val_stkarg3);
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJSWAP(val_target_objptr, val_tmp_reg);

				//SQObjectPtr &key = STK(arg1);
				//SQObjectPtr &o = STK(arg2);
				//if (!Get(o, key, temp_reg,false,arg2)) {
				//	SQ_THROW();
				//}
				//STK(arg3) = o;
				//_Swap(TARGET,temp_reg);
				continue;
			}
		case _OP_PREPCALLK:
			{
				Value* val_key = val_literals[_i_._arg1];
				Value* val_arg2 = INT_TO_VALUE(_i_._arg2);
				Value* val_o = OP_STK(val_vmptr, val_stkbase, val_arg2);

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_o);
				args.push_back(val_key);
				args.push_back(m_llvmBuilder.getFalse());	//False
				args.push_back(val_arg2);

				Value* val_func_call = m_InlineFunctions[INL_GET];
				Value* val_get = m_llvmBuilder.CreateCall(val_func_call, args, "");

				Value* val_stkarg3 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg3));
				OP_SQOBJASSIGN(val_o, val_stkarg3);
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJSWAP(val_target_objptr, val_tmp_reg);

				//SQObjectPtr &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
				//SQObjectPtr &o = STK(arg2);
				//if (!Get(o, key, temp_reg,false,arg2)) {
				//	SQ_THROW();
				//}
				//STK(arg3) = o;
				//_Swap(TARGET,temp_reg);//TARGET = temp_reg;
			}
			continue;
		case _OP_GETK:
			{
				Value* val_o = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg2));
				Value* val_arg2 = INT_TO_VALUE(_i_._arg2);
				Value* val_key = val_literals[_i_._arg1];
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_o);
				args.push_back(val_key);
				args.push_back(m_llvmBuilder.getFalse());
				args.push_back(val_arg2);
				Value* val_func_call = m_llvmModule.getFunction("SQ_LLVM_GET_PROXY");
				Value* val_get = m_llvmBuilder.CreateCall(val_func_call, args, "");

				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJSWAP(val_target_objptr, val_tmp_reg);
				//if (!Get(STK(arg2), ci->_literals[arg1], temp_reg, false,arg2)) { SQ_THROW();}
				//_Swap(TARGET,temp_reg);//TARGET = temp_reg;
			}
			continue;
		case _OP_MOVE:
			{
				Value* val_src_objptr = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg1));
				Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);
				//TARGET = STK(arg1); 
			}
			continue;
		case _OP_NEWSLOT:
			{
				Value* val_func_newslot = m_InlineFunctions[INL_NEWSLOT]; //m_llvmModule.getFunction("SQ_LLVM_NEWSLOT_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_newslot, args, "");

				//std::vector<Value*> args;
				//args.push_back(val_context);
				//args.push_back(OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1)));
				//args.push_back(OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2)));
				//args.push_back(OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg3)));
				//args.push_back(m_llvmBuilder.getFalse());
				//m_llvmBuilder.CreateCall(val_func_newslot, args, "");
				//if(arg0 != 0xFF)
				//{
				//	Value* val_src_objptr = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg3));
				//	Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
				//	OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);
				//}

				//_GUARD(NewSlot(STK(arg1), STK(arg2), STK(arg3),false));
				//if(arg0 != 0xFF) TARGET = STK(arg3);
			}
			continue;
		case _OP_DELETE:
			{
				Value* val_func_newslot = m_llvmModule.getFunction("SQ_LLVM_DELETESLOT_PROXY");
				Value* val_stk1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));
				Value* val_stk2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_stk1);
				args.push_back(val_stk2);
				m_llvmBuilder.CreateCall(val_func_newslot, args, "");

				//_GUARD(DeleteSlot(STK(arg1), STK(arg2), TARGET)); 
			}
			continue;
		case _OP_SET:
			{
				Value* val_func_set = m_llvmModule.getFunction("SQ_LLVM_SET_PROXY");
				Value* val_o = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));
				Value* val_key = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_value = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg3));

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_o);
				args.push_back(val_key);
				args.push_back(val_value);
				args.push_back(INT_TO_VALUE(arg1));
				m_llvmBuilder.CreateCall(val_func_set, args, "");

				if(arg0 == 0xFF)
				{
					Value* val_src_objptr = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg3));
					Value* val_target_objptr = OP_TARGET(val_vmptr, val_stkbase, _i_);
					OP_SQOBJASSIGN(val_src_objptr, val_target_objptr);
				}

				//if (!Set(STK(arg1), STK(arg2), STK(arg3),arg1)) { SQ_THROW(); }
				//if (arg0 != 0xFF) TARGET = STK(arg3);
			}
			continue;
		case _OP_GET:
			{
				Value* val_func_get = m_llvmModule.getFunction("SQ_LLVM_GET_PROXY");
				Value* val_o = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));
				Value* val_key = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_arg1 = INT_TO_VALUE(arg1);
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_o);
				args.push_back(val_key);
				args.push_back(m_llvmBuilder.getFalse());
				args.push_back(val_arg1);


				//if (!Get(STK(arg1), STK(arg2), temp_reg, false,arg1)) { SQ_THROW(); }
				//_Swap(TARGET,temp_reg);//TARGET = temp_reg;
			}
			continue;
		case _OP_EQ:
			{
				Value* val_func_eq = m_llvmModule.getFunction("SQ_LLVM_EQ_NE_PROXY");
				Value* val_a = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_b = ( arg3 != 0 ) ?
					val_literals[arg1] :
					OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_a);
				args.push_back(val_b);
				args.push_back(m_llvmBuilder.getFalse());
				m_llvmBuilder.CreateCall(val_func_eq, args);

				//#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))
				//bool res;
				//if(!IsEqual(STK(arg2),COND_LITERAL,res,this)) { SQ_THROW(); }
				//TARGET = res?true:false;
			}
			continue;
		case _OP_NE:
			{
				Value* val_func_eq = m_llvmModule.getFunction("SQ_LLVM_EQ_NE_PROXY");
				Value* val_a = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_b = ( arg3 != 0 ) ?
					val_literals[arg1] :
					OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_a);
				args.push_back(val_b);
				args.push_back(m_llvmBuilder.getTrue());
				m_llvmBuilder.CreateCall(val_func_eq, args);

				//bool res;
				//if(!IsEqual(STK(arg2),COND_LITERAL,res,this)) { SQ_THROW(); }
				//TARGET = (!res)?true:false;
			}
			continue;
		case _OP_ADD:
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_add = m_InlineFunctions[INL_ADD]; //m_llvmModule.getFunction("SQ_LLVM_ADD_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_add, args);

				/*
				Value* val_type1 = OP_EMIT_MEMBER_ACCESSING(val_o1, offsetof(SQObjectPtr, _type), m_llvmBuilder.getInt32Ty());
				Value* val_type2 = OP_EMIT_MEMBER_ACCESSING(val_o2, offsetof(SQObjectPtr, _type), m_llvmBuilder.getInt32Ty());
				
				Value* val_mask = m_llvmBuilder.CreateBinOp(llvm::Instruction::Or, val_type1, val_type2);
				
				Value* val_mask_is_int = m_llvmBuilder.CreateICmpEQ(val_mask, INT_TO_VALUE(OT_INTEGER));
				Value* val_mask_is_flt_and_int = m_llvmBuilder.CreateICmpEQ(val_mask, INT_TO_VALUE(OT_FLOAT | OT_INTEGER));
				Value* val_mask_is_flt = m_llvmBuilder.CreateICmpEQ(val_mask, INT_TO_VALUE(OT_FLOAT));
				Value* val_mask_is_flt_or_int = m_llvmBuilder.CreateOr(val_mask_is_flt_and_int, val_mask_is_flt);

				BasicBlock* bb_int = BasicBlock::Create(m_llvmContext);
				BasicBlock* bb_not_int = BasicBlock::Create(m_llvmContext);
				BasicBlock* bb_flt_or_int = BasicBlock::Create(m_llvmContext);
				BasicBlock* bb_none = BasicBlock::Create(m_llvmContext);
				BasicBlock* bb_goon = BasicBlock::Create(m_llvmContext);
				
				m_llvmBuilder.CreateCondBr(val_mask_is_int, bb_int, bb_not_int);

				//if mask == OT_INTEGER
				m_llvmBuilder.SetInsertPoint(bb_int);
				Value* val_int1 = OP_EMIT_MEMBER_ACCESSING(val_o1, offsetof(SQObject, _unVal), m_llvmBuilder.getInt32Ty());	//FIXME : 64Bit
				Value* val_int2 = OP_EMIT_MEMBER_ACCESSING(val_o2, offsetof(SQObject, _unVal), m_llvmBuilder.getInt32Ty());	//FIXME : 64Bit
				Value* val_add = m_llvmBuilder.CreateAdd(val_int1, val_int2);
				Value* val_target_int_ptr = OP_EMIT_MEMBER_PTR_ACCESSING(val_target, offsetof(SQObject, _unVal), m_llvmBuilder.getInt32Ty());
				m_llvmBuilder.CreateStore(val_add, val_target_int_ptr);
				m_llvmBuilder.CreateBr(bb_goon);

				//not OT_INTEGER
				m_llvmBuilder.SetInsertPoint(bb_not_int);
				m_llvmBuilder.CreateCondBr(val_mask_is_flt_or_int, bb_flt_or_int, bb_none);

				//flt
				m_llvmBuilder.SetInsertPoint(bb_flt_or_int);
				*/


				//#define _ARITH_(op,trg,o1,o2) \
				//{ \
				//	SQInteger tmask = sqobjtype(o1)|sqobjtype(o2); \
				//	switch(tmask) { \
				//		case OT_INTEGER: trg = _integer(o1) op _integer(o2);break; \
				//		case (OT_FLOAT|OT_INTEGER): \
				//		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
				//		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
				//	} \
				//}
				//_ARITH_(+,TARGET,STK(arg2),STK(arg1)); 
			}
			continue;
		case _OP_SUB:
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_sub = m_InlineFunctions[INL_SUB]; //m_llvmModule.getFunction("SQ_LLVM_SUB_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_sub, args);

				//_ARITH_(-,TARGET,STK(arg2),STK(arg1));
			}
			continue;
		case _OP_MUL: 
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_mul = m_InlineFunctions[INL_MUL]; //m_llvmModule.getFunction("SQ_LLVM_MUL_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_mul, args);

				//_ARITH_(*,TARGET,STK(arg2),STK(arg1));
			}
			continue;
		case _OP_DIV:
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_div = m_InlineFunctions[INL_DIV]; //m_llvmModule.getFunction("SQ_LLVM_DIV_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_div, args);

				//_ARITH_NOZERO(/,TARGET,STK(arg2),STK(arg1),_SC("division by zero")); 
			}
			continue;
		case _OP_MOD:
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_mod = m_InlineFunctions[INL_MOD]; //m_llvmModule.getFunction("SQ_LLVM_MOD_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_mod, args);

				//ARITH_OP('%',TARGET,STK(arg2),STK(arg1)); 
			}
			continue;
		case _OP_BITW:
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_op = INT_TO_VALUE(arg3);
				Value* val_o1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				Value* val_o2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				Value* val_func_bwop = m_llvmModule.getFunction("SQ_LLVM_BWOP_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_op);
				args.push_back(val_target);
				args.push_back(val_o1);
				args.push_back(val_o2);
				m_llvmBuilder.CreateCall(val_func_bwop, args);

				//_GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1)));
			}
			continue;
		case _OP_RETURN:
			{
				Value* val_func_return = m_InlineFunctions[INL_RETURN];
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));

				Value* val_if_return = m_llvmBuilder.CreateCall(val_func_return, args);
								
				BasicBlock* bb_goon = BasicBlock::Create(m_llvmContext, GetBlockName("goon", nip), f);
				m_llvmBuilder.CreateCondBr(val_if_return, bb_leave, bb_goon);

				m_llvmBuilder.SetInsertPoint(bb_goon);

				//if((ci)->_generator) {
				//	(ci)->_generator->Kill();
				//}
				//if(Return(arg0, arg1, temp_reg)){
				//	assert(traps==0);
				//	//outres = temp_reg;
				//	_Swap(outres,temp_reg);
				//	return true;
				//}
			}
			continue;
		case _OP_LOADNULLS:
			{
				Value* val_func_null = m_llvmModule.getFunction("SQ_LLVM_OBJECT_NULL_PROXY");
				for(SQInt32 n=0; n < arg1; n++)
				{
					Value* val_o = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg0 + n));
					std::vector<Value*> args;
					args.push_back(val_context);
					args.push_back(val_o);
					m_llvmBuilder.CreateCall(val_func_null, args);
				}
				//for(SQInt32 n=0; n < arg1; n++)STK(arg0+n).Null(); 
			}
			continue;
		case _OP_LOADROOT:
			{
				Value* val_roottable = OP_EMIT_MEMBER_PTR_ACCESSING(val_vmptr, offsetof(SQVM, _roottable));
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJASSIGN(val_roottable, val_target);

				//TARGET = _roottable; 
			}
			continue;
		case _OP_LOADBOOL: 
			{
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				Value* val_func_obj_assign_bool = m_llvmModule.getFunction("");
				std::vector<Value*> args;
				args.push_back(val_target);
				args.push_back(arg1 ? m_llvmBuilder.getTrue() : m_llvmBuilder.getFalse());
				m_llvmBuilder.CreateCall(val_func_obj_assign_bool, args);

				//TARGET = arg1?true:false;
			}
			continue;
		case _OP_DMOVE:
			{
				Value* val_src_objptr1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg1));
				Value* val_target_objptr1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg0));
				OP_SQOBJASSIGN(val_src_objptr1, val_target_objptr1);
				Value* val_src_objptr2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg3));
				Value* val_target_objptr2 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(_i_._arg2));
				OP_SQOBJASSIGN(val_src_objptr2, val_target_objptr2);

				//STK(arg0) = STK(arg1);
				//STK(arg2) = STK(arg3); 
			}
			continue;
		case _OP_JMP:
			{
				SQInteger jmpip = nip + sarg1;
				assert(jmp_blocks[jmpip] != nullptr);
				m_llvmBuilder.CreateBr(jmp_blocks[jmpip]);
				
				//ci->_ip += (sarg1); 
			}
			continue;
		//case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
		case _OP_JCMP: 
			{
				Value* val_func_isfalse = m_llvmModule.getFunction("SQ_LLVM_ISFALSE_PROXY");
				Value* val_func_cmpop = m_llvmModule.getFunction("SQ_LLVM_CMPOP_PROXY");

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg3));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg0));
				m_llvmBuilder.CreateCall(val_func_cmpop, args);

				args.clear();
				args.push_back(val_context);
				args.push_back(val_tmp_reg);
				Value* val_isfalse = m_llvmBuilder.CreateCall(val_func_isfalse, args);

				SQInteger jmpip = nip + sarg1;
				assert(jmp_blocks[jmpip] != nullptr);

				BasicBlock* bb_goon = BasicBlock::Create(m_llvmContext, "goon", f);	//FIXME: name
				m_llvmBuilder.CreateCondBr(val_isfalse, jmp_blocks[jmpip], bb_goon); 

				//_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg0),temp_reg));
				//if(IsFalse(temp_reg)) ci->_ip+=(sarg1);
			}
			continue;
		case _OP_JZ:
			{
				Value* val_func_isfalse = m_llvmModule.getFunction("SQ_LLVM_ISFALSE_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_tmp_reg);
				Value* val_isfalse = m_llvmBuilder.CreateCall(val_func_isfalse, args);

				SQInteger jmpip = nip + sarg1;
				assert(jmp_blocks[jmpip] != nullptr);

				BasicBlock* bb_goon = BasicBlock::Create(m_llvmContext, GetBlockName("goon", nip).c_str(), f);

				m_llvmBuilder.CreateCondBr(val_isfalse, jmp_blocks[jmpip], bb_goon);

				//if(IsFalse(STK(arg0))) ci->_ip+=(sarg1); 
			}
			continue;
		case _OP_GETOUTER:
			{
				Value* val_ciptr = OP_EMIT_MEMBER_ACCESSING(val_vmptr, offsetof(SQVM, ci), m_VoidPointerTy);
				Value* val_cur_cls = OP_EMIT_MEMBER_ACCESSING(val_ciptr, offsetof(SQVM::CallInfo, _closure), m_VoidPointerTy);
				Value* val_outer_arr = OP_EMIT_MEMBER_ACCESSING(val_cur_cls, offsetof(SQClosure, _outervalues), m_VoidPointerTy);
				Value* val_outer_value = OP_EMIT_ARRAY_INDEXING(val_outer_arr, INT_TO_VALUE(arg1), sizeof(SQObjectPtr));
				Value* val_otr = OP_EMIT_MEMBER_ACCESSING(val_outer_value, offsetof(SQObjectPtr, _unVal), m_VoidPointerTy);
				Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
				OP_SQOBJASSIGN(val_otr, val_target);

				//SQClosure *cur_cls = _closure(ci->_closure);
				//SQOuter *otr = _outer(cur_cls->_outervalues[arg1]);
				//TARGET = *(otr->_valptr);
			}
			continue;
		case _OP_SETOUTER:
			{
				Value* val_ciptr = OP_EMIT_MEMBER_ACCESSING(val_vmptr, offsetof(SQVM, ci), m_VoidPointerTy);
				Value* val_cur_cls = OP_EMIT_MEMBER_ACCESSING(val_ciptr, offsetof(SQVM::CallInfo, _closure), m_VoidPointerTy);
				Value* val_outer_arr = OP_EMIT_MEMBER_ACCESSING(val_cur_cls, offsetof(SQClosure, _outervalues), m_VoidPointerTy);
				Value* val_outer_value = OP_EMIT_ARRAY_INDEXING(val_outer_arr, INT_TO_VALUE(arg1), sizeof(SQObjectPtr));
				Value* val_otr = OP_EMIT_MEMBER_ACCESSING(val_outer_value, offsetof(SQObjectPtr, _unVal), m_VoidPointerTy);
				Value* val_src = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg2));
				OP_SQOBJASSIGN(val_src, val_otr);

				if(arg0 != 0xFF)
				{
					Value* val_target = OP_TARGET(val_vmptr, val_stkbase, _i_);
					OP_SQOBJASSIGN(val_src, val_target);
				}

				//SQClosure *cur_cls = _closure(ci->_closure);
				//SQOuter   *otr = _outer(cur_cls->_outervalues[arg1]);
				//*(otr->_valptr) = STK(arg2);
				//if(arg0 != 0xFF) {
				//	TARGET = STK(arg2);
				//}
			}
			continue;
		case _OP_NEWOBJ: 
			{
				switch(arg3)
				{
				case NOT_TABLE:
					{
						Value* val_func_table_create = m_InlineFunctions[INL_TABLE_CREATE]; //m_llvmModule.getFunction("SQ_LLVM_TABLE_CREATE");
						std::vector<Value*> args;
						args.push_back(val_context);
						args.push_back(INT_TO_VALUE(arg1));
						args.push_back(INT_TO_VALUE(arg0));

						//Assign to target in the function
						Value* val_ret = m_llvmBuilder.CreateCall(val_func_table_create, args);
					}
					break;
				case NOT_ARRAY:
					{
						Value* val_func_array_create = m_InlineFunctions[INL_ARRAY_CREATE]; //m_llvmModule.getFunction("SQ_LLVM_ARRAY_CREATE");
						std::vector<Value*> args;
						args.push_back(val_context);
						args.push_back(INT_TO_VALUE(arg1));
						args.push_back(INT_TO_VALUE(arg0));

						Value* val_ret = m_llvmBuilder.CreateCall(val_func_array_create, args);
					}
					break;
				case NOT_CLASS:
					{
						Value* val_func_array_create = m_InlineFunctions[INL_CLASS_OP]; //m_llvmModule.getFunction("SQ_LLVM_CLASS_OP_CREATE");
						std::vector<Value*> args;
						args.push_back(val_context);
						args.push_back(INT_TO_VALUE(arg0));
						args.push_back(INT_TO_VALUE(arg1));
						args.push_back(INT_TO_VALUE(arg2));

						Value* val_ret = m_llvmBuilder.CreateCall(val_func_array_create, args);
					}
					break;
				default:
					assert(0);
					break;
				}
				//switch(arg3) {
				//	case NOT_TABLE: TARGET = SQTable::Create(_ss(this), arg1); continue;
				//	case NOT_ARRAY: TARGET = SQArray::Create(_ss(this), 0); _array(TARGET)->Reserve(arg1); continue;
				//	case NOT_CLASS: _GUARD(CLASS_OP(TARGET,arg1,arg2)); continue;
				//	default: assert(0); continue;
				//}
			}
			continue;
		case _OP_APPENDARRAY: 
			{
				Value* val_func_append_arr = m_llvmModule.getFunction("SQ_LLVM_APPEND_ARRAY_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));

				m_llvmBuilder.CreateCall(val_func_append_arr, args);

//				SQObject val;
//				val._unVal.raw = 0;
//				switch(arg2)
//				{
//				case AAT_STACK:
//					val = STK(arg1); break;
//				case AAT_LITERAL:
//					val = ci->_literals[arg1]; break;
//				case AAT_INT:
//					val._type = OT_INTEGER;
//#ifndef _SQ64
//					val._unVal.nInteger = (SQInteger)arg1; 
//#else
//					val._unVal.nInteger = (SQInteger)((SQUnsignedInteger32)arg1);
//#endif
//					break;
//				case AAT_FLOAT:
//					val._type = OT_FLOAT;
//					val._unVal.fFloat = *((SQFloat *)&arg1);
//					break;
//				case AAT_BOOL:
//					val._type = OT_BOOL;
//					val._unVal.nInteger = arg1;
//					break;
//				default:
//					assert(0);
//					break;
//				}
//				_array(STK(arg0))->Append(val);	
//				continue;
			}
			continue;
		case _OP_COMPARITH:
			{
				Value* val_func_comparith = m_llvmModule.getFunction("SQ_LLVM_COMPARITH_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_comparith, args);

				//SQInteger selfidx = (((SQUnsignedInteger)arg1&0xFFFF0000)>>16);
				//_GUARD(DerefInc(arg3, TARGET, STK(selfidx), STK(arg2), STK(arg1&0x0000FFFF), false, selfidx)); 
			}
			continue;
		case _OP_INC: 
			{
				Value* val_func_inc = m_llvmModule.getFunction("SQ_LLVM_INC_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_inc, args);

				//SQObjectPtr o(sarg3);
				//_GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, false, arg1));
			}
			continue;
		case _OP_INCL:
			{
				Value* val_func_incl = m_llvmModule.getFunction("SQ_LLVM_INCL_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_incl, args);

				//SQObjectPtr &a = STK(arg1);
				//if(sqobjtype(a) == OT_INTEGER) 
				//{
				//	a._unVal.nInteger = _integer(a) + sarg3;
				//}
				//else 
				//{
				//	SQObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
				//	_ARITH_(+,a,a,o);
				//}
			}
			continue;
		case _OP_PINC:
			{
				Value* val_func_pinc = m_llvmModule.getFunction("SQ_LLVM_PINC_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_pinc, args);

				//SQObjectPtr o(sarg3);
				//_GUARD(DerefInc('+',TARGET, STK(arg1), STK(arg2), o, true, arg1));
			}
			continue;
		case _OP_PINCL:
			{
				Value* val_func_pincl = m_llvmModule.getFunction("SQ_LLVM_PINCL_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg3));
				m_llvmBuilder.CreateCall(val_func_pincl, args);

				//SQObjectPtr &a = STK(arg1);
				//if(sqobjtype(a) == OT_INTEGER) 
				//{
				//	TARGET = a;
				//	a._unVal.nInteger = _integer(a) + sarg3;
				//}
				//else 
				//{
				//	SQObjectPtr o(sarg3); _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));
				//}
			}
			continue;
		case _OP_CMP:
			{
				Value* val_func_cmpop = m_llvmModule.getFunction("SQ_LLVM_CMPOP_PROXY");

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg3));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg0));
				m_llvmBuilder.CreateCall(val_func_cmpop, args);
				
				//_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))
			}
			continue;
		case _OP_EXISTS: 
			{
				NOT_IMPL_YET;

				//TARGET = Get(STK(arg1), STK(arg2), temp_reg, true,DONT_FALL_BACK)?true:false;
				//continue;
			}
			continue;
		case _OP_INSTANCEOF: 
			{
				NOT_IMPL_YET;

				//if(sqobjtype(STK(arg1)) != OT_CLASS)
				//{
				//	Raise_Error(_SC("cannot apply instanceof between a %s and a %s"),GetTypeName(STK(arg1)),GetTypeName(STK(arg2))); SQ_THROW();
				//}
				//TARGET = (sqobjtype(STK(arg2)) == OT_INSTANCE) ? (_instance(STK(arg2))->InstanceOf(_class(STK(arg1)))?true:false) : false;
			}
			continue;
		case _OP_AND: 
			{
				NOT_IMPL_YET;

				//if(IsFalse(STK(arg2))) 
				//{
				//	TARGET = STK(arg2);
				//	ci->_ip += (sarg1);
				//}
			}
			continue;
		case _OP_OR:
			{
				NOT_IMPL_YET;

				//if(!IsFalse(STK(arg2))) 
				//{
				//	TARGET = STK(arg2);
				//	ci->_ip += (sarg1);
				//}
			}
			continue;
		case _OP_NEG: 
			{
				NOT_IMPL_YET;

				//_GUARD(NEG_OP(TARGET,STK(arg1))); 
			}
			continue;
		case _OP_NOT: 
			{
				NOT_IMPL_YET;
				//TARGET = IsFalse(STK(arg1)); 
			}
			continue;
		case _OP_BWNOT:
			{
				NOT_IMPL_YET;

				//if(sqobjtype(STK(arg1)) == OT_INTEGER)
				//{
				//	SQInteger t = _integer(STK(arg1));
				//	TARGET = SQInteger(~t);
				//	continue;
				//}
				//Raise_Error(_SC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
				//SQ_THROW();
			}
			continue;
		case _OP_CLOSURE:
			{
				Value* val_func_op_closure = m_InlineFunctions[INL_CLOSURE_OP]; //m_llvmModule.getFunction("SQ_LLVM_OP_CLOSURE_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				m_llvmBuilder.CreateCall(val_func_op_closure, args);

				//SQClosure *c = ci->_closure._unVal.pClosure;
				//SQFunctionProto *fp = c->_function;
				//if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto))
				//{
				//	SQ_THROW();
				//}
				//continue;
			}
			continue;
		case _OP_YIELD:
			{
				NOT_IMPL_YET;

				//if(ci->_generator) {
				//	if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
				//	_GUARD(ci->_generator->Yield(this,arg2));
				//	traps -= ci->_etraps;
				//	if(sarg1 != MAX_FUNC_STACKSIZE) _Swap(STK(arg1),temp_reg);//STK(arg1) = temp_reg;
				//}
				//else { Raise_Error(_SC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName(ci->_generator)); SQ_THROW();}
				//if(Return(arg0, arg1, temp_reg)){
				//	assert(traps == 0);
				//	outres = temp_reg;
				//	return true;
				//}
			}
			continue;
		case _OP_RESUME:
			{
				NOT_IMPL_YET;

				//if(sqobjtype(STK(arg1)) != OT_GENERATOR){ Raise_Error(_SC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1))); SQ_THROW();}
				//_GUARD(_generator(STK(arg1))->Resume(this, TARGET));
				//traps += ci->_etraps;
			}
            continue;
		case _OP_FOREACH:
			{
				NOT_IMPL_YET;

				//int tojump;
				//_GUARD(FOREACH_OP(STK(arg0),STK(arg2),STK(arg2+1),STK(arg2+2),arg2,sarg1,tojump));
				//ci->_ip += tojump; 
			}
			continue;
		case _OP_POSTFOREACH:
			{
				NOT_IMPL_YET;

				//assert(sqobjtype(STK(arg0)) == OT_GENERATOR);
				//if(_generator(STK(arg0))->_state == SQGenerator::eDead) 
				//	ci->_ip += (sarg1 - 1);
			}
			continue;
		case _OP_CLONE:
			{
				NOT_IMPL_YET;

				//_GUARD(Clone(STK(arg1), TARGET)); 
			}
			continue;
		case _OP_TYPEOF: 
			{
				NOT_IMPL_YET;

				//_GUARD(TypeOf(STK(arg1), TARGET));
			}
			continue;
		case _OP_PUSHTRAP:
			{
				NOT_IMPL_YET;

				//SQInstruction *_iv = _closure(ci->_closure)->_function->_instructions;
				//_etraps.push_back(SQExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
				//ci->_etraps++;
			}
			continue;
		case _OP_POPTRAP:
			{
				NOT_IMPL_YET;

				//for(SQInteger i = 0; i < arg0; i++) {
				//	_etraps.pop_back(); traps--;
				//	ci->_etraps--;
				//}
			}
			continue;
		case _OP_THROW:
			{
				NOT_IMPL_YET;

				//Raise_Error(TARGET); 
				//SQ_THROW();
			}
			continue;
		case _OP_NEWSLOTA:
			{
				Function* val_func_newslota = m_InlineFunctions[INL_NEWSLOTA]; //m_llvmModule.getFunction("SQ_LLVM_NEWSLOTA_PROXY");
				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(INT_TO_VALUE(arg0));
				args.push_back(INT_TO_VALUE(arg1));
				args.push_back(INT_TO_VALUE(arg2));
				args.push_back(INT_TO_VALUE(arg3));

				m_llvmBuilder.CreateCall(val_func_newslota, args);

				//_GUARD(NewSlotA(
				//	STK(arg1),
				//	STK(arg2),
				//	STK(arg3),
				//	(arg0&NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2-1) : SQObjectPtr(),
				//	(arg0&NEW_SLOT_STATIC_FLAG)?true:false,false));
			}
			continue;
		case _OP_GETBASE:
			{
				NOT_IMPL_YET;

				//SQClosure *clo = _closure(ci->_closure);
				//if(clo->_base) {
				//	TARGET = clo->_base;
				//}
				//else {
				//	TARGET.Null();
				//}
				//continue;
			}
			continue;
		case _OP_CLOSE:
			{
				Function* val_func_close = m_llvmModule.getFunction("SQ_LLVM_CLOSE_PROXY");
				Value* val_stk_arg1 = OP_STK(val_vmptr, val_stkbase, INT_TO_VALUE(arg1));

				std::vector<Value*> args;
				args.push_back(val_context);
				args.push_back(val_stk_arg1);

				m_llvmBuilder.CreateCall(val_func_close, args);

				//if(_openouters) CloseOuters(&(STK(arg1)));
			}
			continue;
		}
	}
	
	m_llvmBuilder.CreateBr(bb_leave);
	m_llvmBuilder.SetInsertPoint(bb_leave);
	m_llvmBuilder.CreateRetVoid();
}

llvm::Value* SqJitEngine::OP_EMIT_MEMBER_ACCESSING(llvm::Value* srcObject, size_t memberOffsetInBytes, llvm::Type* targetType)
{
	assert(srcObject->getType()->isPointerTy() && "srcObject must be a pointer to a structure");

	if(targetType == NULL)
	{
		targetType = m_VoidPointerTy;
	}

	Value* praw = m_llvmBuilder.CreatePtrToInt(srcObject, Type::getInt32Ty(getGlobalContext()));
	praw = m_llvmBuilder.CreateAdd(praw, INT_TO_VALUE(memberOffsetInBytes));
	Value* ptarget = m_llvmBuilder.CreateCast(Instruction::CastOps::IntToPtr, praw, targetType->getPointerTo(0));

	return m_llvmBuilder.CreateLoad(ptarget);
}

llvm::Value* SqJitEngine::OP_EMIT_MEMBER_PTR_ACCESSING(llvm::Value* srcObject, size_t memberOffsetInBytes, llvm::Type* dstType)
{
	assert(srcObject->getType()->isPointerTy() && "srcObject must be a pointer to a structure");
	
	Value* praw = m_llvmBuilder.CreatePtrToInt(srcObject, Type::getInt32Ty(getGlobalContext()));
	praw = m_llvmBuilder.CreateAdd(praw, INT_TO_VALUE(memberOffsetInBytes));
	return m_llvmBuilder.CreateCast(Instruction::CastOps::IntToPtr, praw, dstType ? dstType : m_VoidPointerTy);
}

llvm::Value* SqJitEngine::OP_EMIT_ARRAY_INDEXING(llvm::Value* arrPtr, llvm::Value* index, size_t elementSize)
{
	assert(arrPtr->getType()->isPointerTy() && "srcObject must be a pointer to a structure");
	
	Value* praw = m_llvmBuilder.CreatePtrToInt(arrPtr, Type::getInt32Ty(getGlobalContext()));
	Value* val_offset = m_llvmBuilder.CreateMul(index, INT_TO_VALUE(elementSize));
	praw = m_llvmBuilder.CreateAdd(praw, val_offset);
	return m_llvmBuilder.CreateCast(Instruction::CastOps::IntToPtr, praw, m_VoidPointerTy);
}

void SqJitEngine::EmitSQObjectAssignment(llvm::Value* srcObjectPtr, llvm::Value* dstObjectPtr)
{
}

//#define STK(a) _stack._vals[_stackbase+(a)]
//#define TARGET _stack._vals[_stackbase+arg0]
llvm::Value* SqJitEngine::OP_STK(llvm::Value* vm, llvm::Value* stkBase, llvm::Value* a)
{
	Value* val_stk = OP_EMIT_MEMBER_PTR_ACCESSING(vm, offsetof(SQVM, _stack));
	Value* val_vals = OP_EMIT_MEMBER_ACCESSING(val_stk, offsetof(SQObjectPtrVec, _vals), m_VoidPointerTy);
	Value* val_idx = m_llvmBuilder.CreateAdd(stkBase, a);
	Value* val_ret = OP_EMIT_ARRAY_INDEXING(val_vals, val_idx, sizeof(SQObjectPtr));
	return val_ret;
}

llvm::Value* SqJitEngine::OP_TARGET(llvm::Value* vm, llvm::Value* stkBase, const SQInstruction& instr)
{
	Value* val_stk = OP_EMIT_MEMBER_PTR_ACCESSING(vm, offsetof(SQVM, _stack));
	Value* val_vals = OP_EMIT_MEMBER_ACCESSING(val_stk, offsetof(SQObjectPtrVec, _vals), m_VoidPointerTy);
	Value* val_idx = m_llvmBuilder.CreateAdd(stkBase, INT_TO_VALUE(instr._arg0));
	Value* val_ret = OP_EMIT_ARRAY_INDEXING(val_vals, val_idx, sizeof(SQObjectPtr));
	return val_ret;
}

void SqJitEngine::OP_SQOBJASSIGN(llvm::Value* src, llvm::Value* dst)
{
	Value* val_func_assign = m_InlineFunctions[INL_OBJ_PTR_ASSIGN];
	std::vector<Value*> args;
	args.push_back(src);
	args.push_back(dst);
	m_llvmBuilder.CreateCall(val_func_assign, args);
}

void SqJitEngine::OP_SQOBJFROMINT(llvm::Value* srcInt, llvm::Value* dst)
{
	llvm::Value* dstIntPtr = OP_EMIT_MEMBER_PTR_ACCESSING(dst, offsetof(SQObjectPtr, _unVal));
	llvm::Value* dstTypePtr = OP_EMIT_MEMBER_PTR_ACCESSING(dst, offsetof(SQObjectPtr, _type));
	m_llvmBuilder.CreateStore(srcInt, dstIntPtr);
	m_llvmBuilder.CreateStore(m_llvmBuilder.getInt32(OT_INTEGER), dstTypePtr);
}

void SqJitEngine::OP_SQOBJFROMFLOAT(llvm::Value* srcFloat, llvm::Value* dst)
{
	llvm::Value* dstFloatPtr = OP_EMIT_MEMBER_PTR_ACCESSING(dst, offsetof(SQObjectPtr, _unVal), llvm::Type::getFloatPtrTy(m_llvmContext));
	llvm::Value* dstTypePtr = OP_EMIT_MEMBER_PTR_ACCESSING(dst, offsetof(SQObjectPtr, _type));
	m_llvmBuilder.CreateStore(srcFloat, dstFloatPtr);
	m_llvmBuilder.CreateStore(m_llvmBuilder.getInt32(OT_FLOAT), dstTypePtr);
}

void SqJitEngine::OP_SQOBJFROMUSERPOINTER(llvm::Value* srcUP, llvm::Value* dst)
{
}

void SqJitEngine::OP_CALLFUNCTION(const std::string& name, llvm::Value* vmptr, llvm::Value* cloptr)
{
}

void SqJitEngine::OP_SQOBJSWAP(llvm::Value* left, llvm::Value* right)
{
	Value* aType = OP_EMIT_MEMBER_ACCESSING(left, offsetof(SQObjectPtr, _type), m_SQIntegerTy);
	Value* aValue = OP_EMIT_MEMBER_ACCESSING(left, offsetof(SQObjectPtr, _unVal), m_SQIntegerTy);

	Value* aTypePtr = OP_EMIT_MEMBER_PTR_ACCESSING(left, offsetof(SQObjectPtr, _type));
	Value* aValuePtr = OP_EMIT_MEMBER_PTR_ACCESSING(left, offsetof(SQObjectPtr, _unVal));

	Value* bType = OP_EMIT_MEMBER_ACCESSING(right, offsetof(SQObjectPtr, _type), m_SQIntegerTy);
	Value* bValue = OP_EMIT_MEMBER_ACCESSING(right, offsetof(SQObjectPtr, _unVal), m_SQIntegerTy);

	Value* bTypePtr = OP_EMIT_MEMBER_PTR_ACCESSING(right, offsetof(SQObjectPtr, _type));
	Value* bValuePtr = OP_EMIT_MEMBER_PTR_ACCESSING(right, offsetof(SQObjectPtr, _unVal));

	m_llvmBuilder.CreateStore(bType, aTypePtr);
	m_llvmBuilder.CreateStore(bValue, aValuePtr);

	m_llvmBuilder.CreateStore(aType, bTypePtr);
	m_llvmBuilder.CreateStore(aValue, bValuePtr);

	//SQObjectType tOldType = a._type;
	//SQObjectValue unOldVal = a._unVal;
	//a._type = b._type;
	//a._unVal = b._unVal;
	//b._type = tOldType;
	//b._unVal = unOldVal;
}

#endif//SQ_JIT_LLVM