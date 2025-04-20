#pragma once
#include "tokenizer.hpp"
#include <vector>
#include <map>
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
		// check name validity and init
		if (!isvalidname(name))
			error("addrule", "invalid rule name: '" + name + "'");
		if (rules.count(name))
			error("addrule", "redefinition of rule: '" + name + "'");
		auto& rule = rules[name] = { name };
		// init tokenizer
		tok.reset();
		tok.tokenizeline(line);
		tok.show();
		// parse
		por(rule);
	}

	int por(Rule& rule) {
		rule.subrules.push_back({ {}, true });
		int id = rule.subrules.size() - 1;
		// first rule
		int id_and = pand(rule);
		rule.subrules.at(id).list.push_back({ "$"+to_string(id_and) });
		// second -> multiple rules
		while (tok.peek() == "|") {
			tok.get();
			int id_and = pand(rule);
			rule.subrules.at(id).list.push_back({ "$"+to_string(id_and) });
		}
		return id;
	}

	int pand(Rule& rule) {
		rule.subrules.push_back({ {}, false });
		int id = rule.subrules.size() - 1;
		// parse each Atom in rule
		while (!tok.eof()) {
			if (tok.peek() == "|" || tok.peek() == ")")  // break rule if we hit an 'or' or 'end-bracket' operator
				break;
			auto atom = patom(rule);
			rule.subrules.at(id).list.push_back(atom);
		}
		// sanity check - should be at least one rule here
		if (rule.subrules.at(id).list.size() == 0)
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

	// helpers - various definitions
	static int ismodifier(const string& s) {
		return s == "?" || s == "*" || s == "+" || s == "!";
	}
	static int isvalidname(const string& name) {
		if (name.length() < 2 || name[0] != '$')  return false;
		for (size_t i = 1; i < name.length(); i++)
			if ( !isalphanum(name[i]) )  return false;
		return true;
	}

	void test() {
		addrule("$test1", "PRINT   $hello  A 1+");
		addrule("$test2", "$hello $world | A");
		addrule("$test3", "$hello $world | PRINT $world | 1");
		addrule("$test4", "a ($addop $subop) b");
		addrule("$test5", "PRINT ($strlit | (100|101|102)+)!");

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
		throw runtime_error("ASTRule: " + type + ": " + msg);
		return false;
	}
};