using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn.AST
{
    // <type> ::= 'int' | '[' [ <type> { '=>' <type> } => ] <type> ']'
    public interface IType
    {
        /// <summary>
        /// 型の等価性を調べます。
        /// </summary>
        bool IsSameType(IType e);
    }

    // 'int'
    public class IntType : IType
    {
        public bool IsSameType(IType obj)
        {
            return obj.GetType() == typeof(IntType);
        }
    }

    // '[' [ <type> { '=>' <type> } => ] <type> ']'
    public class FuncType : IType
    {
        public IType[] ArgsType { get; set; }
        public IType ReturnType { get; set; }
        public bool TailCall { get; set; }

        public bool IsSameType(IType obj)
        {
            var o = (FuncType)obj;
            return o.ArgsType.Zip(this.ArgsType, Tuple.Create).All(x => x.Item1.IsSameType(x.Item2)) && o.ReturnType.IsSameType(this.ReturnType);
        }
    }
}
