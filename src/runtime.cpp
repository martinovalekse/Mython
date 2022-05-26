#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

#include <iostream>
#include <algorithm>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
	: data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
	assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
	return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
	return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
	AssertIsValid();
	return *Get();
}

Object* ObjectHolder::operator->() const {
	AssertIsValid();
	return Get();
}

Object* ObjectHolder::Get() const {
	return data_.get();
}

ObjectHolder::operator bool() const {
	return Get() != nullptr;
}

inline bool StringToBool(std::string_view str_v) {
	using namespace std::literals;
	return str_v == "True"sv ? true : false;
}

inline ClassInstance* ConvertNumsAdressToPointer(std::stringstream& stream) {
	long long unsigned int adress;
	stream >> std::hex >> adress;
	int * raw_ptr=reinterpret_cast<int *>(adress);
	return reinterpret_cast<ClassInstance*>(raw_ptr);
}

bool IsTrue(const ObjectHolder& object) {
	using namespace std::literals;
	std::stringstream string_stream;
	SimpleContext SC(std::cout);
	if (object) {
		(object.Get())->Print(string_stream, SC);
		if (object.TryAs<String>()) {
			return !string_stream.str().empty() ? true : false;
		} else if (object.TryAs<Number>()) {
			return std::stoi(string_stream.str()) != 0 ? true : false;
		} else if (object.TryAs<Bool>()) {
			return string_stream.str() == "True"sv ? true : false;
		}
	}
	return false;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
	if (cls_.GetMethod("__str__") != nullptr) {
		std::ostringstream str_stream;
		ObjectHolder result = Call("__str__", {}, context);
		result.Get()->Print(os, context);
	} else {
		os << this;
	}
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
	auto ptr = cls_.GetMethod(method);
	return (ptr != nullptr && ptr->formal_params.size() == argument_count) ? true : false;
}

Closure& ClassInstance::Fields() { return glosure_; }
const Closure& ClassInstance::Fields() const { return glosure_; }

ClassInstance::ClassInstance(const Class& cls) : cls_(cls) { }

ObjectHolder ClassInstance::Call(const std::string& method,
								 const std::vector<ObjectHolder>& actual_args,
								 Context& context) {
	if (HasMethod(method, actual_args.size())) {
		const Method* method_ptr = cls_.GetMethod(method);
		Closure glosure;
		glosure.insert({"self", ObjectHolder::Share(*this)});
		for (size_t i = 0; i < actual_args.size(); ++i) {
			glosure[method_ptr->formal_params[i]] = actual_args[i];
		}
		return cls_.GetMethod(method)->body->Execute(glosure, context);
	}
	throw std::runtime_error("Not implemented_"s);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) : name_(name), methods_(std::move(methods)), parent_(parent) {
}

const Method* Class::GetMethod(const std::string& name) const {
	auto iter = std::find_if(methods_.begin(), methods_.end(), [name](const Method& method){ return method.name == name ? true : false; });
	if (iter == methods_.end() && parent_ != nullptr) {
		auto parent_find = parent_->GetMethod(name);
		if (parent_find) {
			return parent_find;
		}
	}
	return iter != end(methods_) ? &(*iter) : nullptr;
}

[[nodiscard]] const std::string& Class::GetName() const {
	return name_;
}

void Class::Print(ostream& os, [[maybe_unused]]  Context& context) {
	os << "Class " +  GetName();
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
	os << (GetValue() ? "True"sv : "False"sv);
}

bool ComparisonClassInstance(std::string method, const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	std::stringstream lhs_stream, resulting_stream, context_stream;
	SimpleContext simple_context(context_stream);
	lhs.Get()->Print(lhs_stream, simple_context);
	ClassInstance* left_ptr(ConvertNumsAdressToPointer(lhs_stream));
	if (left_ptr != nullptr && left_ptr->HasMethod(method, 1U)) {
		auto result_holder = left_ptr->Call(method, {rhs}, context);
		if (result_holder.TryAs<Bool>()) {
			(result_holder.Get())->Print(resulting_stream, simple_context);
			return StringToBool(resulting_stream.str());
		}
	}
	throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	if (lhs.TryAs<ClassInstance>()) {
		return ComparisonClassInstance("__eq__", lhs, rhs, context);
	}
	return !lhs && !rhs ? true : (!Less(lhs, rhs, context) && !Less(rhs, lhs, context));
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	std::stringstream lhs_stream, rhs_stream, context_stream;
	if (lhs && rhs) {
		SimpleContext simple_context(context_stream);
		(lhs.Get())->Print(lhs_stream, simple_context);
		(rhs.Get())->Print(rhs_stream, simple_context);
		if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
			return lhs_stream.str() < rhs_stream.str() ? true : false;
		} else if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
			return std::stoi(lhs_stream.str()) < std::stoi(rhs_stream.str()) ? true : false;
		} else if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
			return StringToBool(lhs_stream.str()) < StringToBool(rhs_stream.str()) ? true : false;
		} else if (lhs.TryAs<ClassInstance>() ) {
			return ComparisonClassInstance("__lt__", lhs, rhs, context);
		}
	}
	throw std::runtime_error("Cannot compare objects for equality"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	return GreaterOrEqual(lhs, rhs, context) && NotEqual(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
	return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime
