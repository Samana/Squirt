#include "sqpcheader.h"
#ifndef NO_COMPILER
#include "sqreflection.h"
#include "sqstaticanalyser.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqstring.h"
#include "sqtable.h"


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
		_SC("UnaryExpr"),
		_SC("BinaryExpr"),
		_SC("AssignmentExpr"),
		_SC("CondEvalExpr"),
		_SC("Var"),
		_SC("Statement"),
		_SC("Variable"),
		_SC("Member"),
		_SC("Local"),
		_SC("FunctionParam"),
		_SC("FunctionDef"),
		_SC("Call"),
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
	std::scstring buf;
	vm->ToString(_identifier, strIdentifier);
	scprintf(_SC("  '%s'  [%s]  .........  [ %d : %d - %d : %d ]\n"),
		_string(strIdentifier)->_val,
		_typetag.ToString(buf),
		_startline, _startcol, _endline, _endcol);
	for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
	{
		_commonchildren[i]->Dump(nodedepth + 1, vm);
	}
}

bool SQAstNode::CollectDefinedTypes(SQTable* pTypeTable, const std::scstring& namePrefix)
{
	return true;
}

bool SQAstNode_NamespaceBase::AddNamespace(SQAstNode_Namespace* pns)
{
	pns->SetParent(this);
	_namespaces.push_back(pns);
	return true;
}

bool SQAstNode_NamespaceBase::AddClass(SQAstNode_Class* pcl)
{
	pcl->SetParent(this);
	_classes.push_back(pcl);
	return true;
}

bool SQAstNode_NamespaceBase::CollectDefinedTypes(SQTable* pTypeTable, const std::scstring& namePrefix)
{
	std::scstring childPrefix = namePrefix;
	if(childPrefix.length() > 0)
	{
		childPrefix += _SC(".");
	}

	if(this->GetNodeType() != SQAST_Document)
	{
		childPrefix += _string(_identifier)->_val;
		childPrefix += _SC(".");
	}

	for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
	{
		if(_commonchildren[i]->GetNodeType() == SQAST_Namespace
			|| _commonchildren[i]->GetNodeType() == SQAST_Class
			|| _commonchildren[i]->GetNodeType() == SQAST_Document)
		{
			SQObjectPtr key = SQString::Create(pTypeTable->_sharedstate, (childPrefix + _string(_commonchildren[i]->_identifier)->_val).c_str());
			SQObjectPtr val = SQTable::Create(pTypeTable->_sharedstate, 0);
			if(!pTypeTable->NewSlot(key, val))
				return false;

			if(!_commonchildren[i]->CollectDefinedTypes(_table(val), childPrefix))
				return false;
		}
	}

	return true;
}

bool SQAstNode_Constant::DecorateWithTypeInfo(SQAssembly* pAssembly)
{
	return true;
}

bool SQAstNode_Var::DecorateWithTypeInfo(SQAssembly* pAssembly)
{
	return true;
}

void SQAstNode_FunctionDef::GetSignatureString(std::scstring& signature) const
{
	//FIXME: Potential inf loop if function param has the same function type?
	std::basic_stringstream<SQChar> stream;
	stream << _SC("function ( ");
	std::scstring buf;
	for(SQUnsignedInteger i=0; i<_params.size(); i++)
	{
		stream << _params[i]->_typetag.ToString(buf);
		if(i != _params.size() - 1)
		{
			stream << _SC(", ");
		}
	}
	stream << _SC(" ) : ");
	stream << _returntype.ToString(buf);
	signature = stream.str();
}

#endif//NO_COMPILER