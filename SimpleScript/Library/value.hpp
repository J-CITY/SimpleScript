#pragma once

#include <ostream>
#include "types.hpp"
#include "utf8Utils.hpp"
namespace IkigaiScript {
    struct Null {};

    
    struct Value {
        TypeDescriptor typeDescriptor;

        union Payload {
            Null asNull;
            Bool asBool;
            char32_t asChar;
            Int asInt;
            Float asFloat;
            FunctionRef asFunction;
            CoroRef asCoro;
            void* asPointer;
            std::string asString;
            Array asArray;
            List asList;
            SetRef asSet;
            DictionaryRef asDictionary;
            ClassRef asClass;
            RangeValue asRange;
            ValuePtr asOptional;
            ResultDataPtr asResult;

            Payload() {}
            ~Payload() {}
        } value;

        void destroy() {
            switch(getType()) {
            case Type::Function: value.asFunction.~shared_ptr(); break;
            case Type::Coro: value.asCoro.~shared_ptr(); break;
            case Type::String: value.asString.~basic_string(); break;
            case Type::Array: value.asArray.~Array(); break;
            case Type::List:
            case Type::Tuple: value.asList.~vector(); break;
            case Type::Set: value.asSet.~shared_ptr(); break;
            case Type::Map: value.asDictionary.~shared_ptr(); break;
            case Type::Class: value.asClass.~shared_ptr(); break;
            case Type::Optional: value.asOptional.~shared_ptr(); break;
            case Type::Result: value.asResult.~shared_ptr(); break;
            default: break;
            }
        }

        void copyFrom(const Value& o) {
            typeDescriptor = o.typeDescriptor;
            switch(getType()) {
            case Type::Null: new (&value.asNull) Null(o.value.asNull); break;
            case Type::Bool: new (&value.asBool) Bool(o.value.asBool); break;
            case Type::Char: new (&value.asChar) char32_t(o.value.asChar); break;
            case Type::Int: new (&value.asInt) Int(o.value.asInt); break;
            case Type::Float: new (&value.asFloat) Float(o.value.asFloat); break;
            case Type::Pointer: new (&value.asPointer) void*(o.value.asPointer); break;
            case Type::Function: new (&value.asFunction) FunctionRef(o.value.asFunction); break;
            case Type::Coro: new (&value.asCoro) CoroRef(o.value.asCoro); break;
            case Type::String: new (&value.asString) std::string(o.value.asString); break;
            case Type::Array: new (&value.asArray) Array(o.value.asArray); break;
            case Type::List: new (&value.asList) List(o.value.asList); break;
            case Type::Set: new (&value.asSet) SetRef(o.value.asSet); break;
            case Type::Map: new (&value.asDictionary) DictionaryRef(o.value.asDictionary); break;
            case Type::Class: new (&value.asClass) ClassRef(o.value.asClass); break;
            case Type::Tuple: new (&value.asList) List(o.value.asList); break;
            case Type::Range: new (&value.asRange) RangeValue(o.value.asRange); break;
            case Type::Optional: new (&value.asOptional) ValuePtr(o.value.asOptional); break;
            case Type::Result: new (&value.asResult) ResultDataPtr(o.value.asResult); break;
            default: break;
            }
        }

        void moveFrom(Value&& o) {
            typeDescriptor = std::move(o.typeDescriptor);
            switch(getType()) {
            case Type::Null: new (&value.asNull) Null(std::move(o.value.asNull)); break;
            case Type::Bool: new (&value.asBool) Bool(std::move(o.value.asBool)); break;
            case Type::Char: new (&value.asChar) char32_t(std::move(o.value.asChar)); break;
            case Type::Int: new (&value.asInt) Int(std::move(o.value.asInt)); break;
            case Type::Float: new (&value.asFloat) Float(std::move(o.value.asFloat)); break;
            case Type::Pointer: new (&value.asPointer) void*(std::move(o.value.asPointer)); break;
            case Type::Function: new (&value.asFunction) FunctionRef(std::move(o.value.asFunction)); break;
            case Type::Coro: new (&value.asCoro) CoroRef(std::move(o.value.asCoro)); break;
            case Type::String: new (&value.asString) std::string(std::move(o.value.asString)); break;
            case Type::Array: new (&value.asArray) Array(std::move(o.value.asArray)); break;
            case Type::List: new (&value.asList) List(std::move(o.value.asList)); break;
            case Type::Set: new (&value.asSet) SetRef(std::move(o.value.asSet)); break;
            case Type::Map: new (&value.asDictionary) DictionaryRef(std::move(o.value.asDictionary)); break;
            case Type::Class: new (&value.asClass) ClassRef(std::move(o.value.asClass)); break;
            case Type::Tuple: new (&value.asList) List(std::move(o.value.asList)); break;
            case Type::Range: new (&value.asRange) RangeValue(std::move(o.value.asRange)); break;
            case Type::Optional: new (&value.asOptional) ValuePtr(std::move(o.value.asOptional)); break;
            case Type::Result: new (&value.asResult) ResultDataPtr(std::move(o.value.asResult)); break;
            default: break;
            }
        }

        explicit Value() { typeDescriptor = TypeDescriptor{Type::Null, true, true, true, false }; new (&value.asNull) Null(); }
        explicit Value(bool a) { typeDescriptor = TypeDescriptor{ Type::Bool, true, false, true, false }; new (&value.asBool) Bool(a); }
        explicit Value(char32_t a) { typeDescriptor = TypeDescriptor{ Type::Char, true, false, true, false }; new (&value.asChar) char32_t(a); }
        explicit Value(Int a) { typeDescriptor = TypeDescriptor{ Type::Int, true, false, true, false }; new (&value.asInt) Int(a); }
        explicit Value(size_t a) { typeDescriptor = TypeDescriptor{ Type::Int, true, false, true, false }; new (&value.asInt) Int(a); }
        explicit Value(Float a) { typeDescriptor = TypeDescriptor{ Type::Float, true, false, true, false }; new (&value.asFloat) Float(a); }
        explicit Value(FunctionRef a) { typeDescriptor = TypeDescriptor{ Type::Function, true, false, true, false }; new (&value.asFunction) FunctionRef(a); }
        explicit Value(CoroRef a) { typeDescriptor = TypeDescriptor{ Type::Coro, true, false, true, false }; new (&value.asCoro) CoroRef(a); }
        // TaskRef is an alias for CoroRef so constructor above covers it
        explicit Value(void* a) { typeDescriptor = TypeDescriptor{ Type::Pointer, true, false, true, false }; new (&value.asPointer) void*(a); }
        explicit Value(std::string a) { typeDescriptor = TypeDescriptor{ Type::String, true, false, true, false }; new (&value.asString) std::string(a); }
        explicit Value(const char* a) { typeDescriptor = TypeDescriptor{ Type::String, true, false, true, false }; new (&value.asString) std::string(a); }
        explicit Value(Array a) { typeDescriptor = TypeDescriptor{ Type::Array, true, false, true, false }; new (&value.asArray) Array(a); }
        explicit Value(List a) { typeDescriptor = TypeDescriptor{ Type::List, true, false, true, false }; new (&value.asList) List(a); }
        explicit Value(SetRef a) { typeDescriptor = TypeDescriptor{ Type::Set, true, false, true, false }; new (&value.asSet) SetRef(a); }
        explicit Value(DictionaryRef a) { typeDescriptor = TypeDescriptor{ Type::Map, true, false, true, false }; new (&value.asDictionary) DictionaryRef(a); }
        explicit Value(ClassRef a) { typeDescriptor = TypeDescriptor{ Type::Class, true, false, true, false }; new (&value.asClass) ClassRef(a); }
        explicit Value(ValuePtr o) { copyFrom(*o); }
        explicit Value(RangeValue a) { typeDescriptor = TypeDescriptor{ Type::Range, true, false, true, false }; new (&value.asRange) RangeValue(a); }
        // Tuple: constructed from a List, stored as asList with Type::Tuple
        static Value makeTuple(List items) {
            Value v;
            v.destroy();
            v.typeDescriptor = TypeDescriptor{ Type::Tuple, true, false, true, false };
            new (&v.value.asList) List(std::move(items));
            return v;
        }

        static Value makeOptional(ValuePtr val, std::shared_ptr<TypeDescriptor> subtype = nullptr) {
            Value v;
            v.destroy();
            v.typeDescriptor = TypeDescriptor{ Type::Optional, true, false, true, false };
            if (subtype) {
                v.typeDescriptor.subtype = subtype;
            } else if (val) {
                v.typeDescriptor.subtype = std::make_shared<TypeDescriptor>(val->typeDescriptor);
            } else {
                v.typeDescriptor.subtype = std::make_shared<TypeDescriptor>(TypeDescriptor{ Type::Null, true, true, true, false });
            }
            new (&v.value.asOptional) ValuePtr(val);
            return v;
        }

        ValuePtr getOptional() const {
            if (getType() != Type::Optional) throw Exception("Value is not an Optional");
            return value.asOptional;
        }

        static Value makeResultOk(ValuePtr val, std::shared_ptr<TypeDescriptor> okType = nullptr, std::shared_ptr<TypeDescriptor> errType = nullptr) {
            Value v;
            v.destroy();
            v.typeDescriptor = TypeDescriptor{ Type::Result, true, false, true, false };
            if (okType) {
                v.typeDescriptor.subtype = okType;
            } else if (val) {
                v.typeDescriptor.subtype = std::make_shared<TypeDescriptor>(val->typeDescriptor);
            } else {
                v.typeDescriptor.subtype = std::make_shared<TypeDescriptor>(TypeDescriptor{ Type::Null, true, true, true, false });
            }
            if (errType) {
                v.typeDescriptor.subtype2 = errType;
            } else {
                v.typeDescriptor.subtype2 = std::make_shared<TypeDescriptor>(TypeDescriptor{ Type::Null, true, true, false, true }); // default: Dynamic
            }
            auto data = std::make_shared<ResultData>();
            data->isOk = true;
            data->value = val ? val : std::make_shared<Value>();
            new (&v.value.asResult) ResultDataPtr(data);
            return v;
        }

        static Value makeResultErr(ValuePtr err, std::shared_ptr<TypeDescriptor> okType = nullptr, std::shared_ptr<TypeDescriptor> errType = nullptr) {
            Value v;
            v.destroy();
            v.typeDescriptor = TypeDescriptor{ Type::Result, true, false, true, false };
            if (okType) {
                v.typeDescriptor.subtype = okType;
            } else {
                v.typeDescriptor.subtype = std::make_shared<TypeDescriptor>(TypeDescriptor{ Type::Null, true, true, false, true }); // default: Dynamic
            }
            if (errType) {
                v.typeDescriptor.subtype2 = errType;
            } else if (err) {
                v.typeDescriptor.subtype2 = std::make_shared<TypeDescriptor>(err->typeDescriptor);
            } else {
                v.typeDescriptor.subtype2 = std::make_shared<TypeDescriptor>(TypeDescriptor{ Type::Null, true, true, true, false });
            }
            auto data = std::make_shared<ResultData>();
            data->isOk = false;
            data->value = err ? err : std::make_shared<Value>();
            new (&v.value.asResult) ResultDataPtr(data);
            return v;
        }

        const ResultData& getResult() const {
            if (getType() != Type::Result) throw Exception("Value is not a Result");
            return *value.asResult;
        }

        bool resultIsOk() const {
            if (getType() != Type::Result) throw Exception("Value is not a Result");
            return value.asResult->isOk;
        }

        ValuePtr resultPayload() const {
            if (getType() != Type::Result) throw Exception("Value is not a Result");
            return value.asResult->value;
        }

        Value(const Value& o) { copyFrom(o); }
        Value(Value&& o) noexcept { moveFrom(std::move(o)); }
        ~Value() { destroy(); }

        Value& operator=(const Value& o) {
            if (this != &o) {
                destroy();
                copyFrom(o);
            }
            return *this;
        }

        Value& operator=(Value&& o) noexcept {
            if (this != &o) {
                destroy();
                moveFrom(std::move(o));
            }
            return *this;
        }

        Type getType() const {
            return typeDescriptor.type;
        }
        
        std::string getPrintString() const;
        
        char32_t& getChar() { return value.asChar; }
        Int& getInt() { return value.asInt; }
        Float& getFloat() { return value.asFloat; }
        CoroRef& getCoro() { return value.asCoro; }
        TaskRef& getTask() { return value.asCoro; }   // alias: Task == Coro
        FunctionRef& getFunction() { return value.asFunction; }
        void*& getPointer() { return value.asPointer; }
        std::string& getString() { return value.asString; }
        Array& getArray() { return value.asArray; }
        template <typename T>
        std::vector<T>& getStdVector();
        List& getList();


        SetRef& getSet() {
            return value.asSet;
        }

        DictionaryRef& getDictionary() {
            return value.asDictionary;
        }

        ClassRef& getClass() {
            return value.asClass;
        }

        RangeValue& getRange() { return value.asRange; }
        List& getTuple() { return value.asList; }  // stored in asList
        
        bool getBool() {
            bool truthiness = false;
            switch (getType()) {
            case Type::Bool:
                truthiness = value.asBool;
                break;
            case Type::Char:
                truthiness = value.asChar != 0;
                break;
            case Type::Int:
                truthiness = getInt();
                break;
            case Type::Float:
                truthiness = getFloat() != 0;
                break;
            //case Type::Vec3:
            //    truthiness = getVec3() != vec3(0);
            //    break;
            case Type::String:
                truthiness = getString().size() > 0;
                break;
            case Type::Array:
                truthiness = getArray().size() > 0;
                break;
            case Type::List:
                truthiness = getList().size() > 0;
                break;
            case Type::Coro:
                truthiness = getCoro()->isActive();
                break;
            default:
                break;
            }
            return truthiness;
        }

        // ReSharper disable once CppInconsistentNaming
        size_t getHash();
        
        void upconvert(Type newType);
        
        void hardconvert(Type newType);
    };

    template <typename T>
    std::vector<T>& Value::getStdVector() {
        return value.asArray.getStdVector<T>();
    }

    inline std::ostream& operator<<(std::ostream& os, const Null&) {
        os << "null";
        return os;
    }
    inline std::ostream& operator<<(std::ostream& os, const List& values) {
        os << Value(values).getPrintString();
        return os;
    }
    
    inline std::ostream& operator<<(std::ostream& os, const Array& arr) {
        os << Value(arr).getPrintString();

        return os;
    }
    inline std::ostream& operator<<(std::ostream& os, const DictionaryRef& dict) {
        os << Value(dict).getPrintString();

        return os;
    }
    inline std::ostream& operator<<(std::ostream& os, const ClassRef& dict) {
        os << Value(dict).getPrintString();

        return os;
    }
    inline void upconvert(Value& a, Value& b) {
        if (a.getType() != b.getType()) {
            if (a.getType() < b.getType()) {
                a.upconvert(b.getType());
            } else {
                b.upconvert(a.getType());
            }
        }
    }
    
    inline void upconvertThrowOnNonNumberToNumberCompare(Value& a, Value& b) {
        if (a.getType() != b.getType()) {
            if (std::max((int)a.getType(), (int)b.getType()) > (int)Type::Float) {
                throw Exception(
                    "Types `"s + getTypeName(a.getType()) + " " + a.getPrintString() + "` and `" 
                    + getTypeName(b.getType()) + " " + b.getPrintString() + "` are incompatible for this operation");
            }
            if (a.getType() < b.getType()) {
                a.upconvert(b.getType());
            } else {
                b.upconvert(a.getType());
            }
        }
    }
    
    inline Value operator+(Value a, Value b) {
        upconvert(a, b);
        auto applyType = [](Value& val) {
            val.typeDescriptor.type = val.getType();
            val.typeDescriptor.isConst = true;
            val.typeDescriptor.isNullable = false;
            val.typeDescriptor.isInit = true;
            val.typeDescriptor.isDynamic = false;
        };
        switch (a.getType()) {
        case Type::Bool:
        {
            auto val = Value{ a.getBool() || b.getBool() };
            applyType(val);
            return val;
            break;
        }
        case Type::Char:
        {
            if (b.getType() == Type::String) {
                // Convert Char to UTF-8 string then concatenate
                std::string s;
                utf8Encode(a.getChar(), s);
                auto val = Value{ s + b.getString() };
                applyType(val);
                return val;
            } else {
                auto val = Value{ (char32_t)(a.getChar() + b.getChar()) };
                applyType(val);
                return val;
            }
            break;
        }
       	case Type::Int:
        {
            auto val = Value{ a.getInt() + b.getInt() };
            applyType(val);
            return val;
            break;
        }
        case Type::Float:
        {
            auto val = Value{ a.getFloat() + b.getFloat() };
            applyType(val);
            return val;
            break;
        }
        case Type::String:
        {
            auto val = Value{ a.getString() + b.getString() };
            applyType(val);
            return val;
            break;
        }
        case Type::Array:
        {
            auto arr = Array(a.getArray());
            arr.push_back(b.getArray());
            return Value{ arr };
        }
        break;
        case Type::List:
        {
            auto list = List(a.getList());
            auto& blist = b.getList();
            list.insert(list.end(), blist.begin(), blist.end());
            return Value{ list };
        }
        break;
        case Type::Set:
        {
            auto list = make_shared<Set>(*a.getSet());
            list->merge(*b.getSet());
            return Value{ list };
        }
        break;
        case Type::Map:
        {
            auto list = make_shared<Dictionary>(*a.getDictionary());
            list->merge(*b.getDictionary());
            return Value{ list };
        }
        break;
        default:
            throw Exception("Operator + not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
    }

    inline Value operator-(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return Value{ a.getInt() - b.getInt() };
            break;
        case Type::Float:
            return Value{ a.getFloat() - b.getFloat() };
            break;
        //case Type::Vec3:
        //    return Value{ a.getVec3() - b.getVec3() };
        //    break;
        default:
            throw Exception("Operator - not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
    }

    inline Value operator*(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return Value{ a.getInt() * b.getInt() };
            break;
        case Type::Float:
            return Value{ a.getFloat() * b.getFloat() };
            break;
        //case Type::Vec3:
        //    return Value{ a.getVec3() * b.getVec3() };
        //    break;
        default:
            throw Exception("Operator * not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
    }

    inline Value operator/(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            if (b.getInt() == 0) {
                throw Exception("Division by zero");
            }
            return Value{ a.getInt() / b.getInt() };
            break;
        case Type::Float:
            return Value{ a.getFloat() / b.getFloat() };
            break;
        //case Type::Vec3:
        //    return Value{ a.getVec3() / b.getVec3() };
        //    break;
        default:
            throw Exception("Operator / not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
    }

    inline Value operator+=(Value& a, Value b) {
        if ((int)a.getType() < (int)Type::Array || b.getType() == Type::List) {
            upconvert(a, b);
        }
        switch (a.getType()) {
        case Type::Int:
            a.getInt() += b.getInt();
            break;
        case Type::Float:
            a.getFloat() += b.getFloat();
            break;
        //case Type::Vec3:
        //    a.getVec3() += b.getVec3();
        //    break;
        case Type::String:
            a.getString() += b.getString();
            break;
        case Type::Array:
        {
            auto& arr = a.getArray();
            if (arr.getType() == b.getType()
                || (b.getType() == Type::Array && b.getArray().getType() == arr.getType())) {
                switch (b.getType()) {
                case Type::Int:
                    arr.push_back(b.getInt());
                    break;
                case Type::Float:
                    arr.push_back(b.getFloat());
                    break;
                //case Type::Vec3:
                //    arr.push_back(b.getVec3());
                //    break;
                case Type::Function:
                    arr.push_back(b.getFunction());
                    break;
                case Type::String:
                    arr.push_back(b.getString());
                    break;
                case Type::Pointer:
                    arr.push_back(b.getPointer());
                    break;
                case Type::Array:
                    arr.push_back(b.getArray());
                    break;
                default:
                    break;
                }
            }
            return Value{ arr };
        }
        break;
        case Type::List:
        {
            auto& list = a.getList();
            switch (b.getType()) {
            case Type::Int:
            case Type::Float:
            //case Type::Vec3:
            case Type::Function:
            case Type::String:
            case Type::Pointer:
                list.push_back(std::make_shared<Value>(b));
                break;
            default:
            {
                b.upconvert(Type::List);
                auto& blist = b.getList();
                list.insert(list.end(), blist.begin(), blist.end());
            }
            break;
            }
        }
        break;
        case Type::Set:
        {
            auto& dict = a.getSet();
            b.upconvert(Type::Set);
            dict->merge(*b.getSet());
        }
        break;
       	case Type::Map:
        {
            auto& dict = a.getDictionary();
            b.upconvert(Type::Map);
            dict->merge(*b.getDictionary());
        }
        break;
        default:
            throw Exception("Operator += not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return a;
    }

    inline Value operator-=(Value& a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            a.getInt() -= b.getInt();
            break;
        case Type::Float:
            a.getFloat() -= b.getFloat();
            break;
        //case Type::Vec3:
        //    a.getVec3() -= b.getVec3();
        //    break;
        default:
            throw Exception("Operator -= not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return a;
    }

    inline Value operator*=(Value& a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            a.getInt() *= b.getInt();
            break;
        case Type::Float:
            a.getFloat() *= b.getFloat();
            break;
        //case Type::Vec3:
        //    a.getVec3() *= b.getVec3();
        //    break;
        default:
            throw Exception("Operator *= not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return a;
    }

    inline Value operator/=(Value& a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            a.getInt() /= b.getInt();
            break;
        case Type::Float:
            a.getFloat() /= b.getFloat();
            break;
        //case Type::Vec3:
        //    a.getVec3() /= b.getVec3();
        //    break;
        default:
            throw Exception("Operator /= not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return a;
    }

    inline Value operator%(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return Value{ a.getInt() % b.getInt() };
            break;
        case Type::Float:
            return Value{ std::fmod(a.getFloat(), b.getFloat()) };
            break;
        default:
            throw Exception("Operator %% not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
    }
    
    bool operator!=(Value a, Value b);
    inline bool operator==(Value a, Value b) {
        if (a.getType() != b.getType()) {
            return false;
        }
        switch (a.getType()) {
        case Type::Null:
            return true;
        case Type::Int:
            return a.getInt() == b.getInt();
            break;
        case Type::Float:
            return a.getFloat() == b.getFloat();
            break;
        //case Type::Vec3:
        //    return a.getVec3() == b.getVec3();
        //    break;
        case Type::String:
            return a.getString() == b.getString();
            break;
        case Type::Array:
            return a.getArray() == b.getArray();
            break;
        case Type::List:
        {
            auto& alist = a.getList();
            auto& blist = b.getList();
            if (alist.size() != blist.size()) {
                return false;
            }
            for (size_t i = 0; i < alist.size(); ++i) {
                if (*alist[i] != *blist[i]) {
                    return false;
                }
            }
            return true;
        }
        break;
        default:
            throw Exception("Operator == not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return true;
    }

    inline bool operator!=(Value a, Value b) {
        if (a.getType() != b.getType()) {
            return true;
        }
        switch (a.getType()) {
        case Type::Null:
            return false;
        case Type::Int:
            return a.getInt() != b.getInt();
            break;
        case Type::Float:
            return a.getFloat() != b.getFloat();
            break;
        //case Type::Vec3:
        //    return a.getVec3() != b.getVec3();
        //    break;
        case Type::String:
            return a.getString() != b.getString();
            break;
        case Type::Array:
        case Type::List:
            return !(a == b);
            break;
        default:
            throw Exception("Operator != not defined for type `"s + getTypeName(a.getType()) + "`");
            break;
        }
        return false;
    }

    inline bool operator||(Value& a, Value& b) {
        return a.getBool() || b.getBool();
    }

    inline bool operator&&(Value& a, Value& b) {
        return a.getBool() && b.getBool();
    }

    inline bool operator<(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return a.getInt() < b.getInt();
            break;
        case Type::Float:
            return a.getFloat() < b.getFloat();
            break;
        case Type::String:
            return a.getString() < b.getString();
            break;
        case Type::Array:
            return a.getArray().size() < b.getArray().size();
            break;
        case Type::List:
            return a.getList().size() < b.getList().size();
            break;
        case Type::Set:
            return a.getSet()->size() < b.getSet()->size();
            break;
       	case Type::Map:
            return a.getDictionary()->size() < b.getDictionary()->size();
            break;
        default:
            break;
        }
        return false;
    }

    inline bool operator>(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return a.getInt() > b.getInt();
            break;
        case Type::Float:
            return a.getFloat() > b.getFloat();
            break;
        case Type::String:
            return a.getString() > b.getString();
            break;
        case Type::Array:
            return a.getArray().size() > b.getArray().size();
            break;
        case Type::List:
            return a.getList().size() > b.getList().size();
            break;
        case Type::Set:
            return a.getSet()->size() > b.getSet()->size();
            break;
       	case Type::Map:
            return a.getDictionary()->size() > b.getDictionary()->size();
            break;
        default:
            break;
        }
        return false;
    }

    inline bool operator<=(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return a.getInt() <= b.getInt();
            break;
        case Type::Float:
            return a.getFloat() <= b.getFloat();
            break;
        case Type::String:
            return a.getString() <= b.getString();
            break;
        case Type::Array:
            return a.getArray().size() <= b.getArray().size();
            break;
        case Type::List:
            return a.getList().size() <= b.getList().size();
            break;
        case Type::Set:
            return a.getSet()->size() <= b.getSet()->size();
            break;
        case Type::Map:
            return a.getDictionary()->size() <= b.getDictionary()->size();
            break;
        default:
            break;
        }
        return false;
    }

    inline bool operator>=(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
        case Type::Int:
            return a.getInt() >= b.getInt();
            break;
        case Type::Float:
            return a.getFloat() >= b.getFloat();
            break;
        case Type::String:
            return a.getString() >= b.getString();
            break;
        case Type::Array:
            return a.getArray().size() >= b.getArray().size();
            break;
        case Type::List:
            return a.getList().size() >= b.getList().size();
            break;
        case Type::Set:
            return a.getSet()->size() >= b.getSet()->size();
            break;
        case Type::Map:
            return a.getDictionary()->size() >= b.getDictionary()->size();
            break;
        default:
            break;
        }
        return false;
    }
}
