#pragma once

#include <iosfwd>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace parse {
namespace token_type {

struct Number {
	int value;
};

struct Id {
	std::string value;
};

struct Char {
	char value;
};

struct String {
	std::string value;
};

struct Class {};
struct Return {};
struct If {};
struct Else {};
struct Def {};
struct Newline {};
struct Print {};
struct Indent {};
struct Dedent {};
struct Eof {};
struct And {};
struct Or {};
struct Not {};
struct Eq {};
struct NotEq {};
struct LessOrEq {};
struct GreaterOrEq {};
struct None {};
struct True {};
struct False {};

}  // namespace token_type

using TokenBase
	= std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
				   token_type::Class, token_type::Return, token_type::If, token_type::Else,
				   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
				   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
				   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
				   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
	using TokenBase::TokenBase;

	template <typename T>
	[[nodiscard]] bool Is() const {
		return std::holds_alternative<T>(*this);
	}

	template <typename T>
	[[nodiscard]] const T& As() const {
		return std::get<T>(*this);
	}

	template <typename T>
	[[nodiscard]] const T* TryAs() const {
		return std::get_if<T>(this);
	}
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

class Lexer {
public:
	explicit Lexer(std::istream& input);

	[[nodiscard]] const Token& CurrentToken() const;
	Token NextToken();

	template <typename T>
	const T& Expect() const;

	template <typename T, typename U>
	void Expect(const U& value) const;

	template <typename T>
	const T& ExpectNext();

	template <typename T, typename U>
	void ExpectNext(const U& value);

private:
	std::istream& input_;
	std::vector<Token> lexemes_;
	size_t indetation_counter_ = 0;
	size_t current_token_id_ = 0;

	void MakeTokens();

	template <typename T>
	inline void RepeatIndetation(int count, T type);
	inline bool СharIsLettersOrNumbers(const char current_char); // может все difiner'ами назвать?
	void DefineCommandOrID(char& current_char);
	bool CheckKeyWords(std::string_view def_str);
	void DefineIndetations(char& current_char);
	void DefineCharacter(char& current_char);
	void IgnoreComment();
	inline void DefineScreenedCharacter(char& current_char);
};

template <typename T>
const T& Lexer::Expect() const {
	using namespace std::literals;
	if (CurrentToken().Is<T>()) {
		return CurrentToken().As<T>();
	} else {
		throw LexerError("Not implemented"s);
	}
}

template <typename T, typename U>
void Lexer::Expect(const U& value) const {
	using namespace std::literals;
	if ( !(CurrentToken().Is<T>() && CurrentToken().As<T>().value == value)) {
		throw LexerError("Not implemented"s);
	}
}

template <typename T>
const T& Lexer::ExpectNext() {
	using namespace std::literals;
	this->NextToken();
	return this->Expect<T>();
}

template <typename T, typename U>
void Lexer::ExpectNext(const U& value) {
	using namespace std::literals;
	this->NextToken();
	this->Expect<T>(value);
}

template <typename T>
inline void Lexer::RepeatIndetation(int count, T type) {
	for (; count > 0; --count) {
		lexemes_.push_back(type);
	}
}

}  // namespace parse
