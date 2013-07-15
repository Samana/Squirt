#ifndef __SQSTATICANALYSER_H__
#define __SQSTATICANALYSER_H__

#include "squirt.h"
#include "sqopcodes.h"

struct SQAstNode;
struct SQAstNode_Namespace;
struct SQAstNode_Class;


enum SQAstNodeType
{
	SQAST_NODE,
	SQAST_Document,
	SQAST_Using,
	SQAST_Namespace,
	SQAST_Table,
	SQAST_Class,
	SQAST_CodeBlock,
	SQAST_Constant,
	SQAST_Expr,
	SQAST_UnaryExpr,
	SQAST_BinaryExpr,
	SQAST_AssignmentExpr,
	SQAST_CondEvalExpr,
	SQAST_Var,
	SQAST_Statement,
	SQAST_DeclVariable,
	SQAST_DeclMember,
	SQAST_DeclLocal,
	SQAST_DeclFunctionParam,
	SQAST_FunctionDef,
	SQAST_Call,
	SQAST_If,
	SQAST_While,
	SQAST_DoWhile,
	SQAST_For,
	SQAST_Foreach,
	SQAST_Switch,
	SQAST_ReturnYield,
	SQAST_Try,
	SQAST_Throw,
	SQAST_MAX,
};

const SQChar* GetStringFromAstNodeType(SQAstNodeType type);

#define AST_NODE_TYPE( X ) virtual SQAstNodeType GetNodeType() const { return X; }

#define SQ_AST_NO_COPY(X)	private: X(const X&); X& operator=(const X&);

struct SQAstNode {
	SQAstNode* _parent;
	SQObjectPtr _identifier;
	SQTypeDesc _typetag;
	SQInteger _startline;
	SQInteger _startcol;
	SQInteger _endline;
	SQInteger _endcol;

	sqvector<SQAstNode*> _commonchildren;

	SQAstNode()
		: _parent(NULL)
		, _startline(0)
		, _startcol(0)
		, _endline(0)
		, _endcol(0)
	{
	}

	virtual ~SQAstNode()
	{
		for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
		{
			delete _commonchildren[i];
		}
	}

	virtual SQAstNode* FindParent(SQAstNodeType type)
	{
		SQAstNode* pParent = _parent;
		while(pParent)
		{
			if(pParent->GetNodeType() == type)
			{
				return pParent;
			}
			pParent = pParent->_parent;
		}
		return NULL;
	}

	virtual SQAstNodeType GetNodeType() const { return SQAST_NODE; }

	virtual bool SetParent(SQAstNode* pParent)
	{
		if(_parent != NULL)
		{
			for(SQUnsignedInteger i=0; i<_parent->_commonchildren.size(); i++)
			{
				if(_parent->_commonchildren[i] == this)
				{
					_parent->_commonchildren.remove(i);
					break;
				}
			}
		}
		
		if(pParent)
		{
			pParent->_commonchildren.push_back(this);
		}
		_parent = pParent;
		return true;
	}

	virtual bool AddNamespace(SQAstNode_Namespace* ) { return false; }
	virtual bool AddClass(SQAstNode_Class* pcl) { return false; }

	virtual void Dump(SQInteger nodedepth, SQVM* vm);

	virtual bool Analysis()
	{
		return true;
	}

	virtual bool CollectDefinedTypes(SQTable* pTypeTable, const std::scstring& namePrefix);
	virtual bool DecorateWithTypeInfo(SQAssembly* pAssembly)
	{
		for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
		{
			if(!_commonchildren[i]->DecorateWithTypeInfo(pAssembly))
			{
				return false;
			}
		}
		return true;
	}
	virtual bool CheckTypes(SQAssembly* pAssembly)
	{
		for(SQUnsignedInteger i=0; i<_commonchildren.size(); i++)
		{
			if(!_commonchildren[i]->CheckTypes(pAssembly))
			{
				return false;
			}
		}
		return true;
	}

	SQ_AST_NO_COPY(SQAstNode);
};

struct SQAstNode_Using : public SQAstNode {
	AST_NODE_TYPE(SQAST_Using);
	SQAstNode_Using() { }
	~SQAstNode_Using() { }
};

struct SQAstNode_NamespaceBase : public SQAstNode {
	SQAstNode_NamespaceBase() { }
	virtual ~SQAstNode_NamespaceBase() { }

	virtual SQAstNodeType GetNodeType() const = 0;
	
	virtual bool AddNamespace(SQAstNode_Namespace* pns);
	virtual bool AddClass(SQAstNode_Class* pcl);

	virtual bool CollectDefinedTypes(SQTable* pTypeTable, const std::scstring& namePrefix);

	sqvector<SQAstNode_Class*> _classes;
	sqvector<SQAstNode_Namespace*> _namespaces;
};

struct SQAstNode_Document : public SQAstNode_NamespaceBase {
	AST_NODE_TYPE(SQAST_Document);
	SQAstNode_Document() { }
	virtual ~SQAstNode_Document() { }
};

struct SQAstNode_Namespace : public SQAstNode_NamespaceBase {
	AST_NODE_TYPE(SQAST_Namespace);
	SQAstNode_Namespace() { }
	virtual ~SQAstNode_Namespace() { }
};

struct SQAstNode_Class : public SQAstNode_NamespaceBase {
	AST_NODE_TYPE(SQAST_Class);
	SQAstNode_Class() { }
	virtual ~SQAstNode_Class() { }
};

struct SQAstNode_Table : public SQAstNode {
	AST_NODE_TYPE(SQAST_Table);
};

struct SQAstNode_CodeBlock : public SQAstNode {
	AST_NODE_TYPE(SQAST_CodeBlock);
	SQAstNode_CodeBlock() { }
	virtual ~SQAstNode_CodeBlock() { }
};

struct SQAstNode_Expr : public SQAstNode {
	AST_NODE_TYPE(SQAST_Expr);
	virtual ~SQAstNode_Expr() { }
};

struct SQAstNode_UnaryExpr : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_UnaryExpr);
	virtual ~SQAstNode_UnaryExpr() { }
	virtual bool CheckTypes(SQAssembly* pAssembly);

	SQOpcode _opcode;
};

struct SQAstNode_BinaryExpr : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_BinaryExpr);
	virtual ~SQAstNode_BinaryExpr() { }
	virtual bool CheckTypes(SQAssembly* pAssembly);

	SQOpcode _opcode;
	SQInteger _op3;
};

struct SQAstNode_AssignmentExpr : public SQAstNode_BinaryExpr {
	AST_NODE_TYPE(SQAST_AssignmentExpr);
	virtual ~SQAstNode_AssignmentExpr() { }
};

struct SQAstNode_CondEvalExpr : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_CondEvalExpr);
	virtual ~SQAstNode_CondEvalExpr() { }
};

struct SQAstNode_Statement : public SQAstNode {
	AST_NODE_TYPE(SQAST_Statement);
	virtual ~SQAstNode_Statement() { }
};

struct SQAstNode_Constant : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_Constant);
	virtual ~SQAstNode_Constant() { }

	virtual bool DecorateWithTypeInfo(SQAssembly* pAssembly);
};

struct SQAstNode_Var : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_Var);
	virtual ~SQAstNode_Var() { }

	virtual bool DecorateWithTypeInfo(SQAssembly* pAssembly);
};

struct SQAstNode_DeclVariable : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_DeclVariable);
	virtual ~SQAstNode_DeclVariable() { }

	SQAstNode_Expr* _initializer;
};

struct SQAstNode_DeclMember : public SQAstNode_DeclVariable {
	AST_NODE_TYPE(SQAST_DeclMember);
	SQAstNode_DeclMember() : _attributes(NULL), _isstatic(false) { }
	virtual ~SQAstNode_DeclMember() { }
	SQAstNode_Table* _attributes;
	bool _isstatic;
};

struct SQAstNode_DeclLocal : public SQAstNode_DeclVariable {
	AST_NODE_TYPE(SQAST_DeclLocal);
	virtual ~SQAstNode_DeclLocal() { }
};

struct SQAstNode_DeclFunctionParam : public SQAstNode_DeclLocal {
	AST_NODE_TYPE(SQAST_DeclFunctionParam);
	virtual ~SQAstNode_DeclFunctionParam() { }
};

struct SQAstNode_FunctionDef : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_FunctionDef);
	SQAstNode_FunctionDef() { _typetag = SQTypeDesc(SQ_TYPE_CLOSURE); }
	virtual ~SQAstNode_FunctionDef() { }

	virtual void GetSignatureString(std::scstring& signature) const;

	SQTypeDesc _returntype;
	sqvector<SQAstNode_DeclLocal*> _params;
	SQAstNode_CodeBlock* _body;
};

struct SQAstNode_Call : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_Call);
	virtual ~SQAstNode_Call() { }

	SQAstNode_Expr _func;
	sqvector<SQAstNode_Expr*> _args;
};

struct SQAstNode_If : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_If);
	virtual ~SQAstNode_If() { }
	
	SQAstNode_Expr* _condition;
	SQAstNode_CodeBlock* _trueblock;
	SQAstNode_CodeBlock* _falseblock;
};

struct SQAstNode_While : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_While);
	virtual ~SQAstNode_While() { }

	SQAstNode_Expr* _condition;
	SQAstNode_CodeBlock* _loopblock;
};

struct SQAstNode_For : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_For);
	virtual ~SQAstNode_For() { }

	SQAstNode_Expr* _init;
	SQAstNode_Expr* _condition;
	SQAstNode_Expr* _inc;
	SQAstNode_CodeBlock* _loopblock;
};
struct SQAstNode_Foreach : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_Foreach);
	virtual ~SQAstNode_Foreach() { }

	SQObjectPtr _indexid;
	SQObjectPtr _valueid;
	SQAstNode_Expr* _enumerable;
	SQAstNode_CodeBlock* _loopblock;
};
struct SQAstNode_Switch : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_Switch);
	virtual ~SQAstNode_Switch() { }

	SQAstNode_Expr* _eval;
	struct Case
	{
		SQAstNode_Expr* _value;
		SQAstNode_CodeBlock* _caseblock;
	};
	sqvector<Case> _cases;
};
struct SQAstNode_ReturnYield : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_ReturnYield);
	virtual ~SQAstNode_ReturnYield() { }

	SQAstNode_Expr* _retvalue;
};

struct SQAstNode_Try : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_Try);
	virtual ~SQAstNode_Try() { }

	SQAstNode_CodeBlock* _tryblock;
	SQObjectPtr _catchid;
	SQAstNode_CodeBlock* _catchblock;
};

struct SQAstNode_Throw : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_Throw);
	virtual ~SQAstNode_Throw() { }

	SQAstNode_Expr* _throw;
};

//------------------------------------------------------------------------------------------------


struct SQAstNodeFactory
{
	template<class T>
	static T* CreateAstNode(SQAstNode* parent)
	{
		T* ret = new T();
		ret->SetParent(parent);
		return ret;
	}

	template<>
	static SQAstNode_Namespace* CreateAstNode<SQAstNode_Namespace>(SQAstNode* parent)
	{
		SQAstNode_Namespace* ret = new SQAstNode_Namespace();
		parent->AddNamespace(ret);
		return ret;
	}
};

#endif//__SQSTATICANALYSER_H__
