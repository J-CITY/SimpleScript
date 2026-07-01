#pragma once

#include "modules.hpp"

#include <iostream>

//#include "ikigaiScript.h"

using namespace IkigaiScript;
    ScopePtr IkigaiScriptInterpreter::newModule(const std::string& name, ModulePrivilegeFlags flags, const std::unordered_map<std::string, Lambda>& functions) {
        auto& modSource = flags ? optionalModules : modules;
        modSource.emplace_back(flags, std::make_shared<Scope>(name, this));
        auto scope = modSource.back().scope;

        for (auto& funcPair : functions) {
            newFunction(funcPair.first, scope, funcPair.second);
        }

        return modSource.back().scope;
    }

    Module* IkigaiScriptInterpreter::getOptionalModule(const std::string& name) {
        auto iter = std::find_if(optionalModules.begin(), optionalModules.end(), [&name](const auto& mod) {return mod.scope->name == name; });
        if (iter != optionalModules.end()) {
            return &*iter;
        }
        return nullptr;
    }

    void IkigaiScriptInterpreter::createStandardLibrary() {
        newModule("StandardLib"s, 0, {
            {"=", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("=");
                }
                if (args.size() == 1) {
                    return args[0];
                }
                *args[0] = *args[1];
                return args[0];
            }},

            {"+", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("+");
                }
                if (args.size() == 1) {
                    return args[0];
                }
                return std::make_shared<Value>(*args[0] + *args[1]);
                }},

            {"-", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("-");
                }
                if (args.size() == 1) {
                    auto zero = Value(Int(0));
                    upconvert(*args[0], zero);
                    return std::make_shared<Value>(zero - *args[0]);
                }
                return std::make_shared<Value>(*args[0] - *args[1]);
            }},

            {"*", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("*");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(*args[0] * *args[1]);
                }},

            {"/", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("/");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(*args[0] / *args[1]);
                }},

            {"%", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("%");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(*args[0] % *args[1]);
                }},

            {"==", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("==");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] == *args[1]));
                }},

            {"!=", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("!=");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] != *args[1]));
                }},

            {"||", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("||");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(1));
                }
                return std::make_shared<Value>((Int)(*args[0] || *args[1]));
                }},

            {"&&", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("&&");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] && *args[1]));
                }},

            {"++", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto i = args.size() - 1;
                if (i) {
                    // prefix
                    *args[i] += Value(Int(1));
                    return args[i];
                } else {
                    // postfix
                    auto val = std::make_shared<Value>(*args[i]);
                    *args[i] += Value(Int(1));
                    return val;
                }
                }},

            {"--", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto i = args.size() - 1;
                if (i) {
                    // prefix
                    *args[i] -= Value(Int(1));
                    return args[i];
                } else {
                    // postfix
                    auto val = std::make_shared<Value>(*args[i]);
                    *args[i] -= Value(Int(1));
                    return val;
                }
                }},

            {"+=", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                if (args.size() == 1) {
                    return args[0];
                }
                *args[0] += *args[1];
                return args[0];
                }},

            {"-=", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                if (args.size() == 1) {
                    return args[0];
                }
                *args[0] -= *args[1];
                return args[0];
                }},

            {"*=", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                if (args.size() == 1) {
                    return args[0];
                }
                *args[0] *= *args[1];
                return args[0];
                }},

            {"/=", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                if (args.size() == 1) {
                    return args[0];
                }
                *args[0] /= *args[1];
                return args[0];
                }},

            {">", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable(">");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] > *args[1]));
                }},

            {"<", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("<");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] < *args[1]));
                }},

            {">=", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable(">=");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] >= *args[1]));
                }},

            {"<=", [this](const List& args) {
                if (args.size() == 0) {
                    return resolveVariable("<=");
                }
                if (args.size() < 2) {
                    return std::make_shared<Value>(Int(0));
                }
                return std::make_shared<Value>((Int)(*args[0] <= *args[1]));
                }},

            {"!", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Int(0));
                }
                if (args.size() == 1) {
                    if (args[0]->getType() != Type::Int) return std::make_shared<Value>();
                    auto val = Int(1);
                    for (auto i = Int(1); i <= args[0]->getInt(); ++i) {
                        val *= i;
                    }
                    return std::make_shared<Value>(val);
                }
                if (args.size() == 2) {
                    return std::make_shared<Value>((Int)(!args[1]->getBool()));
                }
                return std::make_shared<Value>();
                }},

        // aliases
            {"identity", [](List args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                return args[0];
                }},

            {"copy", [](List args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Class) {
                    return std::make_shared<Value>(std::make_shared<Class>(*args[0]->getClass()));
                }
                return std::make_shared<Value>(*args[0]);
                }},

            {"listindex", [](List args) {
                if (args.size() > 0) {
                    if (args.size() == 1) {
                        return args[0];
                    }

                    auto var = args[0];
                    if (args[1]->getType() != Type::Int) {
                        var->upconvert(Type::Map);
                    }

                    switch (var->getType()) {
                    case Type::Array:
                    {
                        auto ival = args[1]->getInt();
                        auto& arr = var->getArray();
                        if (ival < 0 || ival >= (Int)arr.size()) {
                            throw Exception("Out of bounds array access index "s + std::to_string(ival) + ", array length " + std::to_string(arr.size()));
                        } else {
                            switch (arr.getType()) {
                            case Type::Int:
                                return std::make_shared<Value>(arr.value.asInt[ival]);
                                break;
                            case Type::Float:
                                return std::make_shared<Value>(arr.value.asFloat[ival]);
                                break;
                            //case Type::Vec3:
                            //    return std::make_shared<Value>(get<vector<vec3>>(arr.value)[ival]);
                            //    break;
                            case Type::String:
                                return std::make_shared<Value>(arr.value.asString[ival]);
                                break;
                            default:
                                throw Exception("Attempting to access array of illegal type");
                                break;
                            }
                        }
                    }
                    break;
                    default:
                        var = std::make_shared<Value>(*var);
                        var->upconvert(Type::List);
                        [[fallthrough]];
                    case Type::List:
                    {
                        auto ival = args[1]->getInt();

                        auto& list = var->getList();
                        if (ival < 0 || ival >= (Int)list.size()) {
                            throw Exception("Out of bounds list access index "s + std::to_string(ival) + ", list length " + std::to_string(list.size()));
                        } else {
                            return list[ival];
                        }
                    }
                    break;
                    case Type::Class:
                    {
                        auto strval = args[1]->getString();
                        auto& struc = var->getClass();
                        auto iter = struc->variables.find(strval);
                        if (iter == struc->variables.end()) {
                            throw Exception("Class `"s + struc->name + "` does not contain member `" + strval + "`");
                        } else {
                            return iter->second;
                        }
                    }
                    break;
                    case Type::Map:
                    {
                        auto& dict = var->getDictionary();
                        auto& ref = (*dict)[args[1]->getHash()];
                        if (ref == nullptr) {
                            ref = std::make_shared<Value>();
                        }
                        return ref;
                    }
                    break;
                    }
                }
                return std::make_shared<Value>();
                }},
        // casting
            {"bool", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Int(0));
                }
                auto val = *args[0];
                val.hardconvert(Type::Int);
                val.value.asInt = (Int)args[0]->getBool();
                return std::make_shared<Value>(val);
                }},

            {"int", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Int(0));
                }
                auto val = *args[0];
                val.hardconvert(Type::Int);
                return std::make_shared<Value>(val);
                }},

            {"float", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Float(0.0));
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                return std::make_shared<Value>(val);
                }},

            //{"vec3", [](const List& args) {
            //    if (args.size() == 0) {
            //        return std::make_shared<Value>(vec3());
            //    }
            //    if (args.size() < 3) {
            //        auto val = *args[0];
            //        val.hardconvert(Type::Float);
            //        return std::make_shared<Value>(vec3((float)val.getFloat()));
            //    }
            //    auto x = *args[0];
            //    x.hardconvert(Type::Float);
            //    auto y = *args[1];
            //    y.hardconvert(Type::Float);
            //    auto z = *args[2];
            //    z.hardconvert(Type::Float);
            //    return std::make_shared<Value>(vec3((float)x.getFloat(), (float)y.getFloat(), (float)z.getFloat()));
            //     }},

            {"string", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(""s);
                }
                auto val = *args[0];
                val.hardconvert(Type::String);
                return std::make_shared<Value>(val);
                }},

            {"array", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Array());
                }
                auto list = std::make_shared<Value>(args);
                list->hardconvert(Type::Array);
                return list;
                }},

            {"list", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(List());
                }
                return std::make_shared<Value>(args);
                }},

            {"dictionary", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(std::make_shared<Dictionary>());
                }
                if (args.size() == 1) {
                    auto val = *args[0];
                    val.hardconvert(Type::Map);
                    return std::make_shared<Value>(val);
                }
                auto dict = std::make_shared<Value>(std::make_shared<Dictionary>());
                for (auto&& arg : args) {
                    auto val = *arg;
                    val.hardconvert(Type::Map);
                    dict->getDictionary()->merge(*val.getDictionary());
                }
                return dict;
                }},

            {"toarray", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(Array());
                }
                auto val = *args[0];
                val.hardconvert(Type::Array);
                return std::make_shared<Value>(val);  
                }},

            {"tolist", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>(List());
                }
                auto val = *args[0];
                val.hardconvert(Type::List);
                return std::make_shared<Value>(val);
                }},

        // overal stdlib
            {"typeof", [](List args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(getTypeName(args[0]->getType()));
                }},

            {"sqrt", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                return std::make_shared<Value>(sqrt(val.getFloat()));
                }},

            {"sin", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                return std::make_shared<Value>(sin(val.getFloat()));
                }},

            {"cos", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                return std::make_shared<Value>(cos(val.getFloat()));
                }},

            {"tan", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                return std::make_shared<Value>(tan(val.getFloat()));
                }},

            {"pow", [](const List& args) {
                if (args.size() < 2) {
                    return std::make_shared<Value>(Float(0));
                }
                auto val = *args[0];
                val.hardconvert(Type::Float);
                auto val2 = *args[1];
                val2.hardconvert(Type::Float);
                return std::make_shared<Value>(pow(val.getFloat(), val2.getFloat()));
                }},

            {"abs", [](const List& args) {
                if (args.size() == 0) {
                    return std::make_shared<Value>();
                }
                switch (args[0]->getType()) {
                case Type::Int:
                    return std::make_shared<Value>(Int(abs(args[0]->getInt())));
                    break;
                case Type::Float:
                    return std::make_shared<Value>(Float(fabs(args[0]->getFloat())));
                    break;
                default:
                    return std::make_shared<Value>();
                    break;
                }                
                }},

            {"min", [](const List& args) {
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                auto val2 = *args[1];
                upconvertThrowOnNonNumberToNumberCompare(val, val2);
                if (val > val2) {
                    return std::make_shared<Value>(val2);
                }
                return std::make_shared<Value>(val);
                }},

            {"max", [](const List& args) {
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                auto val = *args[0];
                auto val2 = *args[1];
                upconvertThrowOnNonNumberToNumberCompare(val, val2);
                if (val < val2) {
                    return std::make_shared<Value>(val2);
                }
                return std::make_shared<Value>(val);
                }},

            {"swap", [](const List& args) {
                if (args.size() < 2) {
                    return std::make_shared<Value>();
                }
                auto v = *args[0];
                *args[0] = *args[1];
                *args[1] = v;

                return std::make_shared<Value>();
                }},

            {"print", [](const List& args) {
                for (auto&& arg : args) {
                    printf("%s", arg->getPrintString().c_str());
                }
                printf("\n");
                return std::make_shared<Value>();
                }},

            {"getline", [](const List& args) {
                std::string s;
                // blocking calls are fine
                getline(std::cin, s);
                if (args.size() > 0) {
                    args[0]->value = s;
                }
                return std::make_shared<Value>(s);
                }},

            {"map", [this](const List& args) {
                if (args.size() < 2 || args[1]->getType() != Type::Function) {
                    return std::make_shared<Value>();
                }
                auto ret = std::make_shared<Value>(List());
                auto& retList = ret->getList();
                auto func = args[1]->getFunction();

                if (args[0]->getType() == Type::Array) {
                    auto val = *args[0];
                    val.upconvert(Type::List);
                    for (auto&& v : val.getList()) {
                        retList.push_back(callFunction(func, { v }));
                    }
                    return ret;
                }

                for (auto&& v : args[0]->getList()) {
                    retList.push_back(callFunction(func, { v }));
                }
                return ret;

                }},

            {"fold", [this](const List& args) {
                if (args.size() < 3 || args[1]->getType() != Type::Function) {
                    return std::make_shared<Value>();
                }

                auto func = args[1]->getFunction();
                auto iter = args[2];

                if (args[0]->getType() == Type::Array) {
                    auto val = *args[0];
                    val.upconvert(Type::List);
                    for (auto&& v : val.getList()) {
                        iter = callFunction(func, { iter, v });
                    }
                    return iter;
                }

                for (auto&& v : args[0]->getList()) {
                    iter = callFunction(func, { iter, v });
                }
                return iter;
                }},

            {"clock", [](const List&) {
                return std::make_shared<Value>(Int(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                }},

            {"getduration", [](const List& args) {
                if (args.size() == 2 && args[0]->getType() == Type::Int && args[1]->getType() == Type::Int) {
                    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[1]->getInt())) -
                        std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
                    return std::make_shared<Value>(Float(duration.count()));
                }
                return std::make_shared<Value>();
                }},

            {"timesince", [](const List& args) {
                if (args.size() == 1 && args[0]->getType() == Type::Int) {
                    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - 
                        std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
                    return std::make_shared<Value>(Float(duration.count()));
                }
                return std::make_shared<Value>();
                }},

        // collection functions
            {"length", [](const List& args) {
                if (args.size() == 0 || (int)args[0]->getType() < (int)Type::String) {
                    return std::make_shared<Value>(Int(0));
                }
                if (args[0]->getType() == Type::String) {
                    return std::make_shared<Value>((Int)args[0]->getString().size());
                }
                if (args[0]->getType() == Type::Array) {
                    return std::make_shared<Value>((Int)args[0]->getArray().size());
                }
                return std::make_shared<Value>((Int)args[0]->getList().size());
                }},

            {"find", [](const List& args) {
                if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    if (args[1]->getType() == args[0]->getArray().getType()) {
                        switch (args[0]->getArray().getType()) {
                        case Type::Int:
                        {
                            auto& arry = args[0]->getStdVector<Int>();
                            auto iter = find(arry.begin(), arry.end(), args[1]->getInt());
                            if (iter == arry.end()) {
                                return std::make_shared<Value>();
                            }
                            return std::make_shared<Value>((Int)(iter - arry.begin()));
                        }
                        break;
                        case Type::Float:
                        {
                            auto& arry = args[0]->getStdVector<Float>();
                            auto iter = find(arry.begin(), arry.end(), args[1]->getFloat());
                            if (iter == arry.end()) {
                                return std::make_shared<Value>();
                            }
                            return std::make_shared<Value>((Int)(iter - arry.begin()));
                        }
                        break;
                        //case Type::Vec3:
                        //{
                        //    auto& arry = args[0]->getStdVector<vec3>();
                        //    auto iter = find(arry.begin(), arry.end(), args[1]->getVec3());
                        //    if (iter == arry.end()) {
                        //        return std::make_shared<Value>();
                        //    }
                        //    return std::make_shared<Value>((Int)(iter - arry.begin()));
                        //}
                        break;
                        case Type::String:
                        {
                            auto& arry = args[0]->getStdVector<std::string>();
                            auto iter = find(arry.begin(), arry.end(), args[1]->getString());
                            if (iter == arry.end()) {
                                return std::make_shared<Value>();
                            }
                            return std::make_shared<Value>((Int)(iter - arry.begin()));
                        }
                        break;
                        case Type::Function:
                        {
                            auto& arry = args[0]->getStdVector<FunctionRef>();
                            auto iter = find(arry.begin(), arry.end(), args[1]->getFunction());
                            if (iter == arry.end()) {
                                return std::make_shared<Value>();
                            }
                            return std::make_shared<Value>((Int)(iter - arry.begin()));
                        }
                        break;
                        default:
                            break;
                        }
                    }
                    return std::make_shared<Value>();
                }
                auto& list = args[0]->getList();
                for (size_t i = 0; i < list.size(); ++i) {
                    if (*list[i] == *args[1]) {
                        return std::make_shared<Value>((Int)i);
                    }
                }
                return std::make_shared<Value>();
                }},

            {"erase", [](const List& args) {
                if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array || args[1]->getType() != Type::Int) {
                    return std::make_shared<Value>();
                }
                
                if (args[0]->getType() == Type::Array) {
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                        args[0]->getStdVector<Int>().erase(args[0]->getStdVector<Int>().begin() + args[1]->getInt());
                        break;
                    case Type::Float:
                        args[0]->getStdVector<Float>().erase(args[0]->getStdVector<Float>().begin() + args[1]->getInt());
                        break;
                    //case Type::Vec3:
                    //    args[0]->getStdVector<vec3>().erase(args[0]->getStdVector<vec3>().begin() + args[1]->getInt());
                    //    break;
                    case Type::String:
                        args[0]->getStdVector<std::string>().erase(args[0]->getStdVector<std::string>().begin() + args[1]->getInt());
                        break;
                    case Type::Function:
                        args[0]->getStdVector<FunctionRef>().erase(args[0]->getStdVector<FunctionRef>().begin() + args[1]->getInt());
                        break;
                    default:
                        break;
                    }
                } else {
                    args[0]->getList().erase(args[0]->getList().begin() + args[1]->getInt());
                }
                return std::make_shared<Value>();
                }},

            {"pushback", [](const List& args) {
                if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }

                if (args[0]->getType() == Type::Array) {
                    if (args[0]->getArray().getType() == args[1]->getType()) {
                        switch (args[0]->getArray().getType()) {
                        case Type::Int:
                            args[0]->getStdVector<Int>().push_back(args[1]->getInt());
                            break;
                        case Type::Float:
                            args[0]->getStdVector<Float>().push_back(args[1]->getFloat());
                            break;
                        //case Type::Vec3:
                        //    args[0]->getStdVector<vec3>().push_back(args[1]->getVec3());
                        //    break;
                        case Type::String:
                            args[0]->getStdVector<std::string>().push_back(args[1]->getString());
                            break;
                        case Type::Function:
                            args[0]->getStdVector<FunctionRef>().push_back(args[1]->getFunction());
                            break;
                        default:
                            break;
                        }
                    }
                } else {
                    args[0]->getList().push_back(args[1]);
                }
                return std::make_shared<Value>();
                }},

            {"popback", [](const List& args) {
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    args[0]->getArray().pop_back();
                } else {
                    args[0]->getList().pop_back();
                }
                return std::make_shared<Value>();
                }},

            {"popfront", [](const List& args) {
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                    {
                        auto& arry = args[0]->getStdVector<Int>();
                        arry.erase(arry.begin());
                    }
                    break;
                    case Type::Float:
                    {
                        auto& arry = args[0]->getStdVector<Float>();
                        arry.erase(arry.begin());
                    }
                    break;
                    //case Type::Vec3:
                    //{
                    //    auto& arry = args[0]->getStdVector<vec3>();
                    //    arry.erase(arry.begin());
                    //}
                    break;
                    case Type::Function:
                    {
                        auto& arry = args[0]->getStdVector<FunctionRef>();
                        arry.erase(arry.begin());
                    }
                    break;
                    case Type::String:
                    {
                        auto& arry = args[0]->getStdVector<std::string>();
                        arry.erase(arry.begin());
                    }
                    break;
                    
                    default:
                        break;
                    }
                    return args[0];
                } else {
                    auto& list = args[0]->getList();
                    list.erase(list.begin());
                }
                return std::make_shared<Value>();
                }},

            {"front", [](const List& args) {
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                        return std::make_shared<Value>(args[0]->getStdVector<Int>().front());
                    case Type::Float:
                        return std::make_shared<Value>(args[0]->getStdVector<Float>().front());
                    //case Type::Vec3:
                    //   return std::make_shared<Value>(args[0]->getStdVector<vec3>().front());
                    case Type::Function:
                        return std::make_shared<Value>(args[0]->getStdVector<FunctionRef>().front());
                    case Type::String:
                        return std::make_shared<Value>(args[0]->getStdVector<std::string>().front());
                    default:
                        break;
                    }
                    return std::make_shared<Value>();
                } else {
                    return args[0]->getList().front();
                }
                }},

            {"back", [](const List& args) {
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                        return std::make_shared<Value>(args[0]->getStdVector<Int>().back());
                    case Type::Float:
                        return std::make_shared<Value>(args[0]->getStdVector<Float>().back());
                    //case Type::Vec3:
                    //    return std::make_shared<Value>(args[0]->getStdVector<vec3>().back());
                    case Type::Function:
                        return std::make_shared<Value>(args[0]->getStdVector<FunctionRef>().back());
                    case Type::String:
                        return std::make_shared<Value>(args[0]->getStdVector<std::string>().back());
                    default:
                        break;
                    }
                    return std::make_shared<Value>();
                } else {
                    return args[0]->getList().back();
                }
                }},

            {"reverse", [](const List& args) {                
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::String) {
                    return std::make_shared<Value>();
                }
                auto copy = std::make_shared<Value>(*args[0]);

                if (args[0]->getType() == Type::String) {
                    auto& str = copy->getString();
                    std::reverse(str.begin(), str.end());
                    return copy;
                } else if (args[0]->getType() == Type::Array) { 
                    switch (copy->getArray().getType()) {
                    case Type::Int:
                    {
                        auto& vl = copy->getStdVector<Int>();
                        std::reverse(vl.begin(), vl.end());
                        return copy;
                    }
                        break;
                    case Type::Float:
                    {
                        auto& vl = copy->getStdVector<Float>();
                        std::reverse(vl.begin(), vl.end());
                        return copy;
                    }
                        break;
                    //case Type::Vec3:
                    //{
                    //    auto& vl = copy->getStdVector<vec3>();
                    //    std::reverse(vl.begin(), vl.end());
                    //    return copy;
                    //}
                    //    break;
                    case Type::String:
                    {
                        auto& vl = copy->getStdVector<std::string>();
                        std::reverse(vl.begin(), vl.end());
                        return copy;
                    }
                        break;
                    case Type::Function:
                    {
                        auto& vl = copy->getStdVector<FunctionRef>();
                        std::reverse(vl.begin(), vl.end());
                        return copy;
                    }
                        break;
                    default:
                        break;
                    }
                } else if (args[0]->getType() == Type::List) {
                    auto& vl = copy->getList();
                    std::reverse(vl.begin(), vl.end());
                    return copy;
                }
                return std::make_shared<Value>();
                }},

            {"range", [](const List& args) {
                if (args.size() == 2 && args[0]->getType() == args[1]->getType()) {
                    if (args[0]->getType() == Type::Int) {
                        auto ret = std::make_shared<Value>(Array(std::vector<Int>{}));
                        auto& arry = ret->getStdVector<Int>();
                        auto a = args[0]->getInt();
                        auto b = args[1]->getInt();
                        if (b > a) {
                            arry.reserve(b - a);
                            for (Int i = a; i <= b; i++) {
                                arry.push_back(i);
                            }
                        } else {
                            arry.reserve(a - b);
                            for (Int i = a; i >= b; i--) {
                                arry.push_back(i);
                            }
                        }
                        return ret;
                    } else if (args[0]->getType() == Type::Float) {
                        auto ret = std::make_shared<Value>(Array(std::vector<Float>{}));
                        auto& arry = ret->getStdVector<Float>();
                        Float a = args[0]->getFloat();
                        Float b = args[1]->getFloat();
                        if (b > a) {
                            arry.reserve((Int)(b - a));
                            for (Float i = a; i <= b; i++) {
                                arry.push_back(i);
                            }
                        } else {
                            arry.reserve((Int)(a - b));
                            for (Float i = a; i >= b; i--) {
                                arry.push_back(i);
                            }
                        }
                        return ret;
                    }                    
                }
                if (args.size() < 3 || (int)args[0]->getType() < (int)Type::String) {
                    return std::make_shared<Value>();
                }
                auto indexA = *args[1];
                indexA.hardconvert(Type::Int);
                auto indexB = *args[2];
                indexB.hardconvert(Type::Int);
                auto intdexA = indexA.getInt();
                auto intdexB = indexB.getInt();

                if (args[0]->getType() == Type::String) {
                    return std::make_shared<Value>(args[0]->getString().substr(intdexA, intdexB));
                } else if (args[0]->getType() == Type::Array) {
                    if (args[0]->getArray().getType() == args[1]->getType()) {
                        switch (args[0]->getArray().getType()) {
                        case Type::Int:
                            return std::make_shared<Value>(Array(std::vector<Int>(args[0]->getStdVector<Int>().begin() + intdexA, args[0]->getStdVector<Int>().begin() + intdexB)));
                            break;
                        case Type::Float:
                            return std::make_shared<Value>(Array(std::vector<Float>(args[0]->getStdVector<Float>().begin() + intdexA, args[0]->getStdVector<Float>().begin() + intdexB)));
                            break;
                        //case Type::Vec3:
                        //    return std::make_shared<Value>(Array(vector<vec3>(args[0]->getStdVector<vec3>().begin() + intdexA, args[0]->getStdVector<vec3>().begin() + intdexB)));
                        //    break;
                        case Type::String:
                            return std::make_shared<Value>(Array(std::vector<std::string>(args[0]->getStdVector<std::string>().begin() + intdexA, args[0]->getStdVector<std::string>().begin() + intdexB)));
                            break;
                        case Type::Function:
                            return std::make_shared<Value>(Array(std::vector<FunctionRef>(args[0]->getStdVector<FunctionRef>().begin() + intdexA, args[0]->getStdVector<FunctionRef>().begin() + intdexB)));
                            break;
                        default:
                            break;
                        }
                    }
                } else {
                    return std::make_shared<Value>(List(args[0]->getList().begin() + intdexA, args[0]->getList().begin() + intdexB));
                }
                return std::make_shared<Value>();
                }},

            {"replace", [](const List& args) {
                if (args.size() < 3 || args[0]->getType() != Type::String || args[1]->getType() != Type::String || args[2]->getType() != Type::String) {
                    return std::make_shared<Value>();
                }

                std::string& input = args[0]->getString();
                const std::string& lookfor = args[1]->getString();
                const std::string& replacewith = args[2]->getString();
                size_t pos = 0;
                size_t lpos = 0;
                while ((pos = input.find(lookfor, lpos)) != std::string::npos) {
                    input.replace(pos, lookfor.size(), replacewith);
                    lpos = pos + replacewith.size();
                }
                    
                return std::make_shared<Value>(input);
                }},

            {"startswith", [](const List& args) {
                if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(Int(startswith(args[0]->getString(), args[1]->getString())));
                }},

            {"endswith", [](const List& args) {
                if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
                    return std::make_shared<Value>();
                }
                return std::make_shared<Value>(Int(endswith(args[0]->getString(), args[1]->getString())));
                }},

            {"contains", [](const List& args) {
                if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>(Int(0));
                }
                if (args[0]->getType() == Type::Array) {
                    auto item = *args[1];
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                        item.hardconvert(Type::Int);
                        return std::make_shared<Value>((Int)contains(args[0]->getStdVector<Int>(), item.getInt()));
                    case Type::Float:
                        item.hardconvert(Type::Float);
                        return std::make_shared<Value>((Int)contains(args[0]->getStdVector<Float>(), item.getFloat()));
                    //case Type::Vec3:
                    //    item.hardconvert(Type::Vec3);
                    //    return std::make_shared<Value>((Int)contains(args[0]->getStdVector<vec3>(), item.getVec3()));
                    case Type::String:
                        item.hardconvert(Type::String);
                        return std::make_shared<Value>((Int)contains(args[0]->getStdVector<std::string>(), item.getString()));
                    default:
                        break;
                    }
                    return std::make_shared<Value>(Int(0));
                }
                auto& list = args[0]->getList();
                for (size_t i = 0; i < list.size(); ++i) {
                    if (*list[i] == *args[1]) {
                        return std::make_shared<Value>(Int(1));
                    }
                }
                return std::make_shared<Value>(Int(0));
                }},

            {"split", [](const List& args) {
                if (args.size() > 0 && args[0]->getType() == Type::String) {
                    if (args.size() == 1) {
                        std::vector<std::string> chars;
                        for (auto c : args[0]->getString()) {
                            chars.push_back(""s + c);
                        }
                        return std::make_shared<Value>(Array(chars));
                    }
                    return std::make_shared<Value>(Array(split(args[0]->getString(), args[1]->getPrintString())));
                }
                return std::make_shared<Value>();
                }},

            {"sort", [](const List& args) {
                if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
                    return std::make_shared<Value>();
                }
                if (args[0]->getType() == Type::Array) {
                    switch (args[0]->getArray().getType()) {
                    case Type::Int:
                    {
                        auto& arry = args[0]->getStdVector<Int>();
                        std::sort(arry.begin(), arry.end());
                    }
                    break;
                    case Type::Float:
                    {
                        auto& arry = args[0]->getStdVector<Float>();
                        std::sort(arry.begin(), arry.end());
                    }
                    break;
                    case Type::String:
                    {
                        auto& arry = args[0]->getStdVector<std::string>();
                        std::sort(arry.begin(), arry.end());
                    }
                    break;
                    //case Type::Vec3:
                    //{
                    //    auto& arry = args[0]->getStdVector<vec3>();
                    //    std::sort(arry.begin(), arry.end(), [](const vec3& a, const vec3& b) { return a.x < b.x; });
                    //}
                    //break;
                    default:
                        break;
                    }
                    return args[0];
                }
                auto& list = args[0]->getList();
                std::sort(list.begin(), list.end(), [](const ValuePtr& a, const ValuePtr& b) { return *a < *b; });
                return args[0];
                }},
            { "applyfunction", [this](List args) {
                if (args.size() < 2 || args[1]->getType() != Type::Class) {
                    auto func = args[0]->getType() == Type::Function ? args[0] : args[0]->getType() == Type::String ? resolveVariable(args[0]->getString()) : throw Exception("Cannot call non existant function: null");
                    auto list = List();
                    for (size_t i = 1; i < args.size(); ++i) {
                        list.push_back(args[i]);
                    }
                    return callFunction(func->getFunction(), list);
                }
                return std::make_shared<Value>();
                }},
        });

        listIndexFunctionVarLocation = resolveVariable("listindex", modules.back().scope);
        identityFunctionVarLocation = resolveVariable("identity", modules.back().scope);
	}
