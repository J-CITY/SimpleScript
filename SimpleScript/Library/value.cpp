#include "value.hpp"

#include "exception.h"

namespace IkigaiScript {
	List& Value::getList()
	{
		return value.asList;
	}

    size_t Value::getHash() {
        size_t hash = 0;
        switch (getType()) {
        default: break;
        case Type::Bool:
            hash = (size_t)getBool();
            break;
        case Type::Char:
            hash = std::hash<char32_t>{}(getChar());
            break;
        case Type::Int:
            hash = (size_t)getInt();
            break;
        case Type::Float:
            hash = std::hash<Float>{}(getFloat());
            break;
        case Type::Function:
            hash = std::hash<size_t>{}((size_t)getFunction().get());
            break;
        case Type::String:
            hash = std::hash<std::string>{}(getString());
            break;
        }
        return hash ^ typeHashBits(getType());
    }

    // convert this value up to the newType
    void Value::upconvert(Type newType) {
        if (newType > getType()) {
            switch (newType) {
            default:
                throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                break;
            case Type::Int:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Bool:
                    *this = Value((Int)getBool());
                    break;
                case Type::Char:
                    *this = Value((Int)getChar());
                    break;
                }
                break;
            case Type::Float:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Int:
                    *this = Value((Float)getInt());
                    break;
                }
                break;
            case Type::String:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Bool:
                    *this = Value(getBool() ? "true" : "false");
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Char:
                {
                    std::string s;
                    if (getChar() <= 0x7F) s += (char)getChar();
                    else s += "?";
                    *this = Value(s);
                    break;
                }
                case Type::Int:
                    *this = Value(std::to_string(getInt()));
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Float:
                    *this = Value(std::to_string(getFloat()));
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                //case Type::Vec3:
                //{
                //    auto& vec = getVec3();
                //    value = std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z);
                //    break;
                //}
                case Type::Function:
                    *this = Value(getFunction()->name);
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Coro:
                    *this = Value(getCoro()->func->name);
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                }
                break;
            case Type::Array:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    *this = Value(Array());
                    getArray().push_back(Int(0));
                    break;
                case Type::Int:
                    *this = Value(Array(std::vector<Int>{ getInt() }));
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Float:
                    *this = Value(Array(std::vector<Float>{ getFloat() }));
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                //case Type::Vec3:
                //    value = Array(vector<vec3>{ getVec3() });
                //    break;
                case Type::String:
                {
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    auto str = getString();
                    *this = Value(Array(std::vector<std::string>{ }));
                    //auto& arry = getStdVector<std::string>();
                    //for (auto&& ch : str) {
                    //    arry.push_back(""s + ch);
                    //}
                }
                break;
                }
                break;
            case Type::List:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                case Type::Int:
                case Type::Float:
                //case Type::Vec3:
                    *this = Value(List({ std::make_shared<Value>(*this) }));
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::String:
                {
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    auto str = getString();
                    *this = Value(List());
                    auto& list = getList();
                    for (auto&& ch : str) {
                        list.push_back(make_shared<Value>(""s + ch));
                    }
                }
                break;
                case Type::Array:
                    Array arr = getArray();
                    *this = Value(List());
                    auto& list = getList();
                    switch (arr.getType()) {
                    case Type::Int:
                        for (auto&& item : arr.value.asInt) {
                            list.push_back(std::make_shared<Value>(item));
                        }
                        break;
                    case Type::Float:
                        for (auto&& item : arr.value.asFloat) {
                            list.push_back(std::make_shared<Value>(item));
                        }
                        break;
                    //case Type::Vec3:
                    //    for (auto&& item : get<vector<vec3>>(arr.value)) {
                    //        list.push_back(make_shared<Value>(item));
                    //    }
                    //    break;
                    case Type::String:
                        for (auto&& item : arr.value.asString) {
                            list.push_back(std::make_shared<Value>(item));
                        }
                        break;
                    default:
                        throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                        break;
                    }
                    break;
                }
                break;
            case Type::Set:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                case Type::Int:
                case Type::Float:
                case Type::String:
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    *this = Value(std::make_shared<Dictionary>());
                    break;
                case Type::Array:
                {
                    Array arr = getArray();
                    *this = Value(std::make_shared<Set>());
                    auto hashbits = typeHashBits(Type::Int);
                    auto& dict = getSet();
                    size_t index = 0;
                    switch (arr.getType()) {
                    case Type::Int:
                        for (auto&& item : arr.value.asInt) {
                            (*dict).insert(std::make_shared<Value>(item));
                        }
                        break;
                    case Type::Float:
                        for (auto&& item : arr.value.asFloat) {
                            (*dict).insert(std::make_shared<Value>(item));
                        }
                        break;
                        //case Type::Vec3:
                        //    for (auto&& item : get<vector<vec3>>(arr.value)) {
                        //        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                        //    }
                        //    break;
                    case Type::String:
                        for (auto&& item : arr.value.asString) {
                            (*dict).insert(std::make_shared<Value>(item));
                        }
                        break;
                    default:
                        throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                        break;
                    }
                }
                break;
                case Type::List:
                {
                    List list = getList();
                    *this = Value(std::make_shared<Set>());
                    auto& dict = getSet();
                    for (auto&& item : list) {
                        (*dict).insert(item);
                    }
                }
                break;
                }
                break;
            case Type::Map:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Null:
                case Type::Int:
                case Type::Float:
                case Type::String:
                    //throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    *this = Value(std::make_shared<Dictionary>());
                    break;
                case Type::Array:
                {
                    Array arr = getArray();
                    *this = Value(std::make_shared<Dictionary>());
                    auto hashbits = typeHashBits(Type::Int);
                    auto& dict = getDictionary();
                    size_t index = 0;
                    switch (arr.getType()) {
                    case Type::Int:
                        for (auto&& item : arr.value.asInt) {
                            (*dict)[index++ ^ hashbits] = std::make_shared<Value>(item);
                        }
                        break;
                    case Type::Float:
                        for (auto&& item : arr.value.asFloat) {
                            (*dict)[index++ ^ hashbits] = std::make_shared<Value>(item);
                        }
                        break;
                    //case Type::Vec3:
                    //    for (auto&& item : get<vector<vec3>>(arr.value)) {
                    //        (*dict)[index++ ^ hashbits] = make_shared<Value>(item);
                    //    }
                    //    break;
                    case Type::String:
                        for (auto&& item : arr.value.asString) {
                            (*dict)[index++ ^ hashbits] = std::make_shared<Value>(item);
                        }
                        break;
                    default:
                        throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                        break;
                    }
                }
                break;
                case Type::List:
                {
                    auto hashbits = typeHashBits(Type::Int);
                    List list = getList();
                    *this = Value(std::make_shared<Dictionary>());
                    auto& dict = getDictionary();
                    size_t index = 0;
                    for (auto&& item : list) {
                        (*dict)[index++ ^ hashbits] = item;
                    }
                }
                break;
                case Type::Set:
                {
                    auto hashbits = typeHashBits(Type::Int);
                    auto list = getSet();
                    *this = Value(std::make_shared<Set>());
                    auto& dict = getDictionary();
                    size_t index = 0;
                    for (auto&& item : *list) {
                        (*dict)[index++ ^ hashbits] = item;
                    }
                }
                break;
                }
                break;
            }
        }
    }

    // convert this value to the newType even if it's a downcast
    void Value::hardconvert(Type newType) {
        if (newType >= getType()) {
            upconvert(newType);
        } else {
            switch (newType) {
            default:
                throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                break;
            case Type::Null:
                *this = Value(Int(0));
                break;
            case Type::Int:
                switch (getType()) {
                default:
                    break;
                case Type::Float:
                    *this = Value((Int)getFloat());
                    break;
                case Type::Char:
                    *this = Value((Int)getChar());
                    break;
                case Type::String:
                    *this = Value((Int)toDouble(getString()));
                    break;
                case Type::Array:
                    *this = Value((Int)getArray().size());
                    break;
                case Type::List:
                    *this = Value((Int)getList().size());
                    break;
                }
                break;
            case Type::Float:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::String:
                    *this = Value((Float)toDouble(getString()));
                    break;
                case Type::Array:
                    *this = Value((Float)getArray().size());
                    break;
                case Type::List:
                    *this = Value((Float)getList().size());
                    break;
                }
                break;
            case Type::String:
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Char:
                {
                    std::string s;
                    if (getChar() <= 0x7F) s += (char)getChar();
                    else s += "?";
                    *this = Value(s);
                    break;
                }
                case Type::Array:
                {
                    std::string newval;
                    auto& arr = getArray();
                    switch (arr.getType()) {
                    case Type::Int:
                        for (auto&& item : arr.value.asInt) {
                            newval += Value(item).getPrintString() + ", ";
                        }
                        break;
                    case Type::Float:
                        for (auto&& item : arr.value.asFloat) {
                            newval += Value(item).getPrintString() + ", ";
                        }
                        break;
                    //case Type::Vec3:
                    //    for (auto&& item : get<vector<vec3>>(arr.value)) {
                    //        newval += Value(item).getPrintString() + ", ";
                    //    }
                    //    break;
                    case Type::String:
                        for (auto&& item : arr.value.asString) {
                            newval += Value(item).getPrintString() + ", ";
                        }
                        break;
                    default:
                        throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                        break;
                    }
                    if (arr.size()) {
                        newval.pop_back();
                        newval.pop_back();
                    }
                    *this = Value(newval);
                }
                break;
                case Type::List:
                {
                    std::string newval;
                    auto& list = getList();
                    for (auto val : list) {
                        newval += val->getPrintString() + ", ";
                    }
                    if (newval.size()) {
                        newval.pop_back();
                        newval.pop_back();
                    }
                    *this = Value(newval);
                }
                break;
                case Type::Map:
                {
                    std::string newval;
                    auto& dict = getDictionary();
                    for (auto&& val : *dict) {
                        newval += "`"s + std::to_string(val.first)  + ": " + val.second->getPrintString() + "`, ";
                    }
                    if (newval.size()) {
                        newval.pop_back();
                        newval.pop_back();
                    }
                    *this = Value(newval);
                }
                break;
                case Type::Class:
                {
                    auto& strct = getClass();
                    std::string newval = strct->name + ":\n"s;
                    for (auto&& val : strct->variables) {
                        newval += "`"s + val.first + ": " + val.second->getPrintString() + "`\n";
                    }
                    *this = Value(newval);
                }
                break;
                }
                break;
            case Type::Array:
            {
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Map:
                {
                    Array arr;
                    auto dict = getDictionary();
                    auto listType = dict->begin()->second->getType();
                    switch (listType) {
                    case Type::Int:
                        arr = Array(std::vector<Int>{});
                        for (auto&& item : *dict) {
                            if (item.second->getType() == listType) {
                                arr.push_back(item.second->getInt());
                            }
                        }
                        break;
                    case Type::Float:
                        arr = Array(std::vector<Float>{});
                        for (auto&& item : *dict) {
                            if (item.second->getType() == listType) {
                                arr.push_back(item.second->getFloat());
                            }
                        }
                        break;
                    //case Type::Vec3:
                    //    arr = Array(vector<vec3>{});
                    //    for (auto&& item : *dict) {
                    //        if (item.second->getType() == listType) {
                    //            arr.push_back(item.second->getVec3());
                    //        }
                    //    }
                    //    break;
                    case Type::Function:
                        arr = Array(std::vector<FunctionRef>{});
                        for (auto&& item : *dict) {
                            if (item.second->getType() == listType) {
                                arr.push_back(item.second->getFunction());
                            }
                        }
                        break;
                    case Type::String:
                        arr = Array(std::vector<std::string>{});
                        for (auto&& item : *dict) {
                            if (item.second->getType() == listType) {
                                arr.push_back(item.second->getString());
                            }
                        }
                        break;
                    default:
                        throw Exception("Array cannot contain collections");
                        break;
                    }
                    *this = Value(arr);
                }
                break;
                case Type::List:
                {
                    auto list = getList();
                    auto listType = list[0]->getType();
                    Array arr;
                    switch (listType) {
                    case Type::Null:
                        arr = Array(std::vector<Int>{});
                        break;
                    case Type::Int:
                        arr = Array(std::vector<Int>{});
                        for (auto&& item : list) {
                            if (item->getType() == listType) {
                                arr.push_back(item->getInt());
                            }
                        }
                        break;
                    case Type::Float:
                        arr = Array(std::vector<Float>{});
                        for (auto&& item : list) {
                            if (item->getType() == listType) {
                                arr.push_back(item->getFloat());
                            }
                        }
                        break;
                    //case Type::Vec3:
                    //    arr = Array(vector<vec3>{});
                    //    for (auto&& item : list) {
                    //        if (item->getType() == listType) {
                    //            arr.push_back(item->getVec3());
                    //        }
                    //    }
                    //    break;
                    case Type::Function:
                        arr = Array(std::vector<FunctionRef>{});
                        for (auto&& item : list) {
                            if (item->getType() == listType) {
                                arr.push_back(item->getFunction());
                            }
                        }
                        break;
                    case Type::String:
                        arr = Array(std::vector<std::string>{});
                        for (auto&& item : list) {
                            if (item->getType() == listType) {
                                arr.push_back(item->getString());
                            }
                        }
                        break;
                    default:
                        throw Exception("Array cannot contain collections");
                        break;
                    }
                    *this = Value(arr);
                }
                break;
                }
                break;
            }
            break;
            case Type::List:
            {
                switch (getType()) {
                default:
                    throw Exception("Conversion not defined for types `"s + getTypeName(getType()) + "` to `" + getTypeName(newType) + "`");
                    break;
                case Type::Map:
                {
                    List list;
                    for (auto&& item : *getDictionary()) {
                        list.push_back(item.second);
                    }
                    *this = Value(list);
                }
                break;
                case Type::Class:
                    *this = Value(List({ std::make_shared<Value>(*this) }));
                    break;
                }
            }
            break;
            case Type::Map:
            {
                Dictionary dict;
                for (auto&& item : getClass()->variables) {
                    dict[std::hash<std::string>()(item.first) ^ typeHashBits(Type::String)] = item.second;
                }
            }
            }
        }
    }

std::string Value::getPrintString() const {
    if (getType() == Type::Char) {
        std::string s;
        if (value.asChar <= 0x7F) s += (char)value.asChar;
        else {
            // UTF-8 encode code point
            char32_t cp = value.asChar;
            if (cp < 0x80) s += (char)cp;
            else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
            else if (cp < 0x10000) { s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
            else { s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 0x3F)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
        }
        return s;
    }
    if (getType() == Type::Range) {
        return std::to_string(value.asRange.start) + (value.asRange.inclusive ? "..=" : "..") + std::to_string(value.asRange.end_);
    }
    if (getType() == Type::Tuple) {
        std::string s = "(";
        for (size_t i = 0; i < value.asList.size(); ++i) {
            if (i > 0) s += ", ";
            s += value.asList[i]->getPrintString();
        }
        s += ")";
        return s;
    }
    auto t = *this;
    t.hardconvert(Type::String);
    return t.value.asString;
}
}



