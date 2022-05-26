#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
	using namespace token_type;

	if (lhs.index() != rhs.index()) {
		return false;
	}
	if (lhs.Is<Char>()) {
		return lhs.As<Char>().value == rhs.As<Char>().value;
	}
	if (lhs.Is<Number>()) {
		return lhs.As<Number>().value == rhs.As<Number>().value;
	}
	if (lhs.Is<String>()) {
		return lhs.As<String>().value == rhs.As<String>().value;
	}
	if (lhs.Is<Id>()) {
		return lhs.As<Id>().value == rhs.As<Id>().value;
	}
	return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
	return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
	using namespace token_type;

#define VALUED_OUTPUT(type) \
	if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

	VALUED_OUTPUT(Number);
	VALUED_OUTPUT(Id);
	VALUED_OUTPUT(String);
	VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
	if (rhs.Is<type>()) return os << #type;

	UNVALUED_OUTPUT(Class);
	UNVALUED_OUTPUT(Return);
	UNVALUED_OUTPUT(If);
	UNVALUED_OUTPUT(Else);
	UNVALUED_OUTPUT(Def);
	UNVALUED_OUTPUT(Newline);
	UNVALUED_OUTPUT(Print);
	UNVALUED_OUTPUT(Indent);
	UNVALUED_OUTPUT(Dedent);
	UNVALUED_OUTPUT(And);
	UNVALUED_OUTPUT(Or);
	UNVALUED_OUTPUT(Not);
	UNVALUED_OUTPUT(Eq);
	UNVALUED_OUTPUT(NotEq);
	UNVALUED_OUTPUT(LessOrEq);
	UNVALUED_OUTPUT(GreaterOrEq);
	UNVALUED_OUTPUT(None);
	UNVALUED_OUTPUT(True);
	UNVALUED_OUTPUT(False);
	UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

	return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_(input) {
	MakeTokens();
}

const Token& Lexer::CurrentToken() const {
	if (!lexemes_.empty()) {
		return lexemes_[current_token_id_];
	} else {
		throw std::logic_error("Not implemented"s);
	}
}

Token Lexer::NextToken() {
	if (current_token_id_ < lexemes_.size() - 1) {
		++current_token_id_;
	}
	return CurrentToken();
}

inline bool Lexer::СharIsLettersOrNumbers(const char current_char) {
	return  (96 < current_char && current_char < 123) || (64 < current_char && current_char < 91) ||  (47 < current_char && current_char < 58);
}

void Lexer::DefineCommandOrID(char& current_char) {
	std::string result_str;
	result_str += current_char;
	while (!input_.eof() && (input_.peek() == '_' || СharIsLettersOrNumbers(input_.peek()))){
		result_str += input_.get();
	}
	if (!CheckKeyWords(result_str)) {
		token_type::Id id;
		id.value = std::move(result_str);
		lexemes_.push_back(std::move(id));
	}
}

bool Lexer::CheckKeyWords(std::string_view def_str) {
	using namespace std::literals;
	if (def_str.size() > 6) {
		return false;
	}
	if (def_str == "class"sv) {
		lexemes_.push_back(token_type::Class());
	} else if (def_str == "return"sv) {
		lexemes_.push_back(token_type::Return());
	} else if (def_str == "if"sv) {
		lexemes_.push_back(token_type::If());
	} else if (def_str == "else"sv) {
		lexemes_.push_back(token_type::Else());
	} else if (def_str == "def"sv) {
		lexemes_.push_back(token_type::Def());
	} else if (def_str == "print"sv) {
		lexemes_.push_back(token_type::Print());
	} else if (def_str == "or"sv) {
		lexemes_.push_back(token_type::Or());
	} else if (def_str == "None"sv) {
		lexemes_.push_back(token_type::None());
	} else if (def_str == "and"sv) {
		lexemes_.push_back(token_type::And());
	} else if (def_str == "not"sv) {
		lexemes_.push_back(token_type::Not());
	} else if (def_str == "True"sv) {
		lexemes_.push_back(token_type::True());
	} else if (def_str == "False"sv) {
		lexemes_.push_back(token_type::False());
	} else {
		return false;
	}
	return true;
}

template <typename T>
inline void RepeatIndetation(int count, T type);

void Lexer::DefineIndetations(char& current_char) {
	size_t counter = 0;
	while (!input_.eof() && current_char == ' ') {
		current_char = input_.get();
		++counter;
	}
	counter /= 2;
	if (!(current_char == '\n' || current_char == '\r') || input_.eof()) {
		if (counter > indetation_counter_) {
			RepeatIndetation(counter - indetation_counter_, token_type::Indent());
			indetation_counter_ = counter;
		} else if (counter < indetation_counter_) {
			RepeatIndetation(indetation_counter_ - counter, token_type::Dedent());
			indetation_counter_ = counter;
		}
	}
}

void Lexer::DefineCharacter(char& current_char) {
	if (current_char == '>' && input_.peek() == '=') {
		current_char = input_.get();
		lexemes_.push_back(token_type::GreaterOrEq());
	} else if (current_char == '<' && input_.peek() == '=') {
		current_char = input_.get();
		lexemes_.push_back(token_type::LessOrEq());
	} else if (current_char == '=' && input_.peek() == '=') {
		current_char = input_.get();
		lexemes_.push_back(token_type::Eq());
	} else if (current_char == '!' && input_.peek() == '=') {
		current_char = input_.get();
		lexemes_.push_back(token_type::NotEq());
	} else if (current_char == '\n' && current_char != '\r' ) {
		if (!lexemes_.empty() && !lexemes_.back().Is<token_type::Newline>() && !lexemes_.back().Is<token_type::Dedent>()) {
			lexemes_.push_back(token_type::Newline());
		}
	} else if (current_char != ' ') {
		token_type::Char char_token;
		char_token.value = current_char;
		lexemes_.push_back(char_token);
	}
}

void Lexer::IgnoreComment() {
	std::string comment_txt;
	std::getline(input_, comment_txt);
	if (!lexemes_.empty() && !(lexemes_.back().Is<token_type::Newline>() || lexemes_.back().Is<token_type::Dedent>())) {
		lexemes_.push_back(token_type::Newline());
	}
}

inline void Lexer::DefineScreenedCharacter(char& current_char) { // определить экранированный символ
	char next_char = input_.peek();
	if (next_char == '\'') {
		current_char = input_.get();
	} else if (next_char == '\"') {
		current_char = input_.get();
	} else if (next_char == 'n') {
		input_.get();
		current_char = '\n';
	}  else if (next_char == 't') {
		input_.get();
		current_char = '\t';
	}
}

void Lexer::MakeTokens() {
	while (!input_.eof()) {
		char defining_char(input_.get());
		if ((!lexemes_.empty() && lexemes_.back().Is<token_type::Newline>()) ) { // Отступы
			DefineIndetations(defining_char);
		}
		if (defining_char == '_' || (96 < defining_char && defining_char < 123) || (64 < defining_char && defining_char < 91)) { // Идентификаторы и ключевые слова
			DefineCommandOrID(defining_char);
			continue;
		}
		if (47 < defining_char && defining_char < 58) { // Цифры
			input_.putback(defining_char);
			token_type::Number number_token;
			input_ >> number_token.value;
			lexemes_.push_back(number_token);
			continue;
		}
		if (defining_char == '\'' || defining_char == '\"' ) { // Строки
			token_type::String string_token;
			char quotation_marks_type(defining_char);
			defining_char = input_.get();
			while ( defining_char != quotation_marks_type) {
				if (!input_.eof() && defining_char == '\\') {
					DefineScreenedCharacter(defining_char);
				}
				string_token.value += defining_char;
				defining_char = input_.get();
			}
			lexemes_.push_back(std::move(string_token));
			continue;
		}
		if (defining_char == '#') { // Комментарии
			IgnoreComment();
			continue;
		}
		if (!input_.eof()) {
			DefineCharacter(defining_char); // Отдельные знаки
		}
	}
	if (!lexemes_.empty() && !(lexemes_.back().Is<token_type::Newline>() || lexemes_.back().Is<token_type::Dedent>())) {
		lexemes_.push_back(token_type::Newline());
	}
	lexemes_.push_back(token_type::Eof());
}

}  // namespace parse
