#pragma once
#include "tokenizer.hpp"
#include <vector>
#include <cassert>
using namespace std;


struct ASTParseRule : TokenHelpers {
	// struct Rule    { string name; vector<SubRule> subrules; };
	struct Atom    { string rule, mod; };
	struct Subrule { vector<Atom> list; bool flagor; };
	vector<Subrule> subrules;
	// string errormsg;
	
	Tokenizer tok;

	void addrule(const string& name, const string& line) {
		cout << "making rule: " << name << endl;
		tok.tokenizeline(line);
		tok.show();

		por();
	}

	int por() {
		pand();
		while (tok.peek() == "|") {
			tok.get();
			cout << "or" << endl;
			pand();
		}
		return -1;
	}

	int pand() {
		subrules.push_back({});
		int id = subrules.size() - 1;
		while (!tok.eof()) {
			if (tok.peek() == "|")
				break;
			auto atom = patom();
			subrules.at(id).list.push_back(atom);
		}
		assert(subrules.at(id).list.size() >= 1);
		return id;
	}

	Atom patom() {
		Atom atom;
		assert( !tok.eof() );
		assert( !ismodifier(tok.peek()) && !isoperator(tok.peek()) );
		if (tok.peek() == "$") {
			atom.rule = tok.get();
			assert(isidentifier(tok.peek()));
			atom.rule += tok.get();
			atom.mod = pmodifiers();
		}
		else {
			atom.rule = tok.get();
			atom.mod = pmodifiers();
		}
		cout << "  atom: " << atom.rule << atom.mod << endl;
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
	int isoperator(string s) {
		return s == "(" || s == ")" || s == "|";
	}

	void test() {
		addrule("test1", "PRINT   $hello  A 1+");
		addrule("test2", "$hello $world | PRINT $world | 1");
		// addrule("test1", "PRINT ($strlit | (0|1|2)+)!");
	}

	int error(const string& type, const string& msg) {
		throw runtime_error(type + ": " + msg);
		return false;
	}
};