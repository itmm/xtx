#include <iostream>

#include "arith.h"
#include "eval.h"
#include "read.h"
#include "print.h"

int main() {
	State_Ptr state { std::make_shared<State>() };
	add_arith(state);
	Node_Ptr root;
	while (std::cin) {
		std::cin >> root;
		root = eval(root, *state);
		std::cout << *root;
	}
	return 0;
}
