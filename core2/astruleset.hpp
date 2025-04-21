#pragma once
#include "tokenizer.hpp"
#include <vector>
#include <map>
#include <cassert>
using namespace std;


struct ASTRuleset : TokenHelpers {
	struct Atom    { string rule, mod; };
	struct Subrule { vector<Atom> atoms; bool flag_or; };
	struct Rule    { string name; map<int, Subrule> subrules; };
	map<string, Rule> rules;
	Tokenizer tok;
	int srindex = 0;

	void addrule(const string& name, const string& line) {
		cout << "making rule: " << name << endl;
		// check name validity and init
		if (!isvalidname(name))
			error("addrule", "invalid rule name: '" + name + "'");
		if (rules.count(name))
			error("addrule", "redefinition of rule: '" + name + "'");
		auto& rule = rules[name] = { name };
		srindex = 0;
		// init tokenizer
		tok.reset();
		tok.tokenizeline(line);
		tok.show();
		// parse
		por(rule);
	}

	int por(Rule& rule) {
		int id = srindex++;
		rule.subrules[id] = { {}, true };
		// parse multiple 'and' rules seperated by '|' deliminator
		int id_and = 0;
		while (true) {
			id_and = pand(rule);
			// if 'and' rule has a single Atom, just add it directly, and erase 
			if (rule.subrules.at(id_and).atoms.size() == 1) {
				rule.subrules.at(id).atoms.push_back( rule.subrules.at(id_and).atoms.at(0) );
				rule.subrules.erase(id_and);
			}
			else
				rule.subrules.at(id).atoms.push_back({ "$"+to_string(id_and) });
			// second -> multiple rules
			if (tok.peek() == "|") {
				tok.get();
				continue;
			}
			break;
		}
		// if 'or' rule has a single 'and' rule, erase 'or' and return it
		// if (rule.subrules.at(id).atoms.size() == 1 && isinternalrule(rule.subrules.at(id).atoms.at(0).rule, id_and)) {
		// 	rule.subrules.erase(id);
		// 	return id_and;
		// }
		return id;
	}

	int pand(Rule& rule) {
		int id = srindex++;
		rule.subrules[id] = { {}, false };
		// parse each Atom in rule
		while (!tok.eof()) {
			if (tok.peek() == "|" || tok.peek() == ")")  // break rule if we hit an 'or' or 'end-bracket' operator
				break;
			auto atom = patom(rule);
			rule.subrules.at(id).atoms.push_back(atom);
		}
		// sanity check - should be at least one rule here
		if (rule.subrules.at(id).atoms.size() == 0)
			error("pand", "expected at least one rule");
		return id;
	}

	Atom patom(Rule& rule) {
		Atom atom;
		// error checking
		if (tok.eof() || ismodifier(tok.peek()) || tok.peek() == "|" || tok.peek() == ")") {
			error("patom", "unexpected in atom: '" + tok.get() + "'");
		}
		// $rulename
		else if (tok.peek() == "$") {
			atom.rule = tok.get();
			if (!isidentifier(tok.peek()))
				error("patom", "expected $(rulename), got: '" + tok.peek() + "'");
			atom.rule += tok.get();
			atom.mod = pmodifiers();
		}
		// bracketed rules
		else if (tok.peek() == "(") {
			tok.get();
			int id_or = por(rule);
			atom.rule = "$" + to_string(id_or);
			if (tok.peek() != ")")
				error("patom", "expected close-parenthasis, got: '" + tok.peek() + "'");
			tok.get();
			atom.mod = pmodifiers();
		}
		// string match, punctuation, or anything else
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
		if (mod.size() > 2 || (mod.size() == 2 && mod != "+!"))
			error("pmodifiers", "bad modifers: '" + mod + "'");
		return mod;
	}

	void test() {
		addrule("$test1", "PRINT   $hello  A 1+");
		addrule("$test2", "$hello $world | A*");
		addrule("$test3", "$hello $world | PRINT $world | 1");
		addrule("$test4", "a ($addop $subop) b");
		addrule("$test5", "PRINT ($strlit | (100|101|102)+)!");
		addrule("$test6", "ass");
		addrule("$test7", "a (ass)");

		showall();
	}


	// === helpers
	int error(const string& type, const string& msg) {
		throw runtime_error("ASTRule: " + type + ": " + msg);
		return false;
	}
	void showall() const {
		for (auto&[rulename, rule] : rules)
			showrule(rule);
	}
	void showrule(const Rule& rule) const {
		// show rulename
		cout << rule.name << ":" << endl;
		// loop subrules
		for (auto&[i, subrule] : rule.subrules) {
			cout << "   " << (rule.subrules.size() >= 10 && i < 10 ? "0" : "") << i << ": ";  // show subrule id number
			// show each rule in subrule on a line
			for (auto& atom : subrule.atoms) {
				cout << atom.rule << atom.mod << " ";
				if (subrule.flag_or && &atom != &subrule.atoms.back())  // split 'or' rules with '|' character
					cout << "| ";
			}
			cout << endl;
		}
	}


	// === Definitions
	static int ismodifier(const string& s) {
		return s == "?" || s == "*" || s == "+" || s == "!";
	}
	static int isvalidname(const string& name) {
		if (name.length() < 2 || name[0] != '$')  return false;
		for (size_t i = 1; i < name.length(); i++)
			if ( !isalphanum(name[i]) )  return false;
		return true;
	}
	static int isrulename(const string& name) {
		if (name.length() < 2 || name[0] != '$')  return false;
		if (!isidentifier(name.substr(1)))  return false;
		return true;
	}
	static int isinternalrule(const string& name) {
		if (name.length() < 2 || name[0] != '$')  return false;
		if (!isnumber(name.substr(1)))  return false;
		return true;
	}
	static int isinternalrule(const string& name, int& i) {
		if (name.length() < 2 || name[0] != '$')  return false;
		auto num = name.substr(1);
		if (!isnumber(num))  return false;
		i = stoi(num);
		return true;
	}
};