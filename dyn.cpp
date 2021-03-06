#include <cassert>
#include "dyn.h"
#include "err.h"
#include "invocation.h"
#include "map.h"
#include "token.h"
#include "eval.h"

static Invocation::Iter eat_space(Invocation::Iter it, Invocation::Iter end) {
	while (it != end && (**it).as_space()) { ++it; }
	return it;
}

Node_Ptr Dynamic::eval(Node_Ptr invocation, Node_Ptr state) const {
	auto dyn_state { std::make_shared<Map>(state_) };
	auto inv { invocation->as_invocation() };
	assert(inv);
	auto it { inv->begin() };
	auto end { inv->end() };
	it = eat_space(it, end);
	if (it != end) {
		dyn_state->as_map()->push(*it++, "self");
	} else { err("dyn: empty invocation"); }
	for (;;) {
		it = eat_space(it, end);
		if (it == end) { break; }
		Node_Ptr key { *it++ };
		const Token *key_token { key->as_token() };
		if (key_token && ! key_token->token().empty() &&
			key_token->token().back() == ':'
		) {
			Node_Ptr arg { parameters_->as_map()->find(
				key_token->token()
			) };
			if (! arg) {
				err("unknown argument '" + key_token->token() + "'");
			}
			if (! arg->as_token()) {
				err("argument for '" + key_token->token() +
					"' is no token"
				);
			}
			it = eat_space(it, end);
			if (it != end) {
				Node_Ptr value { *it++ };
				value = ::eval(value, state);
				dyn_state->as_map()->push(
					value, arg->as_token()->token()
				);
				continue;
			}
		}
		err("dyn: invalid map pair");
	}
	Node_Ptr result = ::eval(body_, dyn_state);
	return result;
}

class Dyn_Def: public Command {
public:
	[[nodiscard]] Node_Ptr eval(
		Node_Ptr invocation, Node_Ptr state
	) const override;
};

static bool is_key(
	const std::string &key, Invocation::Iter it, Invocation::Iter end
) {
	return it != end && (**it).as_token() &&
		(**it).as_token()->token() == key;
}

Node_Ptr Dyn_Def::eval(Node_Ptr invocation, Node_Ptr state) const {
	Node_Ptr name;
	Node_Ptr value;
	auto inv { *invocation->as_invocation() };
	auto it { inv.begin() };
	auto end { inv.end() };
	it = eat_space(it, end);
	++it;
	it = eat_space(it, end);
	if (is_key("name:", it, end)) {
		++it;
		it = eat_space(it, end);
		if (it != end) {
			name = *it++;
		} else { err("def: no name value"); }
	} else { err("def: no name"); }
	it = eat_space(it, end);
	if (is_key("as:", it, end)) {
		++it;
		it = eat_space(it, end);
		if (it != end) {
			value = ::eval(*it++, state);
		} else { err("def: no value 1"); }
	} else { err("def: no value 2"); }
	it = eat_space(it, end);
	if (it != end) { err("def: too many arguments"); }
	auto key { name->as_token() };
	if (! key) { err("def: name is no token"); }
	auto tok { key->token() };
	if (tok.empty()) { err("def: invalid key"); }
	state->as_map()->push(value, tok);
	return name;
}

class Dyn_Fn: public Command {
public:
	[[nodiscard]] Node_Ptr eval(
		Node_Ptr invocation, Node_Ptr state
	) const override;
};

Node_Ptr Dyn_Fn::eval(Node_Ptr invocation, Node_Ptr state) const {
	Node_Ptr params;
	Node_Ptr body;
	auto inv { *invocation->as_invocation() };
	auto it { inv.begin() };
	auto end { inv.end() };
	it = eat_space(it, end);
	++it;
	it = eat_space(it, end);
	if (is_key("with-params:", it, end )) {
		++it;
		it = eat_space(it, end);
		if (it != end) {
			params = ::eval(*it++, state);
		} else { err("dyn: no params value"); }
	}
	it = eat_space(it, end);
	if (is_key("as:", it, end)) {
		++it;
		it = eat_space(it, end);
		if (it != end) {
			body = *it++;
		} else { err("dyn: no body value"); }
	} else { err("dyn: no body"); }
	it = eat_space(it, end);
	if (it != end) { err("def: too many arguments"); }
	return std::make_shared<Dynamic>(params, body, state);
}

void add_dyn_commands(const Node_Ptr &state) {
	Map *m { state->as_map() };
	m->push(std::make_shared<Dyn_Def>(), "def");
	m->push(std::make_shared<Dyn_Fn>(), "fn");
}
