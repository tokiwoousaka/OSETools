using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn.AST
{
    // <stmt> ::= ( <declare> | <subst> ) '\n'
    public interface IStmt
    {
    }

    // <declare> ::= <identity> '::' <type> 
    public class Declare : IStmt, IMember
    {
        public string Identity { get; set; }
        public IType Type { get; set; }
    }

    public class Inline : IStmt
    {
        public string Text { get; set; }
    }

    // <subst> ::= ( <identity> '=' |  [ ( '_' | '$' ) '=' ] ) <expr>
    public class Subst : IStmt
    {
        public string Identity { get; set; }
        public IExpr Expr { get; set; }
        public static readonly string Trash = "_";
        public static readonly string Break = "$";
    }
}
