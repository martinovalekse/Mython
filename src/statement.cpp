#include "statement.h"

#include <iostream>
#include <sstream>
#include <ostream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;

}  // namespace

inline runtime::ClassInstance* ConvertNumsAdressToPointer(std::stringstream& stream) {
	long long unsigned int adress;
	stream >> std::hex >> adress;
	int * raw_ptr=reinterpret_cast<int *>(adress);
	return reinterpret_cast<runtime::ClassInstance*>(raw_ptr);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_(var), rv_(std::move(rv)) { }

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
	return closure[var_] = rv_->Execute(closure, context);
}

VariableValue::VariableValue(const std::string& var_name) : ids_chain_({var_name}) { }
VariableValue::VariableValue(std::vector<std::string> dotted_ids) : ids_chain_(std::move(dotted_ids)) { }

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
	Closure* current_closure = &closure;
	for (size_t i = 0; i+1 < ids_chain_.size(); ++i) {
		std::string& id = ids_chain_[i];
		auto iter_object = current_closure->find(id);
		if (iter_object == current_closure->end()) {
			throw std::runtime_error("Name not found in the scope");
		}
		auto* ptr = iter_object->second.TryAs<runtime::ClassInstance>();
		if (ptr) {
			current_closure = &ptr->Fields();
		}
		else {
			throw std::runtime_error("Cant access fields");
		}
	}
	auto it = current_closure->find(ids_chain_.back());
	if (it != current_closure->end()) {
		return it->second;
	}
	throw std::runtime_error("VariableValue::Execute didn't work!"s);
}

Print::Print(unique_ptr<Statement> argument) {
	args_.push_back(std::move(argument));
}

Print::Print(std::vector<std::unique_ptr<Statement>> args)  {
	args_.reserve(args.size());
	std::for_each(args.begin(), args.end(), [this](std::unique_ptr<Statement>& arg){ args_.push_back(std::move(arg)); });
}

unique_ptr<Print> Print::Variable(const std::string& name)  {
	return std::make_unique<Print>(Print(std::move(make_unique<VariableValue>(name))));
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
	using namespace std::literals;
	for (auto& arg : args_) {
		ObjectHolder object = arg.get()->Execute(closure, context);
		if (object) {
			object->Print(context.GetOutputStream(), context);
		}  else {
			context.GetOutputStream() << "None"sv;
		}
		if (arg != args_.back()) {
			context.GetOutputStream()  << " "sv;
		}
	}
	context.GetOutputStream() << "\n"sv	;
	std::ostringstream stream;
	stream << context.GetOutputStream().rdbuf();
	return ObjectHolder::Own(runtime::String(stream.str()));
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
					   std::vector<std::unique_ptr<Statement>> args) : object_(std::move(object)), method_(std::move(method)){
	args_.reserve(args.size());
	std::for_each(args.begin(), args.end(), [this](std::unique_ptr<Statement>& arg){ args_.push_back(std::move(arg)); });
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
	ObjectHolder obj = object_->Execute(closure, context);
	std::vector<ObjectHolder> actual_args;
	for (auto& arg : args_) {
		actual_args.push_back(arg->Execute(closure, context));
	}
	return obj.TryAs<runtime::ClassInstance>()->Call(method_, std::move(actual_args), context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
	auto object = argument_->Execute(closure, context);
	std::ostringstream stream;
	if (object) {
		if (object.TryAs<runtime::Class>()) {
			auto class_instance_ptr = reinterpret_cast<NewInstance*>(argument_.get());
			runtime::ObjectHolder class_obj = class_instance_ptr->Execute(closure, context);
			class_obj.Get()->Print(stream, context);
		} else {
			object.Get()->Print(stream, context);
		}
	} else {
		stream << "None";
	}
	runtime::String str(stream.str());
	return ObjectHolder::Own(std::move(str));
}

int GetNumberFromObjectHolder(ObjectHolder& object, Context& context) {
	std::ostringstream stream;
	object.Get()->Print(stream, context);
	int num = std::stoi(stream.str());
	return  num;
}

// ------------ arithmetic binary operations
ObjectHolder Add::Execute(Closure& closure, Context& context) {
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
	std::ostringstream stream;
	if (lhs_obj.TryAs<runtime::Number>() && rhs_obj.TryAs<runtime::Number>()) {
		lhs_obj.Get()->Print(stream, context);
		int lhs_num = std::stoi(stream.str());
		stream.str("");
		stream.clear();
		rhs_obj.Get()->Print(stream, context);
		int rhs_num = std::stoi(stream.str());
		runtime::Number result(lhs_num + rhs_num);
		return  ObjectHolder::Own(std::move(result));
	} else if (lhs_obj.TryAs<runtime::String>() && rhs_obj.TryAs<runtime::String>()) {
		lhs_obj.Get()->Print(stream, context);
		rhs_obj.Get()->Print(stream, context);
		runtime::String result(stream.str());
		return  ObjectHolder::Own(std::move(result));
	} else if (lhs_obj.TryAs<runtime::ClassInstance>()) {
		auto class_ptr = reinterpret_cast<runtime::ClassInstance*>(lhs_obj.Get());
		return class_ptr->Call("__add__", {rhs_obj}, context);
	}
	throw std::runtime_error("Addition is not possible");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
	//std::cerr << "Sub - Execute" << std::endl;
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
	if (!lhs_obj.TryAs<runtime::Number>() || !rhs_obj.TryAs<runtime::Number>() ) {
		throw std::runtime_error("One or both objects are not numbers");
	}
	runtime::Number result(GetNumberFromObjectHolder(lhs_obj, context) - GetNumberFromObjectHolder(rhs_obj, context));
	return ObjectHolder::Own(std::move(result));
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
	if (!lhs_obj.TryAs<runtime::Number>() || !rhs_obj.TryAs<runtime::Number>() ) {
		throw std::runtime_error("One or both objects are not numbers");
	}
	runtime::Number result(GetNumberFromObjectHolder(lhs_obj, context) * GetNumberFromObjectHolder(rhs_obj, context));
	return ObjectHolder::Own(std::move(result));
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
	if (!lhs_obj.TryAs<runtime::Number>() || !rhs_obj.TryAs<runtime::Number>() ) {
		throw std::runtime_error("One or both objects are not numbers");
	}
	int rhs_num(GetNumberFromObjectHolder(rhs_obj, context));
	if (rhs_num == 0) {
		throw std::runtime_error("Division by zero");
	}
	runtime::Number result(GetNumberFromObjectHolder(lhs_obj, context) / rhs_num);
	return ObjectHolder::Own(std::move(result));
}

// ------------ End of operations list

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
	for (auto& item : compounds_) {
		item->Execute(closure, context);
	}
	return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& /*closure*/, Context& /*context*/) {
	throw statement_.get();
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(cls) {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
	auto class_ptr = reinterpret_cast<runtime::Class*>(cls_.Get());
	auto instance_ptr = reinterpret_cast<runtime::ClassInstance*>(class_ptr);
	return closure[class_ptr->GetName()] = ObjectHolder::Share(*instance_ptr);
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv)
								: object_(object), field_name_(field_name), rv_(std::move(rv)) { }

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
	std::stringstream stream;
	auto object = object_.Execute(closure, context);
	auto* ptr = object.TryAs<runtime::ClassInstance>();
	if (ptr) {
		return ptr->Fields()[field_name_] = rv_->Execute(closure, context);
	}
	throw std::runtime_error("There is no such field"s);
}

// ------------ logical binary operations

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
			   std::unique_ptr<Statement> else_body) : condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_(std::move(else_body)){
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
	ObjectHolder condition_result = condition_.get()->Execute(closure, context);
	std::stringstream stream;
	condition_result.Get()->Print(stream, context);
	if (stream.str() == "True") {
		return if_body_.get()->Execute(closure, context);
	} else {
		return else_body_.get() != nullptr ? else_body_.get()->Execute(closure, context) : ObjectHolder::None();
	}
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	std::ostringstream stream;
	lhs_obj.Get()->Print(stream, context);
	if (lhs_obj.TryAs<runtime::Bool>()) {
		if (stream.str() == "True") {
			return ObjectHolder::Own(runtime::Bool(true));
		} else {
			ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
			stream.str("");
			stream.clear();
			rhs_obj.Get()->Print(stream, context);
			if (stream.str() == "True") {
				return ObjectHolder::Own(runtime::Bool(true));
			}
		}
	}
	return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
	ObjectHolder lhs_obj = lhs_.get()->Execute(closure, context);
	std::ostringstream stream;
	lhs_obj.Get()->Print(stream, context);
	if (lhs_obj.TryAs<runtime::Bool>()) {
		if (stream.str() == "True") {
			ObjectHolder rhs_obj = rhs_.get()->Execute(closure, context);
			stream.str("");
			stream.clear();
			rhs_obj.Get()->Print(stream, context);
			if (stream.str() == "True") {
				return ObjectHolder::Own(runtime::Bool(true));
			}
		}
	}
	return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
	ObjectHolder obj = argument_.get()->Execute(closure, context);
	std::ostringstream stream;
	obj.Get()->Print(stream, context);
	return stream.str() == "True" ?  ObjectHolder::Own(runtime::Bool(false)) : ObjectHolder::Own(runtime::Bool(true));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
	: BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
	auto result = cmp_(lhs_.get()->Execute(closure, context), rhs_.get()->Execute(closure, context), context);
	return ObjectHolder::Own(runtime::Bool(result));
}

// ------------ End of logical operations list

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) : class__(class_) {
	args_.reserve(args.size());
	std::for_each(args.begin(), args.end(), [this](std::unique_ptr<Statement>& arg){ args_.push_back(std::move(arg)); });
}

NewInstance::NewInstance(const runtime::Class& class_) : class__(class_) { }

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
	auto instance = ObjectHolder::Own(runtime::ClassInstance(class__));
	const auto* m = class__.GetMethod(INIT_METHOD);
	if (m != nullptr) {
		vector<ObjectHolder> actual_args;
		for (auto& st : args_) {
			actual_args.push_back(st->Execute(closure, context));
		}
		instance.TryAs<runtime::ClassInstance>()->Call(INIT_METHOD, actual_args, context);
	}
	return instance;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(std::move(body)) { }

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
	try {
		body_.get()->Execute(closure, context);
	} catch (Statement* item) {
		return item->Execute(closure, context);
	}
	return ObjectHolder::None();
}

}  // namespace ast
