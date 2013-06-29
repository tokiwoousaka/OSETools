using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn.AST
{
    // <program> ::= ( ( <declare> | <func> ) '\n' )+
    public class FulynProg
    {
        public IMember[] Members { get; set; }
    }

    // <declare> | <func>
    public interface IMember
    {
    }

    // <func> ::= <identity> '(' <identity> { ',' <identity> } ')' '\n'
	//                <stmt>+
    //            'end'
    //            |
    //            <identity> = <lambda>
    public class Function : IMember
    {
        public string Identity { get; set; }
        public FuncType Type { get; set; }
        public string[] Args { get; set; }
        public IStmt[] Stmts { get; set; }
    }
}
