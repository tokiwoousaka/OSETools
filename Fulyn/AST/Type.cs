using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn.AST
{
    // <type> ::= 'int' | '[' [ <type> { '->' <type> } => ] <type> ']'
    public interface IType
    {
    }

    // 'int'
    public class IntType : IType
    {
    }

    // '[' [ <type> { '->' <type> } => ] <type> ']'
    public class FuncType : IType
    {
        public IType[] ArgsType { get; set; }
        public IType RerurnType { get; set; }
    }
}
