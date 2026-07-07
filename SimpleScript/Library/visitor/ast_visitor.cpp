#include "ast_visitor.hpp"

namespace IkigaiScript {

// ---------------------------------------------------------------------------
// Expression base — dispatches by dynamic type (fallback for unknown nodes)
// ---------------------------------------------------------------------------
void Expression::accept(AstVisitor& v) { v.visit(*this); }

// ---------------------------------------------------------------------------
// Concrete node accept() implementations — each calls the specific overload
// ---------------------------------------------------------------------------
void FunctionExpression::accept(AstVisitor& v)    { v.visit(*this); }
void MemberVariable::accept(AstVisitor& v)        { v.visit(*this); }
void MemberFunctionCall::accept(AstVisitor& v)    { v.visit(*this); }
void Return::accept(AstVisitor& v)                { v.visit(*this); }
void Yield::accept(AstVisitor& v)                  { v.visit(*this); }
void Continue::accept(AstVisitor& v)              { v.visit(*this); }
void Break::accept(AstVisitor& v)                 { v.visit(*this); }
void IfElse::accept(AstVisitor& v)                { v.visit(*this); }
void Loop::accept(AstVisitor& v)                  { v.visit(*this); }
void Foreach::accept(AstVisitor& v)               { v.visit(*this); }
void DeferExpression::accept(AstVisitor& v)       { v.visit(*this); }
void ResolveVar::accept(AstVisitor& v)            { v.visit(*this); }
void DefineVar::accept(AstVisitor& v)             { v.visit(*this); }
void LiveRebind::accept(AstVisitor& v)            { v.visit(*this); }
void NamedArgumentExpression::accept(AstVisitor& v){ v.visit(*this); }
void ValueNode::accept(AstVisitor& v)             { v.visit(*this); }
void BlockExpression::accept(AstVisitor& v)       { v.visit(*this); }
void MatchExpression::accept(AstVisitor& v)       { v.visit(*this); }
void TupleLiteralExpression::accept(AstVisitor& v){ v.visit(*this); }
void DestructuringAssign::accept(AstVisitor& v)   { v.visit(*this); }
void AwaitExpression::accept(AstVisitor& v)       { v.visit(*this); }
void SpawnExpression::accept(AstVisitor& v)       { v.visit(*this); }
void SyncBlockExpression::accept(AstVisitor& v)   { v.visit(*this); }
void RaceBlockExpression::accept(AstVisitor& v)   { v.visit(*this); }
void BranchBlockExpression::accept(AstVisitor& v) { v.visit(*this); }
void SafeBlockExpression::accept(AstVisitor& v)   { v.visit(*this); }

} // namespace IkigaiScript
