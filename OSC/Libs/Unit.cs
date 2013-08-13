using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Alice.Functional.Monads;

namespace Alice.Functional
{
    public static class UnitExtemsion
    {
        public static Error<T> ToError<T>(this T value)
        {
            return new Error<T>(value);
        }

        public static Error<T> ToError<T>(this Func<T> function)
        {
            return new Error<T>(function);
        }
    }
}
