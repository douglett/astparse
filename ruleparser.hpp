#include "tokenizer.hpp"
#include <map>
#include <exception>
#include <algorithm>
#include <cassert>
using namespace std;


struct Ruleset : TokenHelpers {
	struct Rule     { string type; vector<string>list; };
	struct RuleExpr { string name; char expr; bool require; };
	static inline const vector<string> 
		RULE_TYPES       = { "and", "or" },
		RULE_EXPRESSIONS = { "*", "?", "+" },
		RULE_PREDEF      = { "$eof", "$eol", "$opplus", "$opmultiply", "$opdollar", "$identifier", "$stringliteral", "$integer" };
	string name;
	map<string, Rule> rules;


	// === build ruleset ===
	int add(const string& name, const string& list, const string& type = "and") {
		if ( rules.count(name) )
			return error( "add", "duplicate rule name: " + name );
		rules[ name ] = { type, splitstr(list) };
		return true;
	}

	int validate() {
		for (const auto& [name, rule] : rules) {
			// check name
			if ( !isvalidname(name) )
				return error( "validate", name + ": bad rule name" );
			// check rule type
			if ( !isruletype(rule.type) )
				return error( "validate", name + ": unknown rule type: " + rule.type );
			// make sure the $program rule is defined
			if ( !rules.count("$program") )
				return error( "validate", "missing entry rule $program" );
			// check individual rules exist
			for (auto& rexstr : rule.list) {
				auto rex = splitruleexpr(rexstr);
				if      ( rex.name == "" )  return error( "validate", name + ": empty rule" );
				else if ( ispredef( rex.name ) ) ;
				else if ( isuserdef( rex.name ) ) ;
				else if ( !isrulename( rex.name ) ) ;
				else    return error( "validate", name + ": unknown or invalid rule: " + rex.name );
			}
		}
		printf("ruleset validated!\n");
		return true;
	}

	int isrulename(const string& name) {
		return name.length() >= 2 && name[0] == '$';
	}
	int isvalidname(const string& name) {
		if (name.length() < 2)  return false;
		if (name[0] != '$')  return false;
		for (size_t i = 1; i < name.length(); i++)
			if ( !isalphanum(name[i]) )  return false;
		return true;
	}
	int isruletype(const string& type) {
		return find( RULE_TYPES.begin(), RULE_TYPES.end(), type ) != RULE_TYPES.end();
	}
	int isruleexpr(const string& ex) {
		return find( RULE_EXPRESSIONS.begin(), RULE_EXPRESSIONS.end(), ex ) != RULE_EXPRESSIONS.end();
	}
	int ispredef(const string& name) {
		return find( RULE_PREDEF.begin(), RULE_PREDEF.end(), name ) != RULE_PREDEF.end();
	}
	int isuserdef(const string& name) {
		return rules.count( name );
	}

	RuleExpr splitruleexpr(string rexstr) {
		RuleExpr rex = { "", 0, 0 };
		if (rexstr.length() && rexstr.back() == '!')
			rex.require = true, rexstr.pop_back();
		if (rexstr.length() && isruleexpr( string()+rexstr.back() ) )
			rex.expr = rexstr.back(), rexstr.pop_back();
		rex.name = rexstr;
		return rex;
	}


	//  === helpers ===
	void show() {
		printf(":: %s ::\n", name.c_str());
		for (const auto& [name, rule] : rules) {
			printf("%s: (%s)\n    ", name.c_str(), rule.type.c_str());
			for (auto& r : rule.list)
				printf("%s ", r.c_str());
			printf("\n");
		} 
	}

	int error(const string& rule, const string& msg) {
		throw runtime_error( rule + " error: " + msg );
		return false;
	}
};


struct Json {
	enum JTYPE { JNULL, JNUMBER, JSTRING, JARRAY, JOBJECT };
	JTYPE type; double num; string str; vector<Json> arr; map<string, Json> obj;
};


struct Parser : TokenHelpers {
	Tokenizer tok;
	Ruleset ruleset;
	Json ast;

	int parse(const string& fname) {
		// tokenize
		printf("-----\n");
		printf("loading file: %s\n", fname.c_str());
		if ( !tok.tokenize(fname) )
			error("parse", tok.errormsg);
		tok.show();
		// parse program
		ast = { Json::JARRAY };
		if (!pruleexpr("$program", ast))
			return error("$program", "unknown error parsing $program");
		printf("file parsed successfully!\n");
		// ok
		return true;
	}

	int pruleexpr(const string& ruleexpr, Json& parent) {
		// printf("parsing RuleExpr: '%s' @ Line %d '%s'\n", ruleexpr.c_str(), tok.linepos(), tok.peek().c_str());
		bool found = false;
		auto rex = ruleset.splitruleexpr(ruleexpr);
		// run the rule expression
		switch (rex.expr) {
			// match
			case 0:
				found = prule(rex.name, parent);
				break;
			// 0-to-many
			case '*':
				while (prule(rex.name, parent)) ;
				found = true;
				break;
			// 0-to-1
			case '?':
				prule(rex.name, parent);
				found = true;
				break;
			// 1-to-many
			case '+':
				while (prule(rex.name, parent))
					found = true;
				break;
			// unknown error
			default:
				return error( "pruleexpr", "unexpected error" );
		}
		// found action
		if (found)
			printf("found RuleExpr: '%s' @ Line %d\n", ruleexpr.c_str(), tok.linepos());
		return found;
	}

	int prule(const string& name, Json& parent) {
		// built in rules
		if (name == "$eof")
			return tok.eof() ? paccepttok(parent, name) : false;
		else if (name == "$eol")
			return tok.peek() == "$EOL" ? paccepttok(parent, name) : false;
		else if (name == "$opplus")
			return tok.peek() == "+" ? paccepttok(parent, name) : false;
		else if (name == "$opmultiply")
			return tok.peek() == "*" ? paccepttok(parent, name) : false;
		else if (name == "$opdollar")
			return tok.peek() == "$" ? paccepttok(parent, name) : false;
		else if (name == "$identifier")
			return isidentifier(tok.peek()) ? paccepttok(parent, name) : false;
		else if (name == "$stringliteral")
			return isliteral(tok.peek()) ? paccepttok(parent, name) : false;
		else if (name == "$integer")
			return isnumber(tok.peek()) ? paccepttok(parent, name) : false;

		// string match
		else if (!ruleset.isrulename(name))
			return tok.peek() == name ? paccepttok(parent, name) : false;

		// user defined rules
		else if (ruleset.isuserdef(name)) {
			const auto& rule = ruleset.rules[name];
			int pos = tok.pos;
			parent.arr.push_back({ Json::JOBJECT });
			auto& obj = parent.arr.back();
			obj.obj["type"] = { Json::JSTRING, 0, name };
			obj.obj["value"] = { Json::JARRAY };
			auto& js = obj.obj["value"]; 
			// and
			if (rule.type == "and") {
				for (auto& subrule : rule.list)
					if (!pruleexpr(subrule, js))
						return tok.pos = pos, parent.arr.pop_back(), false;
				return true;
			}
			// or
			else if (rule.type == "or") {
				for (auto& subrule : rule.list)
					if (pruleexpr(subrule, js))
						return true;
					else
						tok.pos = pos;
				return parent.arr.pop_back(), false;
			}
		}

		// unknown rule error
		return error("prule", "unexpected error");
	}

	int paccepttok(Json& parent, const string& type) {
		assert(parent.type == Json::JARRAY);
		int lpos = tok.linepos();
		auto token = tok.get();
		parent.arr.push_back({ Json::JOBJECT });
		auto& obj = parent.arr.back();
		obj.obj["type"] = { Json::JSTRING, 0, type };
		obj.obj["value"] = { Json::JSTRING, 0, token };
		printf("  accept-tok: %s  (line %d)\n", token.c_str(), lpos);
		return true;
	}

	//  === helpers ===
	void show() {
		showjson(ast);
	}
	void showjson(const Json& json, int ind=0) {
		string indent(ind, '.');
		switch (json.type) {
			case Json::JNULL:
				printf("NULL");
				break;
			case Json::JSTRING:
				printf("%s", json.str.c_str());
				break;
			case Json::JNUMBER:
				printf("%f", json.num);
				break;
			case Json::JARRAY:
				for (size_t i = 0; i < json.arr.size(); i++) {
					auto& js = json.arr[i];
					printf("%s.%d: ", indent.c_str(), int(i));
					if (js.type == Json::JNULL || js.type == Json::JSTRING || js.type == Json::JNUMBER)
						showjson(js, ind+1),
						printf("\n");
					else if (js.type == Json::JARRAY)
						printf("[Array]\n"),
						showjson(js, ind+1);
					else if (js.type == Json::JOBJECT)
						printf("[Object]\n"),
						showjson(js, ind+1);
				}
				break;
			case Json::JOBJECT:
				for (auto& pair : json.obj) {
					printf("%s.%s: ", indent.c_str(), pair.first.c_str());
					if (pair.second.type == Json::JNULL || pair.second.type == Json::JSTRING || pair.second.type == Json::JNUMBER)
						showjson(pair.second, ind+1),
						printf("\n");
					else if (pair.second.type == Json::JARRAY)
						printf("[Array]\n"),
						showjson(pair.second, ind+1);
					else if (pair.second.type == Json::JOBJECT)
						printf("[Object]\n"),
						showjson(pair.second, ind+1);
				}
				break;
		}
	}

	int error(const string& rule, const string& msg) {
		throw runtime_error(rule + " error: " + msg);
		return false;
	}
};
