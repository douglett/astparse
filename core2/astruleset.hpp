#pragma once
#include "tokenizer.hpp"
#include <vector>
#include <map>
#include <cassert>
using namespace std;


struct ASTRuleset : TokenHelpers {
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

	void simplify(Rule& rule) {
		// for (auto& subrule : rule.subrules) {

		// }
		cout << "simplify" << endl;

		// for (int i = rule.subrules.size() - 1; i >= 0; i--) {
		for (size_t i = 0; i < rule.subrules.size(); i++) {
			// if the current subrule has a single member, it is a target for simplifying
			auto& subrule = rule.subrules.at(i);
			if (subrule.list.size() != 1)
				continue;
			auto atom = subrule.list.at(0);

			if (isinternalrule(atom.rule) && atom.mod == "") {
				// cout << "here1 " << i << " " << atom.rule << endl;
				int i = stoi(atom.rule.substr(1));  // find target-subrule index
				// cout << i << endl;
				subrule = rule.subrules.at(i);  // replace current-subrule with target-subrule
				rule.subrules.at(i) = {};  // clear current-subrule
			}

			if (!isinternalrule(atom.rule)) {
				cout << "here2 " << i << " " << atom.rule << endl;
				for (size_t j = 0; j < i; j++) {
					for (auto& atom_replace : rule.subrules.at(j).list) {
						if (atom_replace.rule == "$"+to_string(i)) {
							atom_replace = atom;
							subrule = {};
						}
					}
				}
			}
		}
	}

	// Subrule& findsubrule(Rule& const string& subrule) {
	// 	assert(isinternalrule(subrule));
	// 	int i = stoi(rule.substr(1));
	// 	// cout << i << endl;
	// 	return rule.subrules.at(i);
	// }

	void test() {
		addrule("$test1", "PRINT   $hello  A 1+");
		addrule("$test2", "$hello $world | A*");
		addrule("$test3", "$hello $world | PRINT $world | 1");
		addrule("$test4", "a ($addop $subop) b");
		addrule("$test5", "PRINT ($strlit | (100|101|102)+)!");

		showall();
		cout << "-----\n";
		// simplify(rules.at("$test1"));
		// showrule(rules.at("$test1"));
		// simplify(rules.at("$test2"));
		// showrule(rules.at("$test2"));
		simplify(rules.at("$test4"));
		showrule(rules.at("$test4"));
	}


	// === helpers
	int error(const string& type, const string& msg) {
		throw runtime_error("ASTRule: " + type + ": " + msg);
		return false;
	}
	void showall() const {
		for (auto&[key, rule] : rules)
			showrule(rule);
	}
	void showrule(const Rule& rule) const {
		// show rulename
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
};