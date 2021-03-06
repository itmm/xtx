#include "arith.h"
#include "bool.h"
#include "cmd.h"
#include "err.h"
#include "eval.h"
#include "invocation.h"
#include "map.h"
#include "num.h"
#include "token.h"

static Invocation::Iter eat_space(Invocation::Iter it, Invocation::Iter end) {
	while (it != end && (**it).as_space()) { ++it; }
	return it;
}

static Invocation::Iter parse_one_arg(
	const std::string &key, double &value, const Node_Ptr& state,
	Invocation::Iter it, Invocation::Iter end
) {
	it = eat_space(it, end);
	if (it != end && (**it).as_token() && (**it).as_token()->token() == key) {
		++it;
	} else { err("expected key '" + key + "'"); }
	it = eat_space(it, end);
	if (it == end) { err("expected value after key '" + key + "'"); }
	Node_Ptr val { ::eval(*it, state) };
	if (val && val->as_number()) {
		value = val->as_number()->value();
		++it;
	} else { err("no value after key '" + key + "'"); }
	return it;
}

static void parse_two_args_cmd(
	const Node_Ptr& invocation,
	const std::string &key1, double &value1,
	const std::string &key2, double &value2,
	const Node_Ptr& state
) {
	auto inv { *invocation->as_invocation() };
	auto it { inv.begin() };
	auto end { inv.end() };
	it = eat_space(it, end);
	if (it != end) {
		++it;
	} else { err("empty invocation"); }
	it = parse_one_arg(key1, value1, state, it, end);
	it = parse_one_arg(key2, value2, state, it, end);
	it = eat_space(it, end);
	if (it != end) { err("too many parameters"); }
}

class Arith_Add: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Add::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"to:", to, state
	);
	return std::make_shared<Number>(value + to);
}

class Arith_Sub: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Sub::eval(Node_Ptr invocation, Node_Ptr state) const {
	double from, value;
	parse_two_args_cmd(
		invocation, "from:", from,
		"value:", value, state
	);
	return std::make_shared<Number>(from - value);
}

class Arith_Mult: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Mult::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, with;
	parse_two_args_cmd(
		invocation, "value:", value,
		"with:", with, state
	);
	return std::make_shared<Number>(value * with);
}

class Arith_Div: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Div::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, by;
	parse_two_args_cmd(
		invocation, "value:", value,
		"by:", by, state
	);
	return std::make_shared<Number>(value / by);
}

class Arith_Equals: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Equals::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value == to ? bool_true : bool_false;
}

class Arith_Not_Equals: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Not_Equals::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value != to ? bool_true : bool_false;
}

class Arith_Greater: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Greater::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value > to ? bool_true : bool_false;
}

class Arith_Greater_Or_Equals: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Greater_Or_Equals::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value >= to ? bool_true : bool_false;
}

class Arith_Smaller: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Smaller::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value < to ? bool_true : bool_false;
}

class Arith_Smaller_Or_Equals: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

Node_Ptr Arith_Smaller_Or_Equals::eval(Node_Ptr invocation, Node_Ptr state) const {
	double value, to;
	parse_two_args_cmd(
		invocation, "value:", value,
		"compared-to:", to, state
	);
	return value <= to ? bool_true : bool_false;
}

void add_arith(const Node_Ptr &state) {
	Map *s { state->as_map() };
	s->push(std::make_shared<Arith_Add>(), "add");
	s->push(std::make_shared<Arith_Sub>(), "subtract");
	s->push(std::make_shared<Arith_Mult>(), "multiply");
	s->push(std::make_shared<Arith_Div>(), "divide");
	s->push(std::make_shared<Arith_Equals>(), "equals?");
	s->push(std::make_shared<Arith_Not_Equals>(), "not-equals?");
	s->push(std::make_shared<Arith_Greater>(), "greater?");
	s->push(std::make_shared<Arith_Greater_Or_Equals>(), "greater-or-equals?");
	s->push(std::make_shared<Arith_Smaller>(), "smaller?");
	s->push(std::make_shared<Arith_Smaller_Or_Equals>(), "smaller-or-equals?");
}
