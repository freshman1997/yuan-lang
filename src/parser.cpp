
#include "parser.h"
#include "lex.h"

static OperatorType getbinopr (int op) {
  switch (op) {
    case '+': return OperatorType::op_add;
    case '-': return OperatorType::op_sub;
    case '*': return OperatorType::op_mul;
    case '/': return OperatorType::op_div;
    case '%': return OperatorType::op_mod;
    case '!' + '=': return OperatorType::op_ne;
    case '=' + '=': return OperatorType::op_equal;
    case '<': return OperatorType::op_lt;
	case '>': return OperatorType::op_gt;
	case '.': return OperatorType::op_dot;
	case '&' + '&': return OperatorType::op_and;
	case '|' + '|': return OperatorType::op_or;
	case '&': return OperatorType::op_bin_and;
	case '|': return OperatorType::op_bin_or;
	case '<' + '<': return OperatorType::op_bin_lm;
	case ('>' + '>' + 1): return OperatorType::op_bin_rm;
	case '^' + 1: return OperatorType::op_bin_xor;
	case '<' + '=': return OperatorType::op_lt_eq;
	case '>' + '=': return OperatorType::op_gt_eq;
    default: return OperatorType::op_none;
  }
}

static const struct {
  unsigned char left;  /* left priority for each binary operator */
  unsigned char right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {6, 6}, {6, 6}, {7, 7}, {7, 7}, {7, 7},  			/* `+' `-' `/' `%' */
   {3, 3}, {3, 3}, {3, 3}, {8, 8}, {3, 3}, {3, 3},		// |, ^, &, ~, <<, >>
   {3, 3}, {3, 3}, 										/* ==, != */
   {3, 3}, {3, 3}, {3, 3}, {3, 3}, {2, 2}, {2, 2},  	/* <, >, <=, >=, ||, && */
   {8, 7} // .
};

#define UNARY_PRIORITY	8  /* priority for unary operators */

static OperatorType getunopr (int op) {
  switch (op) {
    case '!': return OperatorType::op_not;
    case '-': return OperatorType::op_unary_sub;
    case '#': return OperatorType::op_len;
	case '+' + '+': return OperatorType::op_add_add;
	case '-' + '-': return OperatorType::op_sub_sub;
	case '~':return OperatorType::op_bin_not;
    default: return OperatorType::op_none;
  }
}

static int get_operator_type(TokenReader *reader)
{
	const Token &token = reader->peek();
	if (token.type != TokenType::sym) return -1;
	static unordered_map<string, int> symbol_map;
	if (symbol_map.empty()) {
		symbol_map["!"] = '!';
		symbol_map["#"] = '#';
		symbol_map["++"] = '+' + '+';
		symbol_map["--"] = '-' + '-';

		symbol_map["+"] = '+';
		symbol_map["-"] = '-';
		symbol_map["*"] = '*';
		symbol_map["/"] = '/';
		symbol_map["%"] = '%';
		
		symbol_map["&&"] = '&' + '&';
		symbol_map["||"] = '|' + '|';
		symbol_map[">>"] = '>' + '>' + 1;
		symbol_map["<<"] = '<' + '<';

		symbol_map["|"] = '|';
		symbol_map["&"] = '&';
		symbol_map["^"] = '^' + 1;

		symbol_map[">"] = '>';
		symbol_map["<"] = '<';
		symbol_map["<="] = '<' + '=';
		symbol_map["<="] = '>' + '=';
		symbol_map["=="] = '=' + '=';
		symbol_map["!="] = '!' + '=';

		symbol_map["."] = '.';
	}

	static string t;
	if (!t.empty()) t.clear();
	const char *p = token.from;
	for(int i = 0; i < token.len; ++i) 
		t.push_back(p[i]);
	return symbol_map.count(t) > 0 ? symbol_map[t] : -1;
}

static Operation * parse_primary(TokenReader *reader);

/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where `binop' is any binary operator with a priority higher than `limit'
*/
// 100 + 18 + 20 / 2 + 100 - 20 需要生成以下的树
		/*
										  - 
										/   \
				+					   +     20
			  /   \					 /  \
			 +		'/'			   +    100         
			/ \     /  \          /   \            
		 100   18  20   2       +      '/'   
		 					   /  \    /  \
							100	  18  20   2
							  
		*/
static OperationExpression * subexpr(TokenReader *reader, unsigned int limit) {
	OperationExpression *node = NULL;
	OperationExpression *child1 = NULL;
	if (reader->peek().type != TokenType::eof) {
		OperatorType uop = getunopr(get_operator_type(reader));
		if (uop != OperatorType::op_none) {
			reader->consume();
			child1 = new OperationExpression;
			child1->op_type = uop;
			child1->left = parse_primary(reader);
		}
		else if (!child1) {
			bool isIden = reader->peek().type == TokenType::iden;
			Operation *oper = parse_primary(reader);
			if (oper) {
				child1 = new OperationExpression;
				child1->left = oper;
				child1->op_type = OperatorType::op_none;
			}
			if (isIden) {
				uop = getunopr(get_operator_type(reader));
				if (uop != OperatorType::op_none && reader->peek().len == 2 && (str_equal(reader->peek().from, "++", 2) || str_equal(reader->peek().from, "--", 2))) {
					child1->op_type = uop;
					reader->consume();
				}
			}
		}

		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none && priority[(int)op].left > limit) {
			reader->consume();
			OperationExpression *exp = subexpr(reader, priority[(int)op].left);
			if (exp) {
				node = new OperationExpression;
				node->op_type = op;
				Operation *child2 = new Operation;
				child2->op = new Operation::oper;
				child2->op->op_oper = exp;
				child2->type = OpType::op;
				node->right = child2;

				Operation *left = new Operation;
				left->op = new Operation::oper;
				left->op->op_oper = child1;
				left->type = OpType::op;
				node->left = left;

				if (limit && limit < priority[(int)exp->op_type].left) {
					// 全部解析出来
					exp = node;
					OperatorType pre = op;
					op = getbinopr(get_operator_type(reader));
					while (op != OperatorType::op_none && priority[(int)op].left == priority[(int)pre].left)
					{
						reader->consume();
						OperationExpression *sub = subexpr(reader, priority[(int)op].left);
						if (sub) {
							node = new OperationExpression;
							node->op_type = op;
							Operation *left = new Operation;
							left->op = new Operation::oper;
							left->op->op_oper = exp;
							left->type = OpType::op;
							node->left = left;

							Operation *right = new Operation;
							right->op = new Operation::oper;
							right->op->op_oper = sub;
							right->type = OpType::op;
							node->right = right;
							exp = node;

							pre = op;
							op = getbinopr(get_operator_type(reader));
						}
						else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
					}
				}
			}
			else {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
		} 

		if (!node) node = child1;
		/*else {
			if (reader->peek().type != TokenType::keyword && !str_equal(reader->peek().from, "fn", 2)) {
				node = new OperationExpression;
				node->left = parse_primary(reader);
				node->op_type = OperatorType::op_none;
				if (!node->left) error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
		}*/
	}
	return node;
}

static OperationExpression * parse_operator(TokenReader *reader);
static Function * parse_function_expression(TokenReader *reader, bool hasName);

static vector<Operation *> * parse_parameter(TokenReader *reader, char close, bool isArr)
{
	vector<Operation *> *parameters = new vector<Operation *>;
	while (reader->peek().type != TokenType::eof) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ')') break;  // no parameter
 		OperationExpression *oper = parse_operator(reader);
		if (!isArr) {
			if (reader->peek().type != TokenType::sym || !oper)
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		else if (!oper){
			// 函数
			if (isArr && reader->peek().type == TokenType::keyword && reader->peek().len == 2 && str_equal(reader->peek().from, "fn", 2)) {
				OperationExpression *op = new OperationExpression;
				op->op_type = OperatorType::op_none;
				op->left = new Operation;
				op->left->op = new Operation::oper;
				op->left->type = OpType::function_declear;
				op->left->op->fun_oper = parse_function_expression(reader, false);
				oper = op;
			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}

		if (!oper) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		
		// fill the parameters
		if (oper->op_type == OperatorType::op_none) {
			parameters->push_back(oper->left);
			delete oper;
		}
		else {
			Operation *op = new Operation;
			op->type = OpType::op;
			op->op = new Operation::oper;
			op->op->op_oper = oper;
			parameters->push_back(op);
		}

		if (*(reader->peek().from) == close) break;
		else if (*(reader->peek().from) == ',') reader->consume();
		else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}
	return parameters;
}

static CallExpression * parse_function_call(TokenReader *reader)
{
	CallExpression *call = new CallExpression;
	if (reader->peek().type != TokenType::sym && *reader->peek().from != '(') {
		if (reader->peek().type == TokenType::keyword) {
			if (!str_equal(reader->peek().from, "require", 7)) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid require statement!", __LINE__);
			}
		}
		else {
			if (reader->peek().type != TokenType::iden) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid call statement", __LINE__);
			}
		}
		call->function_name = new IdExpression;
		call->function_name->name = reader->peek().from;
		call->function_name->name_len = reader->peek().len;
		reader->consume(); // consume function name
	}

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "function call need ( to start", __LINE__);
	}
	reader->consume(); // consume (
	call->parameters = parse_parameter(reader, ')', false);
	reader->consume(); // )
	return call;
}

static Operation * build_boolean(TokenReader *reader, bool val)
{
	Boolean *b = new Boolean;
	b->val = val;
	b->raw = reader->peek().from;
	b->raw_len = reader->peek().len;
	Operation *node = new Operation;
	node->op = new Operation::oper;
	node->type = OpType::boolean;
	node->op->boolean_oper = b;
	reader->consume();
	return node;
}

static Operation * build_id(TokenReader *reader)
{
	Operation *node = new Operation;
	node->op = new Operation::oper;
	node->type = OpType::id;
	IdExpression *idOper = new IdExpression;
	idOper->name = reader->peek().from;
	idOper->name_len = reader->peek().len;
	node->op->id_oper = idOper;
	reader->consume();	
	return node;
}

static void parse_index(TokenReader *reader, IndexExpression *index)
{
	while (reader->peek().type != TokenType::eof)
	{
		OperationExpression *oper = NULL;
		if (reader->peek().type == TokenType::sym && *reader->peek().from == '[') {
			reader->consume();
			oper = parse_operator(reader);
			if (reader->peek().type != TokenType::sym || *reader->peek().from != ']') {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
			reader->consume();
		}
		else if(reader->peek().type == TokenType::sym && *reader->peek().from == '(')
		{
			oper = new OperationExpression;
			oper->op_type = OperatorType::op_none;
			oper->left = new Operation;
			oper->left->op = new Operation::oper;
			oper->left->type = OpType::call;
			oper->left->op->call_oper = parse_function_call(reader);
		}
		else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		if (!oper) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		index->keys->push_back(oper);
		if (reader->peek().type != TokenType::sym || !(*reader->peek().from == '[' || *reader->peek().from == '(')) break;
	}

	if (reader->peek().type == TokenType::sym && *reader->peek().from == '=') {
		index->isSet = true;
	}
}

static Function * parse_function_expression(TokenReader *reader, bool hasName);

static Operation * parse_primary(TokenReader *reader)
{
	// id, number, string, array, table, function call, index, condition
	// a = +100;
	Operation *node = NULL;
	if (reader->peek().type != TokenType::eof) {
		// 数字
		// +-
		if (reader->peek().type == TokenType::num) {
			node = new Operation;
			node->op = new Operation::oper;
			node->type = OpType::num;
			Number *num = new Number;
			num->raw = reader->peek().from;
			num->raw_len = reader->peek().len;
			num->val = reader->peek().val;
			node->op->number_oper = num;
			reader->consume();
		}
		else if (reader->peek().type == TokenType::str)
		{
			node = new Operation;
			node->op = new Operation::oper;
			node->type = OpType::str;
			String *str = new String;
			str->raw = reader->peek().from;
			str->raw_len = reader->peek().len;
			node->op->string_oper = str;
			reader->consume();
		}
		else if (reader->peek().type == TokenType::iden) {
			// id(), id[], id., id++, id+, id=, 
			const Token &id_token = reader->get_and_read();
			if (reader->peek().type == TokenType::sym) {
				// ( . [ ：
				char ch = *(reader->peek().from);
				switch (ch)
				{
				case '(':	// function call
				{
					reader->unread();
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::call;
					CallExpression *call = parse_function_call(reader);
					node->op->call_oper = call;
					break;
				}
				case '[':	// index data
				{
					reader->unread();
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::index;
					IndexExpression *index = new IndexExpression;
					index->id = new IdExpression;
					index->id->name = reader->peek().from;
					index->id->name_len = reader->peek().len;
					index->keys = new vector<OperationExpression *>;
					reader->consume();

					// parse [1][2][3][4]
					parse_index(reader, index);

					node->op->index_oper = index;
					break;
				}
				case ':':	
				{
					
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
					break;
				}
				case '.':
				{
					reader->unread();
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::index;
					IndexExpression *index = new IndexExpression;
					index->id = new IdExpression;
					index->id->name = reader->peek().from;
					index->id->name_len = reader->peek().len;
					index->keys = new vector<OperationExpression *>;
					reader->consume();

					// aa.bb.cc  aa.bb[cc].dd
					bool startSubstr = false;
					while (reader->peek().type != TokenType::eof) {
						if (reader->peek().type == TokenType::sym && *reader->peek().from == '.') reader->consume();
						else break;
						if (reader->peek().type != TokenType::iden) {
							error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
						}
						OperationExpression *oper = parse_operator(reader);
						// 由生成代码那边确定逻辑是否正确
						index->keys->push_back(oper);
					}
					node->op->index_oper = index;
					break;
				}
				default:
					reader->unread();
					node = build_id(reader);
					break;
				}
			}
			else {
				reader->unread();
				node = build_id(reader);
			}
		}
		else if (reader->peek().type == TokenType::sym) {
			char ch = *(reader->peek().from);
			if (ch == '(') {		// subexp
				node = new Operation;
				node->op = new Operation::oper;
				node->type = OpType::op;
				reader->consume();
				node->op->op_oper = parse_operator(reader);
				if (reader->peek().type != TokenType::sym || *(reader->peek().from) != ')') {
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
				}
				reader->consume();
			}
			else if (ch == '[') {	// array construct
				reader->unread();
				if (reader->peek().type == TokenType::sym && reader->peek().len == 1 && *reader->peek().from == ')') {
					reader->consume();
					// ()[]
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::index;
					IndexExpression *index = new IndexExpression;
					index->id = NULL;
					index->keys = new vector<OperationExpression *>;
					parse_index(reader, index);
					node->op->index_oper = index;
				}
				else {
					reader->consume();
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::arr;
					Array *array = new Array;
					reader->consume();
					array->fields = parse_parameter(reader, ']', true);
					node->op->array_oper = array;
					reader->consume();
				}
			}
			else if (ch == '{'){	// table construct
				reader->consume();
				node = new Operation;
				node->op = new Operation::oper;
				node->type = OpType::table;
				Table *tb = new Table;
				tb->members = new vector<TableItemPair *>;
				while (reader->peek().type != TokenType::eof)
				{
					if (reader->peek().type == TokenType::sym && *reader->peek().from == '}') {
						reader->consume();
						break;
					}
					if (!(reader->peek().type == TokenType::num || reader->peek().type == TokenType::str)) {
						error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "only support number and string as table key!!!", __LINE__);
					}

					TableItemPair *pair = new TableItemPair;
					pair->k = parse_operator(reader);
					if (!pair->k || reader->peek().type != TokenType::sym || *reader->peek().from != ':') {
						error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
					}
					reader->consume();
					pair->v = parse_operator(reader);
					if (!pair->v) {
						Function *fun = parse_function_expression(reader, false);
						if (!fun) {
							error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
						}
						pair->v = new OperationExpression;
						pair->v->op_type = OperatorType::op_none;
						pair->v->left = new Operation;
						pair->v->left->type = OpType::function_declear;
						pair->v->left->op = new Operation::oper;
						pair->v->left->op->fun_oper = fun;
					}
					if (!pair->v || reader->peek().type != TokenType::sym) {
						error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
					}
					tb->members->push_back(pair);
					if (*reader->peek().from == '}') {
						reader->consume();
						break;
					}
					else if (*reader->peek().from == ',') {
						reader->consume();
					}
					else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
				}
				node->op->table_oper = tb;
			}
			// 让上层处理
			else if (*(reader->peek().from) == ']' || *(reader->peek().from)== '}' || *(reader->peek().from) == ')') {
				return node;
			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		else if (reader->peek().type == TokenType::keyword) {
			if (str_equal(reader->peek().from, "require", 7)) {
				CallExpression *call = parse_function_call(reader);
				if (!call || call->parameters->size() > 2 || call->parameters->front()->type != OpType::str) {
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
				}
				node = new Operation;
				node->op = new Operation::oper;
				node->type = OpType::call;
				node->op->call_oper = call;
			}
			else if (str_equal(reader->peek().from, "nil", 3)) {
				node = new Operation;
				node->type = OpType::nil;
				reader->consume();
			}
			else if (str_equal(reader->peek().from, "true", 4)) {
				node = build_boolean(reader, true);
			}
			else if (str_equal(reader->peek().from, "false", 5)) {
				node = build_boolean(reader, false);
 			}
			else if (str_equal(reader->peek().from, "fn", 2)) {
				return node;
 			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		else {
			// error
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		} 
	}
	return node;
}

static OperationExpression * parse_operator(TokenReader *reader)
{
	OperationExpression *node = NULL;
	OperatorType op = OperatorType::op_none;
	OperationExpression *child1 = subexpr(reader, 0);
	while (child1) {
		// 到这里应该是操作符
		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none) {
			reader->consume();
			OperationExpression *child2 = subexpr(reader, priority[(int)op].left);
			if (child2) {
				node = new OperationExpression;
				node->op_type = op;
				
				node->left = new Operation;
				node->left->type = OpType::op;
				node->left->op = new Operation::oper;
				node->left->op->op_oper = child1;

				node->right = new Operation;
				node->right->op = new Operation::oper;
				node->right->type = OpType::op;
				node->right->op->op_oper = child2;
 				child1 = node;
			}
			else {
				// TODO error
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
		}
		else {
			if (child1){
				node = child1;
			}
			break; // 只有一个
		}
	}
	return node;
}

static AssignmentExpression * parse_assignment(TokenReader *reader)
{
	bool isLocal = false;
	if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "local", 5)) {
		isLocal = true;
		reader->consume();
	}
	if (!(reader->peek().type == TokenType::iden || (reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1)))) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid assign statement", __LINE__);
	}
	
	IdExpression *module = NULL;
	reader->consume();
	if (reader->peek().type == TokenType::sym && *reader->peek().from == '.') {
		reader->unread();
		module = new IdExpression;
		module->name = reader->peek().from;
		module->name_len = reader->peek().len;
		reader->consume();
		reader->consume();
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid field index", __LINE__);
		}
	}
	else reader->unread();

	AssignmentExpression *as = new AssignmentExpression;
	if (module) {
		isLocal = true;
		as->module = module;
	}

	if (!(reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1))) {
		as->id = new IdExpression;
		as->id->is_local = isLocal;
		as->id->name = reader->peek().from;
		as->id->name_len = reader->peek().len;
		reader->consume();
	}
	
	if (reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1)) {
		reader->consume();
		const Token &val = reader->peek();
		if ( (val.type == TokenType::keyword && str_equal(val.from, "require", 7) || str_equal(val.from, "false", 5) || str_equal(val.from, "true", 4) || str_equal(val.from, "nil", 3)) || val.type == TokenType::iden || val.type == TokenType::num || val.type == TokenType::str || val.type == TokenType::sym) {
			OperationExpression *oper = parse_operator(reader);
			if (!oper) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
			as->assign = oper;
		}
		else error_tok(val, reader->get_file_name(), reader->get_content(), "%s on line: %d", "unkown type ", __LINE__);
	}
	else {
		delete as;
		as = NULL;
		reader->unread();
		if (isLocal) reader->unread();
	}
	return as;
}

// 严格要求 {} 
static OperationExpression * parse_if_prefix(TokenReader *reader, bool is_if)
{
	if(!is_if) {
		if (reader->peek().type != TokenType::keyword && reader->peek().len != 4 && !str_equal(reader->peek().from, "else", 4)) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid condition key word", __LINE__);
		}
		reader->consume(); // consume else key word
	}
	if (reader->peek().type != TokenType::keyword && reader->peek().len != 2 && !str_equal(reader->peek().from, "if", 2)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid condition key word", __LINE__);
	}

	reader->consume(); // consume if key word
	if (reader->peek().type != TokenType::sym || *(reader->peek().from) != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition statement needs starting with ( ", __LINE__);
	}
	reader->consume();

	OperationExpression *oper = parse_operator(reader);
	if (!oper) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}

	if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition statement needs ending with ) ", __LINE__);
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition body needs starting with { ", __LINE__);
	}
	reader->consume();
	return oper;
}

static vector<BodyStatment *> * parse_expressions(TokenReader *reader, bool breakable);

static void build_if_body(IfStatement *cond, TokenReader *reader, bool breakable)
{
	cond->body = parse_expressions(reader, breakable);
	if (!cond->body) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition body needs ending with } ", __LINE__);
	}
	reader->consume();
}

static void parse_if_statement(TokenReader *reader, IfExpression *cond, int start, bool breakable)
{
	IfStatement *if_statement = new IfStatement;
	if_statement->body = new vector<BodyStatment *>;
	// if else if ... else
	if (start == 0) { // if
		if_statement->condition = parse_if_prefix(reader, true);
		if (!if_statement->condition) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		build_if_body(if_statement, reader, breakable);
		if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "else", 4)) {
			reader->consume();
			if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "if", 2)) {
				reader->unread();
				parse_if_statement(reader, cond, 1, breakable);
			}
			else {
				reader->unread();
				parse_if_statement(reader, cond, 2, breakable);
			}
			
		}
	}
	else if (start == 1) { // else if
		if_statement->condition = parse_if_prefix(reader, false);
		if (!if_statement->condition) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
		}
		build_if_body(if_statement, reader, breakable);
		if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "else", 4)) {
			reader->consume();
			if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "if", 2)){
				reader->unread();
				parse_if_statement(reader, cond, 1, breakable);
			} else {
				reader->unread();
				parse_if_statement(reader, cond, 2, breakable);
			}
		}
	}
	else if (start == 2) { // else
		if (reader->peek().type != TokenType::keyword || reader->peek().len != 4 || !str_equal(reader->peek().from, "else", 4)) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid condition key word", __LINE__);
		}
		reader->consume(); // consume else 
		if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition body needs starting with { ", __LINE__);
		}
		reader->consume();
		if_statement->condition = NULL; // init
		build_if_body(if_statement, reader, breakable);
	}
	cond->if_statements->push_back(if_statement);
}

static DoWhileExpression * parse_do_while_expression(TokenReader *reader)
{
	if (reader->peek().type != TokenType::keyword || reader->peek().len != 2 || !str_equal(reader->peek().from, "do", 2)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid do while loop statement", __LINE__);
	}
	reader->consume();

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "do while loop expression needs { to start", __LINE__);
	}
	reader->consume();
	// loop body
	DoWhileExpression *doWhileExp = new DoWhileExpression;
	doWhileExp->body = parse_expressions(reader, true);
	if (!doWhileExp->body) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "loop expression needs { to close", __LINE__);
	}
	reader->consume();
	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "while", 5)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid loop expression key word", __LINE__);
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition expression needs ( to start", __LINE__);
	}
	reader->consume();
	doWhileExp->condition = parse_operator(reader);
	if (!doWhileExp->condition) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid condition statement", __LINE__);
	}
	if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition expression needs ) to close", __LINE__);
	}
	reader->consume();

	if (reader->peek().type != TokenType::sym || *reader->peek().from != ';') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "loop expression needs ; to close", __LINE__);
	}
	reader->consume();

	return doWhileExp;
}

static WhileExpression * parse_while_expression(TokenReader *reader)
{
	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "while", 5)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid while loop", __LINE__);
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition expression needs ( to start", __LINE__);
	}
	reader->consume();

	// parse condition
	WhileExpression *whileExp = new WhileExpression;
	whileExp->condition = parse_operator(reader);
	if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "condition expression needs ( to close", __LINE__);
	}
	reader->consume();

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "loop expression needs { to start", __LINE__);
	}
	reader->consume();
	// while body
	// 没办法，只能一一尝试，赋值，if，while，函数调用，do while，for
	whileExp->body = parse_expressions(reader, true);

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "loop expression needs } to close", __LINE__);
	}
	reader->consume();

	return whileExp;
}

static ForExpression * parse_for_expression(TokenReader *reader)
{
	if (reader->peek().type != TokenType::keyword || reader->peek().len != 3 || !str_equal(reader->peek().from, "for", 3)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "for expression needs ( to start", __LINE__);
	}
	reader->consume();
	ForExpression *forExp = new ForExpression;
	forExp->first_statement = new vector<OperationExpression *>;
	bool isIn = false;
	while (reader->peek().type != TokenType::eof) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ';') break;
		else if (reader->peek().type == TokenType::keyword && reader->peek().len == 2 && str_equal(reader->peek().from, "in", 2)) {
			isIn = true;
			break;
		}
		if (reader->peek().type == TokenType::sym) {
			OperationExpression *oper = parse_operator(reader);
			if (!oper) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
			}
			forExp->first_statement->push_back(oper);
		}
		else {
			AssignmentExpression *assign = parse_assignment(reader);
			if (!assign) {
				OperationExpression *oper = parse_operator(reader);
				if (!oper) error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
				forExp->first_statement->push_back(oper);
			}
			else {
				OperationExpression *oper = new OperationExpression;
				Operation *op = new Operation;
				oper->op_type = OperatorType::op_none;
				op->op = new Operation::oper;
				op->type = OpType::assign;
				op->op->assgnment_oper = assign;
				oper->left = op;
				forExp->first_statement->push_back(oper);
			}
		}
		if (reader->peek().type == TokenType::sym) {
			if (*reader->peek().from == ';') {
				break;
			}
			else if (*reader->peek().from == ',') {
				reader->consume();
			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
		}
		else if (reader->peek().type == TokenType::keyword && reader->peek().len == 2 && str_equal(reader->peek().from, "in", 2)) {
			isIn = true;
			break;
		}
		else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
	}

	reader->consume();
	if (!isIn) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ';')   // empty for condition
		{
			forExp->second_statement = new OperationExpression;
			forExp->second_statement->op_type = OperatorType::op_none;
		}
		else forExp->second_statement = parse_operator(reader);

		if (!forExp->second_statement) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
		}
		if (reader->peek().type != TokenType::sym && *reader->peek().from != ';') {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "for loop needs ; to end statement", __LINE__);
		}
		reader->consume();
		forExp->type = ForExpType::for_normal;
		forExp->third_statement = new vector<OperationExpression *>;
		while (reader->peek().type != TokenType::eof) {
			if (reader->peek().type == TokenType::sym && *reader->peek().from == ')') break;
			if (reader->peek().type == TokenType::sym) {
				OperationExpression *oper = parse_operator(reader);
				forExp->third_statement->push_back(oper);
			}
			else {
				AssignmentExpression *assign = parse_assignment(reader);
				if (!assign) {
					OperationExpression *oper = parse_operator(reader);
					if (!oper) error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
					forExp->third_statement->push_back(oper);
				}
				else {
					OperationExpression *oper = new OperationExpression;
					Operation *op = new Operation;
					oper->op_type = OperatorType::op_none;
					op->op = new Operation::oper;
					op->type = OpType::assign;
					op->op->assgnment_oper = assign;
					oper->left = op;
					forExp->third_statement->push_back(oper);
				}
			}
			if (reader->peek().type == TokenType::sym) {
				if (*reader->peek().from == ')') {
					break;
				}
				else if (*reader->peek().from == ',') {
					reader->consume();
				}
				else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
			}
			else {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
			}
		}
		reader->consume();
	}
	else {
		if (forExp->first_statement->empty()) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for in statement", __LINE__);
		}

		for (auto &it : *forExp->first_statement) {
			if (it->op_type != OperatorType::op_none || it->left->type != OpType::id) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for in statement", __LINE__);
			}
		}
		forExp->type = ForExpType::for_in;
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
		}
		
		forExp->third_statement = new vector<OperationExpression *>;
		OperationExpression *idOper = new OperationExpression;
		idOper->left = new Operation;
		idOper->op_type = OperatorType::op_none;
		idOper->left->type = OpType::id;
		idOper->left->op = new Operation::oper;
		idOper->left->op->id_oper = new IdExpression;
		idOper->left->op->id_oper->name = reader->peek().from;
		idOper->left->op->id_oper->name_len = reader->peek().len;
		forExp->third_statement->push_back(idOper);

		reader->consume();
		if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
		}
		reader->consume();
	}
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "for expression needs { to start", __LINE__);
	}
	reader->consume();

	forExp->body = parse_expressions(reader, true);
	if (!forExp->body) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid for statement", __LINE__);
	}
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "for expression needs } to close", __LINE__);
	}
	reader->consume();
	return forExp;
}

static Function * parse_function_expression(TokenReader *reader, bool hasName)
{
	bool isLocal = false;
	int pos = reader->get_pos();
	if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "local", 5)) {
		isLocal = true;
		reader->consume();
	}

	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "fn", 2))
	{
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "funcion declare needs fn ke word.", __LINE__);
	}
	reader->consume();

	if (hasName) {
		// identifier 的合法性由词法分析那边保证
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid funcion name", __LINE__);
		}
		reader->consume();
	}
	IdExpression *moduleName = NULL;
	if (reader->peek().type == TokenType::sym && *reader->peek().from == ':') {
		isLocal = true;
		if (!hasName) {	 // 模块定义的函数必须是 local 的
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid module declaration!", __LINE__);
		}
		reader->unread();
		moduleName = new IdExpression;
		moduleName->name = reader->peek().from;
		moduleName->name_len = reader->peek().len;
		reader->consume();
		reader->consume();
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid funcion name", __LINE__);
		}
	}

	Function *fun = new Function;
	if (moduleName) fun->module = moduleName;

	if (hasName) {
		if (!moduleName) reader->unread();
		IdExpression *funcName = new IdExpression;
		funcName->name = reader->peek().from;
		funcName->name_len = reader->peek().len;
		fun->function_name = funcName;
		reader->consume();
	}

	fun->is_local = isLocal;

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "funcion declare needs ( to start.", __LINE__);
	}
	reader->consume();

	// parse parameter
	int maxParam = 30; // 最多30 个参数
	int count = 0;
	fun->parameters = new vector<IdExpression *>;
	while (reader->peek().type != TokenType::eof) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ')') break;
		if (reader->peek().type == TokenType::sym && reader->peek().len == 3 && str_equal(reader->peek().from, "...", 3)) {
			// 可变参数
			fun->parameters->push_back(NULL);
			reader->consume();
			
			if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "varargs funcion parameter on support at the end of the parameters", __LINE__);
			}
			break;
		}
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid funcion parameter", __LINE__);
		}
		IdExpression *param = new IdExpression;
		param->name = reader->peek().from;
		param->name_len = reader->peek().len;
		fun->parameters->push_back(param);
		reader->consume();
		if (reader->peek().type == TokenType::sym)
		{
			if (*reader->peek().from == ',') {
				reader->consume();
			}
			else if (*reader->peek().from == ')') break;
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "unkown symbol", __LINE__);
		}
		++count;
		if (count >= maxParam) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "parameter's number exceed 30", __LINE__);
		}
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "funcion declare needs { to start", __LINE__);
	}
	reader->consume();
	// parse function body
	fun->body = parse_expressions(reader, false);
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "funcion declare needs } to close", __LINE__);
	}
	reader->consume();
	return fun;
}

static ReturnExpression *parse_return_statement(TokenReader *reader)
{
	// id，基础类型的直接构造，index，field，oper，
	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "return", 6)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid return statement", __LINE__);
	}
	reader->consume();
	ReturnExpression *retExp = new ReturnExpression;
	retExp->statement = parse_operator(reader);
	return retExp;
}

static void build_call(vector<BodyStatment *> *statements, TokenReader *reader)
{
	CallExpression *call = parse_function_call(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->call_exp = call;
	statement->type = ExpressionType::call_statement;
	statements->push_back(statement);
}

static void build_assign(vector<BodyStatment *> *statements, TokenReader *reader)
{
	AssignmentExpression *ass = parse_assignment(reader);
	if (!ass) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
	}
	bool islocal = false;
	if (ass->id) islocal = ass->id->is_local;
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->assign_exp = ass;
	statement->type = ExpressionType::assignment_statement;
	statements->push_back(statement);
	while (reader->peek().type != TokenType::eof) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ',') {
			reader->consume();
			ass = parse_assignment(reader);
			if (!ass) {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
			ass->id->is_local = islocal;
			statement = new BodyStatment;
			statement->body = new BodyStatment::body_expression;
			statement->body->assign_exp = ass;
			statement->type = ExpressionType::assignment_statement;
			statements->push_back(statement);
		}
		else break;
	}
}

static void build_function(vector<BodyStatment *> *statements, TokenReader *reader)
{
	Function *fun = parse_function_expression(reader, true);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->function_exp = fun;
	statement->type = ExpressionType::function_declaration_statement;
	statements->push_back(statement);
}

static void build_if(vector<BodyStatment *> *statements, TokenReader *reader, bool breakable)
{
	IfExpression *ifExp = new IfExpression;
	ifExp->if_statements = new vector<IfStatement *>;
	parse_if_statement(reader, ifExp, 0, breakable);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->if_exp = ifExp;
	statement->type = ExpressionType::if_statement;
	statements->push_back(statement);
}

static void build_return(vector<BodyStatment *> *statements, TokenReader *reader)
{
	ReturnExpression *returnExp = parse_return_statement(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->return_exp = returnExp;
	statement->type = ExpressionType::return_statement;
	statements->push_back(statement);
}

static void build_for(vector<BodyStatment *> *statements, TokenReader *reader)
{
	ForExpression *forExp = parse_for_expression(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->for_exp = forExp;
	statement->type = ExpressionType::for_statement;
	statements->push_back(statement);
}

static void build_while(vector<BodyStatment *> *statements, TokenReader *reader)
{
	WhileExpression *whileExp = parse_while_expression(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->while_exp = whileExp;
	statement->type = ExpressionType::while_statement;
	statements->push_back(statement);
}

static void build_do_while(vector<BodyStatment *> *statements, TokenReader *reader)
{
	DoWhileExpression *doWhileExp = parse_do_while_expression(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->do_while_exp = doWhileExp;
	statement->type = ExpressionType::do_while_statement;
	statements->push_back(statement);
}

static void build_operation(vector<BodyStatment *> *statements, TokenReader *reader)
{
	OperationExpression *oper = parse_operator(reader);
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->oper_exp = oper;
	statement->type = ExpressionType::oper_statement;
	statements->push_back(statement);
}

static vector<BodyStatment *> * parse_expressions(TokenReader *reader, bool breakable)
{
	vector<BodyStatment *> *statements = new vector<BodyStatment *>;
	while (reader->peek().type != TokenType::eof) {
		switch (reader->peek().type)
		{
		case TokenType::iden:
		{
			reader->consume();
			if (reader->peek().type == TokenType::sym) 	{
				if (*reader->peek().from == '(' && reader->peek().len == 1) {
					reader->unread();
					build_call(statements, reader);
				}
				else if (*reader->peek().from == '[' && reader->peek().len == 1) {
					reader->unread();
					build_operation(statements, reader);
				}
				else if (*reader->peek().from == '=' && reader->peek().len == 1) {
					reader->unread();
					build_assign(statements, reader);
				}
				else if (*reader->peek().from == '.' && reader->peek().len == 1) {
					// aa.bb.cc() ?
					// 如果发现前面都未定义，则认为在env中
					// 4种情况要处理，1、aa.bb.cc = 100, 2、aa.bb.vv(), 3、aa.bb.cc[100].dd[qq].ff
					reader->unread();
					build_operation(statements, reader);
				}
				else if (reader->peek().len == 2 && (str_equal(reader->peek().from, "++", 2) || str_equal(reader->peek().from, "--", 2))) {
					reader->unread();
					build_operation(statements, reader);
				}
				else {
					reader->unread();
					build_operation(statements, reader);
				}
			}
			else {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
			break;
		}
		case TokenType::sym:
			if (reader->peek().len == 2 && (str_equal(reader->peek().from, "++", 2) || str_equal(reader->peek().from, "--", 2))) {
				build_operation(statements, reader);
			}
			else if (*reader->peek().from == ']' || *reader->peek().from == ')' || *reader->peek().from == '}') {
				return statements;
			} 
			else if(reader->peek().len == 2 && (str_equal(reader->peek().from, "++", 2) || str_equal(reader->peek().from, "--", 2))) {	// 下面为 前置 ++， --
				build_operation(statements, reader);
			}
			else if (reader->peek().len == 1 && *reader->peek().from == '=')
			{
				build_assign(statements, reader);
			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			break;
		case TokenType::keyword:
		{
			if (str_equal(reader->peek().from, "if", 2)) {
				build_if(statements, reader, breakable);
			}
			else if (str_equal(reader->peek().from, "fn", 2)) {
				build_function(statements, reader);
			}
			else if (str_equal(reader->peek().from, "local", 5)){
				int pos = reader->get_pos();
				reader->consume();
				if (str_equal(reader->peek().from, "fn", 2)) {
					reader->set_pos(pos);
					build_function(statements, reader);
				}
				else if (reader->peek().type == TokenType::iden) {
					reader->consume();
					if (*reader->peek().from == '=') {
						reader->set_pos(pos);
						build_assign(statements, reader);
					}
					else if (*reader->peek().from == '(') {
						reader->set_pos(pos);
						build_call(statements, reader);
					}
					else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
				}
				else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			}
			else if (str_equal(reader->peek().from, "return", 6)) {
				build_return(statements, reader);
			}
			else if (str_equal(reader->peek().from, "while", 5)) {
				build_while(statements, reader);
			}
			else if (str_equal(reader->peek().from, "do", 2)) {
				build_do_while(statements, reader);
			}
			else if (str_equal(reader->peek().from, "for", 3)) {
				build_for(statements, reader);
			}
			else if (breakable && str_equal(reader->peek().from, "break", 5)) {
				BodyStatment *breakStatement = new BodyStatment;
				breakStatement->type = ExpressionType::break_statement;
				statements->push_back(breakStatement);
				reader->consume();
			}
			else if (breakable && str_equal(reader->peek().from, "continue", 8)) {
				BodyStatment *breakStatement = new BodyStatment;
				breakStatement->type = ExpressionType::continue_statement;
				statements->push_back(breakStatement);
				reader->consume();
			}
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			break;
		}
		default:
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s on line: %d", "invalid statement", __LINE__);
			break;
		}
	}
	return statements;
}

unordered_map<string, Chunck *> * parse(unordered_map<string, TokenReader *> &files)
{
	unordered_map<string, Chunck *> *chunks = new unordered_map<string, Chunck *>;
	for(auto &it : files) {
		Chunck *chunk = new Chunck;
		chunk->statements = parse_expressions(it.second, false);
		chunks->insert(std::make_pair(it.first, chunk));
		chunk->reader = it.second;
		cout << it.first << "\t OK" << endl;
	}
	return chunks;
}
