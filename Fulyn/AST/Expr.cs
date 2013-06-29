using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn.AST
{
    // <expr> ::= <literal> | <call> | <match> | <optr> | <var>
    public interface IExpr
    {
        IType Type { get; }
    }

    // <var> ::= <identity>
    public class Variable : IExpr
    {
        public IType Type { get; set; }
        public string Identity { get; set; }
    }

    // <literal> ::= <number> | <lambda>
    public interface ILiteral : IExpr
    {
    }

    // <number> ::= ( '0' | '1' | ... '9' )+
    public class IntLiteral : ILiteral
    {
        IntType Type { get { return new IntType(); } }
        IType IExpr.Type { get { return Type; } }
        public int Value { get; set; }
    }

    // <lambda> ::= ( ( <identity> '=>' )+ | '()' '=>' ) <expr>
    public class LambdaLiteral : ILiteral
    {
        public FuncType Type { get; set; }
        IType IExpr.Type { get { return Type; } }
        public string[] Args { get; set; }
        public IExpr Expr { get; set; }
    }

    // <call> ::= <identity> '(' <expr> { ',' <expr> } ')'
    public class Call : IExpr
    {
        public FuncType Type { get; set; }
        IType IExpr.Type { get { return Type; } }
        public string Identity { get; set; }
        public IExpr[] Args { get; set; }
    }

    // <not-false> ::= '|' ( 'true' | <expr>) '->' <expr> 
	// <false> ::= '|' ( 'false' | '()' ) '->' <expr>
    //
    // <match> ::= '?' <expr> '\n'
	//            { <not-false> '\n' }
	// 		        <false>
	// 		 { '\n' <not-false> }
    public class Match : IExpr
    {
        public IType Type { get; set; }
        public Tuple<IExpr, IExpr> Cases { get; set; }
        public IExpr Otherwise { get; set; }
    }

    // <optr> ::= <expr> <identity> <expr>
    public class Optr : IExpr
    {
        public IType Type { get; set; }
        public string Identity { get; set; }
        public IExpr Left { get; set; }
        public IExpr Right { get; set; }
    }
}
