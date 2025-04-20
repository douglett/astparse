#pragma once
#include "tokenizer.hpp"
#include <vector>
#include <map>
#include <cassert>
using namespace std;


struct ASTParseRule : TokenHelpers {
	// struct Rule    { string name; vector<SubRule> subrules; };
	struct Atom    { string rule, mod; };
	struct Subrule { vector<Atom> list; bool flag_or; };
	struct Rule    { string name; vector<Subrule> subrules; };
	map<string, Rule> rules;
	Tokenizer tok;

	void addrule(const string& name, const string& line) {
		cout << "making rule: " << name << endl;
		assert(rules.count(name) == 0);
		auto& rule = rules[name] = { name };
		// init tokenizer
		tok.reset();
		tok.tokenizeline(line);
		tok.show();
		// parse
		// pand(rule);
		por(rule);
	}

	int por(Rule& rule) {
		rule.subrules.push_back({ {}, true });
		int id = rule.subrules.size() - 1;
		// first rule
		int andid = pand(rule);
		rule.subrules.at(id).list.push_back({ "$"+to_string(andid) });
		// second id's
		while (tok.peek() == "|") {
			tok.get();
			// cout << "or" << endl;
			int andid = pand(rule);
			rule.subrules.at(id).list.push_back({ "$"+to_string(andid) });
		}
		return id;
	}

	int pand(Rule& rule) {
		rule.subrules.push_back({ {}, false });
		int id = rule.subrules.size() - 1;
		while (!tok.eof()) {
			if (tok.peek() == "|" || tok.peek() == ")")
				break;
			auto atom = patom(rule);
			rule.subrules.at(id).list.push_back(atom);
		}
		assert(rule.subrules.at(id).list.size() >= 1);
		return id;
	}

	Atom patom(Rule& rule) {
		Atom atom;
		assert( !tok.eof() );
		// assert( !ismodifier(tok.peek()) && !isoperator(tok.peek()) );
		assert( !ismodifier(tok.peek()) && tok.peek() != "|" );
		if (tok.peek() == "$") {
			atom.rule = tok.get();
			assert(isidentifier(tok.peek()));
			atom.rule += tok.get();
			atom.mod = pmodifiers();
		}
		else if (tok.peek() == "(") {
			tok.get();
			int id_or = por(rule);
			atom.rule = "$" + to_string(id_or);
			assert(tok.peek() == ")");
			tok.get();
			atom.mod = pmodifiers();
		}
		else {
			atom.rule = tok.get();
			atom.mod = pmodifiers();
		}
		// cout << "  atom: " << atom.rule << atom.mod << endl;
		return atom;
	}

	string pmodifiers() {
		string mod;
		while (ismodifier(tok.peek()))
			mod += tok.get();
		// todo: check modifiers
		return mod;
	}

	int ismodifier(string s) {
		return s == "*" || s == "+" || s == "!";
	}
	// int isoperator(string s) {
	// 	return s == "(" || s == ")" || s == "|";
	// }

	void test() {
		addrule("test1", "PRINT   $hello  A 1+");
		addrule("test2", "$hello $world | A");
		addrule("test3", "$hello $world | PRINT $world | 1");
		addrule("test4", "a ($addop $subop) b");
		addrule("test5", "PRINT ($strlit | (100|101|102)+)!");

		showall();
	}

	void showall() const {
		for (auto&[key, value] : rules)
			showrule(key);
	}

	void showrule(const string& rulename) const {
		// show rulename
		auto& rule = rules.at(rulename);
		cout << rule.name << ":" << endl;
		// loop subrules
		for (size_t i = 0; i < rule.subrules.size(); i++) {
			cout << "   " << (rule.subrules.size() >= 10 && i < 10 ? "0" : "") << i << ": ";  // show subrule id number
			auto& subrule = rule.subrules[i];  // get subrule
			// show each rule in subrule on a line
			for (auto& atom : subrule.list) {
				cout << atom.rule << atom.mod << " ";
				if (subrule.flag_or && &atom != &subrule.list.back())  // split 'or' rules with '|' character
					cout << "| ";
			}
			cout << endl;
		}
	}

	int error(const string& type, const string& msg) {
		throw runtime_error(type + ": " + msg);
		return false;
	}
};