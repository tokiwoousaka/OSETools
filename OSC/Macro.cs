using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;

namespace OSC
{
    public class MacroValues
    {
        public IDictionary<string, int> Data { get; private set; }

        public MacroValues(params Expression<Func<string, int>>[] values)
        {
            Data = values.ToDictionary(x => x.Parameters[0].Name, x => x.Compile()(""));
        }

    }

    public static class Macro
    {
        /// <summary>
        /// <para>{to} ... label number</para>
        /// </summary>
        public static readonly string Goto = "03 3F {to}";

        /// <summary>
        /// <para>{0] ... 1 or other number</para>
        /// <para>{to} ... label number</para>
        /// </summary>
        public static readonly string If = "04 {0} 03 3F {to}";

        /// <summary>
        /// <para>{0} ... CMPxx operation</para>
        /// <para>{1} ... CMPxx left arg</para>
        /// <para>{2} ... CMPxx right arg</para>
        /// <para>{to} ... label number</para>
        /// </summary>
        public static readonly string CompIf = "{0} 3F {1} {2} 04 3F 03 3F {to}";
    }
}
