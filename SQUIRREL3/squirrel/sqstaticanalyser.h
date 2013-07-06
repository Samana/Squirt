#ifndef __SQSTATICANALYSER_H__
#define __SQSTATICANALYSER_H__

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
	SQAST_Variable,
	SQAST_Member,
	SQAST_Local,
	SQAST_FunctionParam,
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

struct SQAstNode {
	SQAstNode* _parent;
	SQObjectPtr _identifier;
	SQObjectPtr _datatype;
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

	virtual bool CollectTypeInfo()
	{
		return true;
	}
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

struct SQAstNode_Table : public SQAstNode {
	AST_NODE_TYPE(SQAST_Table);
};

struct SQAstNode_Class : public SQAstNode {
	AST_NODE_TYPE(SQAST_Class);
	SQAstNode_Class() { }
	virtual ~SQAstNode_Class() { }
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

struct SQAstNode_UnaryExpr : public SQAstNode {
	AST_NODE_TYPE(SQAST_UnaryExpr);
	virtual ~SQAstNode_UnaryExpr() { }
};

struct SQAstNode_BinaryExpr : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_BinaryExpr);
	virtual ~SQAstNode_BinaryExpr() { }
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

	enum ConstantType {
		CT_Boolean,
		CT_Integer,
		CT_Float,
		CT_String,
	};

	ConstantType _constanttype;

	AST_NODE_TYPE(SQAST_Constant);
	virtual ~SQAstNode_Constant() { }
};

struct SQAstNode_Var : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_Var);
	virtual ~SQAstNode_Var() { }
};

struct SQAstNode_Variable : public SQAstNode_Statement {
	AST_NODE_TYPE(SQAST_Variable);
	virtual ~SQAstNode_Variable() { }

	SQAstNode_Expr* _initializer;
};

struct SQAstNode_Member : public SQAstNode_Variable {
	AST_NODE_TYPE(SQAST_Member);
	virtual ~SQAstNode_Member() { }
	SQAstNode_Table* _attributes;
};

struct SQAstNode_Local : public SQAstNode_Variable {
	AST_NODE_TYPE(SQAST_Local);
	virtual ~SQAstNode_Local() { }
};

struct SQAstNode_FunctionParam : public SQAstNode_Local {
	AST_NODE_TYPE(SQAST_FunctionParam);
	virtual ~SQAstNode_FunctionParam() { }
};

struct SQAstNode_FunctionDef : public SQAstNode_Expr {
	AST_NODE_TYPE(SQAST_FunctionDef);
	virtual ~SQAstNode_FunctionDef() { }

	sqvector<SQAstNode_Local*> _params;
	SQAstNode_CodeBlock* _body;
};

//struct SQAstNode_PrefixedExpr : public SQAstNode_Expr {
//	AST_NODE_TYPE(SQAST_PrefixedExpr);
//	virtual ~SQAstNode_PrefixedExpr() { }
//
//};

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
