#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqreflection.h"
#include "sqstaticanalyser.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqstring.h"


const SQChar* GetStringFromAstNodeType(SQAstNodeType type)
{
	static const SQChar* s_SQAstNodeTypeLiteral[] =
	{
		_SC("NODE"),
		_SC("Document"),
		_SC("Using"),
		_SC("Namespace"),
		_SC("Table"),
		_SC("Class"),
		_SC("CodeBlock"),
		_SC("Constant"),
		_SC("Expr"),
		_SC("Statement"),
		_SC("Variable"),
		_SC("Member"),
		_SC("Local"),
		_SC("FunctionParam"),
		_SC("FunctionDef"),
		_SC("If"),
		_SC("While"),
		_SC("DoWhile"),
		_SC("For"),
		_SC("Foreach"),
		_SC("Switch"),
		_SC("ReturnYield"),
		_SC("Try"),
		_SC("Throw"),
		_SC("MAX"),
	};

	return s_SQAstNodeTypeLiteral[type];
}

void SQAstNode::Dump(SQInteger nodedepth, SQVM* vm)
{
	for(int i=0; i<nodedepth; i++) { scprintf(_SC("  ")); }
	scprintf(GetStringFromAstNodeType(this->GetNodeType()));
	SQObjectPtr strIdentifier;
	vm->ToString(_identifier, strIdentifier);
	scprintf(_SC("  '%s' [ %d : %d - %d : %d ]\n"),
		_string(strIdentifier)->_val,
		_startline, _startcol, _endline, _endcol);
	for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
	{
		_commonchildren[i]->Dump(nodedepth + 1, vm);
	}
}

bool SQAstNode_NamespaceBase::AddNamespace(SQAstNode_Namespace* pns)
{
	AddCommonChild(pns);
	_namespaces.push_back(pns);
	return true;
}

bool SQAstNode_NamespaceBase::AddClass(SQAstNode_Class* pcl)
{
	AddCommonChild(pcl);
	_classes.push_back(pcl);
	return true;
}

#endif//NO_COMPILER