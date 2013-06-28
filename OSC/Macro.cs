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
        /// <para>{0} ... label pointer register</para>
        /// </summary>
        public static readonly string PGoto = "1E 3F {0}";

        /// <summary>
        /// <para>{0} ... 1 or other number</para>
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

        /// <summary>
        /// <para>{return_label} ... return label</para>
        /// <para>{func_label} ... function label</para>
        /// </summary>
        public static readonly string FuncCall = "03 30 {return_label} 03 3F {func_label} 01 01 {return_label}";

        /// <summary>
        /// <para>{label} ... function label number</para>
        /// </summary>
        public static readonly string FuncStart = "01 01 {label} FE 01 00 3C 00 20 20 00 00 00";

        /// <summary>
        /// none
        /// </summary>
        public static readonly string FuncEnd = "3D 00 20 20 00 00 00 1E 3F 30";
    }
}
