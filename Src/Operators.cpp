#include "Operators.h"

std::string indentation(unsigned indent) {
	std::string res = "";
	for (unsigned i = 0; i < indent; i++)
		res += "  ";
	return res;
}

const bool PRINT_VAR_TABLE = false;

// Block Operator
void Block::addOperator(Operator* op) {
	ops.push_back(op);
}
Block::Block() { ops = std::list<Operator*>(); }
Block::Block(Operator* op) { addOperator(op); }
Block::Block(Operator* op1, Operator* op2) {
	addOperator(op1);
	addOperator(op2);
}
Block::Block(std::list<Operator*> newOps) {
    ops = newOps;
}
size_t Block::size() {
	return ops.size();
}
void Block::print(unsigned indent) {
	std::cout << indentation(indent) << "{" << std::endl;
	for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); i++) {
		(*i)->print(indent + 1);
	}
	std::cout << indentation(indent) << "}" << std::endl;
}
Block::~Block() {
	for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); i++) {
		delete *i;
	}
}
void Block::run(Block* parentBlock) {
	this->parentBlock = parentBlock;
	for (std::list<Operator*>::iterator i = ops.begin(); i != ops.end(); i++) {
		try {
			(*i)->run(this);
		}
		catch (const std::exception & ex) {
			std::cerr << "!!! Error !!!  " << ex.what() << std::endl;
            std::string skipLine;
            std::getline(std::cin, skipLine);
        }
        if (PRINT_VAR_TABLE) {
            printVarTable();
            std::cout << std::endl;
        }
	}
}
Var* Block::findVar(const std::string& id) {
	Block* currBlock = this;
	std::map<std::string, Var*>::iterator result = this->vars.end();
	while (result == vars.end() && currBlock != nullptr) {
		if (currBlock->vars.find(id) != currBlock->vars.end())
			result = currBlock->vars.find(id); // win!
		else
			currBlock = currBlock->parentBlock;
	}
	if (result != this->vars.end())
		return result->second;
	else
		return nullptr;
}
void Block::addVar(const std::string& id, Var* newVar) {
	if (findVar(id) == nullptr) {
		vars.insert(std::pair<std::string, Var*>(id, newVar));
	}
	else
		throw UndefinedVarException("VarExpression with the same ID already exists");
}
void Block::printVarTable() const {
	if (parentBlock) {
		std::cout << "**** Variables of nested block ****" << std::endl;
		parentBlock->printVarTable();
	}
	else
		std::cout << "**** Variables ****" << std::endl;
	for (std::map<std::string, Var*>::const_iterator i = vars.begin(); i != vars.end(); i++) {
		switch ((*i).second->getType()) {
		case T_INT:
			std::cout << "int ";
			break;
		case T_PTR:
			std::cout << "ptr ";
			break;
		case T_ARR:
			std::cout << "arr ";
			break;
		}
		std::cout << (*i).first << " = " << *((*i).second);
        /*
		try {
			switch ((*i).second->getType()) {
			case T_INT: {
				std::cout << (*i).second->getIntVal();
				break;
			}
			case T_PTR: {
				std::cout << (*i).second->getPtrVal();
				break;
			}
			case T_ARR: {
				size_t s = (*i).second->getArrSize();
				std::cout << "[";
				for (size_t j = 0; j < s - 1; j++) {
					try {
						std::cout << (*i).second->getArrAtVal(j) << ", ";
					}
					catch (const NotInitVarException & ex) {
						std::cout << "None" << ", ";
					}
				}
				try {
					std::cout << (*i).second->getArrAtVal(s - 1) << "]";
				}
				catch (const NotInitVarException & ex) {
					std::cout << "None" << "]";
				}
				break;
			}
			}
		}
		catch (const NotInitVarException & ex) {
			std::cout << "None";
		}
		catch (const std::runtime_error & ex) {
			std::cout << "smth went wrong";
		}
        */
		std::cout << std::endl;
	}

	if (parentBlock)
		std::cout << "***********************************" << std::endl;
	else
		std::cout << "*******************" << std::endl;
}
void Block::clearVarTable() {
	vars.clear();
}


// Expression Operator
ExprOperator::ExprOperator(Expression* expr) : expr(expr) {}
void ExprOperator::print(unsigned indent) {
	//std::cout << "print exprop\n";
	std::cout << indentation(indent);
	expr->print();
	std::cout << ";" << std::endl;
}
ExprOperator::~ExprOperator() { delete expr; }
void ExprOperator::run(Block* parentBlock) {
	Visitor v;
	expr->accept(v);
	expr->eval(parentBlock);
	// do nothing...
}


// If Operator
IfOperator::IfOperator(Expression* cond, Block* thenBlock, Block* elseBlock) :
	cond(cond), thenBlock(thenBlock), elseBlock(elseBlock) {}
void IfOperator::print(unsigned indent) {
	std::cout << indentation(indent);
	std::cout << "if ";
	cond->print();
	std::cout << std::endl;
	thenBlock->print(indent);
	if (elseBlock != nullptr) {
		std::cout << indentation(indent) << "else" << std::endl;
		elseBlock->print(indent);
	}
}
IfOperator::~IfOperator() { delete cond; }
void IfOperator::run(Block* parentBlock) {
	int condResult = cond->eval(parentBlock).getIntVal();
	if (condResult == 1) {
		thenBlock->run(parentBlock);
		thenBlock->clearVarTable();
	}
	else {
		if (elseBlock) {
			elseBlock->run(parentBlock);
			elseBlock->clearVarTable();
		}
	}
}


// While Operator
WhileOperator::WhileOperator(Expression* cond, Block* body) :
	cond(cond), body(body) {}
void WhileOperator::print(unsigned indent) {
	std::cout << indentation(indent) << "while ";
	cond->print();
	std::cout << std::endl;
	body->print(indent);
}
WhileOperator::~WhileOperator() { delete cond; }
void WhileOperator::run(Block* parentBlock) {
	int condResult = cond->eval(parentBlock).getIntVal();
	while (condResult == 1) {
		body->run(parentBlock);
		body->clearVarTable();
		condResult = cond->eval(parentBlock).getIntVal();
	}
}


// Assign Operator
AssignOperator::AssignOperator(const std::string& ID, Expression* value, bool isDereferecing) :
	ID(ID), value(value), isDereferecing(isDereferecing) {
	index = nullptr;
}
AssignOperator::AssignOperator(const std::string& ID, Expression* value, Expression* index) :
	ID(ID), value(value), index(index) {
	isDereferecing = false;
}
void AssignOperator::print(unsigned indent) {
	std::cout << indentation(indent) << (isDereferecing ? "*" : "") << ID << " = ";
	value->print();
	std::cout << ";" << std::endl;
}
AssignOperator::~AssignOperator() { delete value; delete index; }
void AssignOperator::run(Block* parentBlock) {
	// value may be: unaryexpr, binaryexpr, value, variable, funccal
	Visitor v;
	value->accept(v);
	
	Var* targetVar;
	if (isDereferecing) {
		targetVar = parentBlock->findVar(ID)->getPtrVal();
	}
	else {
		targetVar = parentBlock->findVar(ID);
	}
	if (targetVar == nullptr) {
		throw UndefinedVarException();
	}
	else if (targetVar->getType() == T_INT && index != nullptr) {
		throw InvalidTypeException("Int has no index");
	}
	/*else if (targetVar->getType() == T_ARR && index == nullptr) {
		throw InvalidTypeException("Assign to array without index");
	}*/


	// x = 5
	if (v.type() == E_VAL) {
        switch (targetVar->getType()) {
            case T_INT: {
                targetVar->setIntVal(value->eval(parentBlock).getIntVal());
            } break;
            case T_PTR: {
                if (index != nullptr)
                    targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
                else
                    throw InvalidTypeException("Assign int to ptr without index");
            } break;
            case T_ARR: {
                if (index != nullptr)
                    targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
                else
                    throw InvalidTypeException("Assign int to arr without index");
            } break;
        }
    }
	// x = y
	else if (v.type() == E_VAR) {
        VarExpression *valueVariable = dynamic_cast<VarExpression *>(value);
        Var *sourceVar = parentBlock->findVar(valueVariable->getID());
        //sourceVar = value->eval();
        switch (targetVar->getType()) {
            case T_INT: {
                if (sourceVar->getType() != T_INT)
                    throw InvalidTypeException("Assign non-int to int");
                targetVar->setIntVal(sourceVar->getIntVal());
            } break;
            case T_PTR: {
                if (sourceVar->getType() == T_INT) {
                    if (index != nullptr)
                        targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(),
                                               index->eval(parentBlock).getIntVal());
                    else
                        throw InvalidTypeException("Assign int to ptr");
                } else if (sourceVar->getType() == T_PTR)
                    targetVar->setPtrVal(sourceVar->getPtrVal());
                else if (sourceVar->getType() == T_ARR)
                    throw InvalidTypeException("Assign arr to ptr");
            } break;
            case T_ARR: {
                if (sourceVar->getType() != T_INT)
                    throw InvalidTypeException("Assign non-int to arr[int]");
                targetVar->setArrAtVal(sourceVar->getIntVal(), index->eval(parentBlock).getIntVal());
            } break;
        }
    }

	// x = malloc(2)
	else if (v.type() == E_FUNC) {
        Var returnValue = value->eval(parentBlock);
        switch (targetVar->getType()) {
            case T_INT: {
                throw InvalidTypeException("Assign ptr to int");
            } break;
            case T_PTR: {
                targetVar->setArrVal(returnValue.getArr(), returnValue.getArrSize());
            } break;
            case T_ARR: {
                if (index == nullptr) {
                    targetVar->setArrVal(returnValue.getArr(), returnValue.getArrSize());
                }
                else
                    throw InvalidTypeException("Assign arr to arr element");
            } break;
        }
    }
	// x = -(y + 5)
	else if (v.type() == E_UNARY) {
        switch (targetVar->getType()) {
            case T_INT: {
                targetVar->setIntVal(value->eval(parentBlock).getIntVal());
            } break;
            case T_PTR: {
                if (index != nullptr)
                    targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
                else
                    targetVar->setPtrVal(value->eval(parentBlock).getPtrVal());
            } break;
            case T_ARR: {
                if (index == nullptr)
                    throw InvalidTypeException("Assign to arr without index");
                targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
            } break;
        }
    }
	// x = y + 2
	else if (v.type() == E_BIN) {
        switch (targetVar->getType()) {
            case T_INT: {
                targetVar->setIntVal(value->eval(parentBlock).getIntVal());
            } break;
            case T_PTR: {
                if (index != nullptr)
                    targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
                else
                    targetVar->setPtrVal(value->eval(parentBlock).getPtrVal());
            } break;
            case T_ARR: {
                if (index == nullptr)
                    throw InvalidTypeException("Assign to arr without index");
                targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
            } break;
        }
    }

	else if (v.type() == E_ARRAT) {
		switch (targetVar->getType()) {
		case T_INT: {
			targetVar->setIntVal(value->eval(parentBlock).getIntVal());
			break;
		}
		case T_PTR: {
			throw InvalidTypeException("Assign arr[i] to ptr");
			break;
		}
		case T_ARR: {
			if (index == nullptr)
				throw InvalidTypeException("Assign to arr without index");
			targetVar->setArrAtVal(value->eval(parentBlock).getIntVal(), index->eval(parentBlock).getIntVal());
			break;
		}
		}
	}
}


// Define Operator
DefOperator::DefOperator(VType T, const std::string& ID, Expression* value) :
	type(T), ID(ID), value(value) {
	size = 0;
}
DefOperator::DefOperator(VType T, const std::string& ID, const std::string& size, Expression* value) :
	type(T), ID(ID), size(stoi(size)), value(value) {}
void DefOperator::print(unsigned indent) {
	//std::cout << "print def\n";
	std::cout << indentation(indent);
	if (type == T_INT)
		std::cout << "int " << ID;
	else if (type == T_PTR)
		std::cout << "ptr " << ID;
	else if (type == T_ARR)
		std::cout << "arr " << ID << "[" << size << "]";
	if (value != nullptr)
		std::cout << " = " << "expr";
	std::cout << ";" << std::endl;
}
void DefOperator::run(Block* parentBlock) {
	//std::cout << "Run def operator" << std::endl;
	if (type == T_ARR && size != 0) {
		Var* newVar = new Var(T_ARR, size);
		parentBlock->addVar(ID, newVar);
	}
	else {
		Var* newVar = new Var(type);
		if (value) {
			if (type == T_INT) {
				newVar->setIntVal(value->eval(parentBlock).getIntVal());
			}
			else if (type == T_PTR) {
				newVar->setPtrVal(value->eval(parentBlock).getPtrVal());
			}
		}
		parentBlock->addVar(ID, newVar);
	}
}


// Function call
FunctionCall::FunctionCall(const std::string& ID, const std::list<Expression*>& args) :
	ID(ID), args(args) {}
void FunctionCall::print() {
	std::cout << ID << "(";
	for (std::list<Expression*>::iterator i = args.begin(); i != args.end(); i++) {
		if (i != args.begin())
			std::cout << ", ";
		(*i)->print();
	}
	std::cout << ")";
}
FunctionCall::~FunctionCall() {
	for (std::list<Expression*>::iterator i = args.begin(); i != args.end(); i++) {
		delete *i;
	}
}
Var FunctionCall::eval(Block* parentBlock) {
	Var resVar;
	if (ID == "malloc" && args.size() == 1) {
		size_t sizemem = (args.front())->eval(parentBlock).getIntVal();
		resVar = Var(new int[sizemem], sizemem);
	}
	else if (ID == "print" && args.size() == 1) {
		std::cout << "PRINT: ";
		std::cout << args.front()->eval(parentBlock) << std::endl;
	}
    else if (ID == "printVarTable" && args.size() == 0) {
        parentBlock->printVarTable();
    }
	return resVar;
}
void FunctionCall::accept(Visitor & v) { v.visit(this); }


// Unary Expression
UnaryExpression::UnaryExpression(const char* op, Expression* arg) :
	op(op), arg(arg) {}
void UnaryExpression::print() {
	std::cout << op;
	arg->print();
}
UnaryExpression::~UnaryExpression() { delete arg; }
Var UnaryExpression::eval(Block* parentBlock) {
	Var result;
	Visitor v;
	arg->accept(v);
	switch (*op)
	{
	case '-':
	{
		result = Var((-1) * arg->eval(parentBlock).getIntVal());
	}
	break;
	case '!':
	{
		result = ((arg->eval(parentBlock).getIntVal()) != 0 ? 0 : 1);
	}
	break;
	case '&':
	{
		// to do: add non variable* arg
		VarExpression* argVariable = dynamic_cast<VarExpression*>(arg);
		UnaryExpression* argUnaryExpr = dynamic_cast<UnaryExpression*>(arg);
		if (argVariable != nullptr)
			result = Var(parentBlock->findVar(argVariable->getID()));
		/*else if (argUnaryExpr != nullptr && *(argUnaryExpr->op) == '*') {
			result = &(argUnaryExpr->eval(parentBlock));
		}*/
		else {
			std::cout << "It's not working :(" << std::endl;
		}
	}
	break;
	case '*':
	{
		VarExpression* argVariable = dynamic_cast<VarExpression*>(arg);
		UnaryExpression* argUnaryExpr = dynamic_cast<UnaryExpression*>(arg);
		if (argVariable != nullptr)
			result = *(parentBlock->findVar(argVariable->getID())->getPtrVal());
		else if (argUnaryExpr != nullptr && *(argUnaryExpr->op) == '&') {
			result = *(argUnaryExpr->eval(parentBlock).getPtrVal());
		}
		else {
			std::cout << "It's not working :(" << std::endl;
		} 
	}
	break;
	}
	return result;
}
void UnaryExpression::accept(Visitor &v) { v.visit(this); }


// Binary Expression
BinaryExpression::BinaryExpression(const char* op, Expression* arg1, Expression* arg2) :
	op(op), arg1(arg1), arg2(arg2) {}
void BinaryExpression::print() {
	std::cout << "(";
	arg1->print();
	std::cout << " " << op << " ";
	arg2->print();
	std::cout << ")";
}
BinaryExpression::~BinaryExpression() { delete arg1; delete arg2; }
Var BinaryExpression::eval(Block* parentBlock) {
	Var result;
	Visitor v1;
	Visitor v2;
	arg1->accept(v1);
	arg2->accept(v2);

	if (*op == '+')
		result = Var(arg1->eval(parentBlock).getIntVal() + arg2->eval(parentBlock).getIntVal());
	else if (*op == '-')
		result = Var(arg1->eval(parentBlock).getIntVal() - arg2->eval(parentBlock).getIntVal());
	else if (*op == '*')
		result = Var(arg1->eval(parentBlock).getIntVal() * arg2->eval(parentBlock).getIntVal());
	else if (*op == '/')
		result = Var(arg1->eval(parentBlock).getIntVal() / arg2->eval(parentBlock).getIntVal());
	else if (*op == '%')
		result = Var(arg1->eval(parentBlock).getIntVal() % arg2->eval(parentBlock).getIntVal());
	else if (strcmp(op, "==") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() == arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (strcmp(op, "<=") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() <= arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (strcmp(op, ">=") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() >= arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (strcmp(op, "!=") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() != arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (*op == '>')
		result = Var((arg1->eval(parentBlock).getIntVal() > arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (*op == '<')
		result = Var((arg1->eval(parentBlock).getIntVal() < arg2->eval(parentBlock).getIntVal()) ? 1 : 0);
	else if (strcmp(op, "&&") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() * arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);
	else if (strcmp(op, "||") == 0)
		result = Var((arg1->eval(parentBlock).getIntVal() + arg2->eval(parentBlock).getIntVal() != 0) ? 1 : 0);

	return result;
}
void BinaryExpression::accept(Visitor &v) { v.visit(this); }


// Array at Expression
ArrayAtExpression::ArrayAtExpression(std::string ID, Expression* index) : ID(ID), index(index) {}
Var ArrayAtExpression::eval(Block* parentBlock) {
	Var result;
	Var* sourceArr = parentBlock->findVar(ID);
	if (sourceArr == nullptr)
		throw UndefinedVarException();
	else if (sourceArr->getType() == T_INT && index != nullptr) {
		throw InvalidTypeException("Int has no index");
	}
	result = Var(sourceArr->getArrAtVal(index->eval(parentBlock).getIntVal()));
	return result;
}
void ArrayAtExpression::accept(Visitor &v) { v.visit(this); }
void ArrayAtExpression::print() {
	//std::cout << ID << "[" << index->eval().getIntVal() << "]";
	std::cout << ID << "[" << "index" << "]";
}


// Value Expression
Value::Value(const std::string& val) : val(atoi(val.c_str())) {}
void Value::print() { std::cout << val; }
Var Value::eval(Block* parentBlock) {
	return Var(val);
}
void Value::accept(Visitor &v) { v.visit(this); }


// Variable Expression
VarExpression::VarExpression(const std::string& ID) : ID(ID) {}
void VarExpression::print() { std::cout << ID; }
std::string VarExpression::getID() { return ID; }
Var VarExpression::eval(Block* parentBlock) {
	Var* thisVar = parentBlock->findVar(ID);
	if (thisVar == nullptr)
		std::cout << "No such variable";
	return *thisVar;
}
void VarExpression::accept(Visitor &v) {
	v.visit(this);
}
