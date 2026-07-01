#pragma once
#include <map>
#include <string>
#include <variant>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace Visual {
	struct Pin;
	struct Link;
	struct Node;

    using Id = int;

    using ArrayType = std::variant<std::vector<bool>, std::vector<int>, std::vector<float>>;

    struct Value;

    class Class {
    public:
        std::string name;
        std::map<std::string, Value> members;
        Id classDeclarationId = 0;
    };

    using ValueType = std::variant<bool, int, float, std::string,
        ArrayType, std::vector<std::shared_ptr<Value>>, std::unordered_map<int, std::shared_ptr<Value>>,
		std::shared_ptr<Class>, void*>;

    struct Value {
	    std::optional<ValueType> value;
    };
    
    using ValuePtr = std::shared_ptr<Value>;
    

    using NodePtr = std::shared_ptr<Visual::Node>;
    using NodePtrOpt = std::optional<NodePtr>;

    using LinkPtr = std::shared_ptr<Visual::Link>;
    using LinkPtrOpt = std::optional<LinkPtr>;

    using PinPtr = std::shared_ptr<Visual::Pin>;
    using PinPtrOpt = std::optional<PinPtr>;

    struct Scope;

    using ScopePtr = std::shared_ptr<Visual::Scope>;


    struct Scope {
        ScopePtr parent;
        std::unordered_map<std::string, ValuePtr> variables;
    };

    //Maybe shoud use only Array and for array type use subtype
    enum class PinType {
        None = 0,

        Bool,
        Int,
        Float,
        String,

        ArrayBool,
        ArrayInt,
        ArrayFloat,
        List,
    	Map,

        Object,

    	Function,
        
        
        Container,

    	Any,

        Variable,

        Flow,
    	//Delegate,
        
        //Expression
    };

    struct PinTypeDescriptor;
    using PinSubtype = std::optional<std::variant<std::string, std::shared_ptr<PinTypeDescriptor>>>;
    struct PinTypeDescriptor {
        PinType type;
        PinSubtype subType;

        //PinTypeDescriptor(PinType t, PinSubtype st) : type(t), subType(st)
        //{
	    //    
        //}

        std::string& getStr() {
            return std::get<std::string>(subType.value());
        }
        std::shared_ptr<PinTypeDescriptor> getPinDescr() {
            if (!subType) {
                return std::make_shared<PinTypeDescriptor>(PinType::None);
            }
            return std::get<std::shared_ptr<PinTypeDescriptor>>(subType.value());
        }
    };

    enum class PinKind {
        Output,
        Input
    };

    enum class NodeType {
        ValueInt,
        ValueFloat,
        ValueBool,
        ValueString,
        ValueArrayInt,
        ValueArrayFloat,
        ValueArrayBool,
        ValueList,
        ValueMap,
        ValueClass,

        Variable,

        Operator,
        Equal,
        FunctionCall,
        FunctionDeclaration,

    	ClassDeclaration,
        ClassMethodCall,
        ClassMethodDeclaration,


        If,
        While,
        For,
        ForEach,

        Return,
        Continue,
        Break,
        Import,

        This,

        Comment,

        Start,
    };

    enum class OperatorType
    {
	    Plus,
        Minus,
        Multiply,
        Divided,

    	Less,
        EqualLess,
        Great,
        EqualGreat,
        Inequality,
        Equality,

        LogicalAnd,
        LogicalOr,
    };
}
