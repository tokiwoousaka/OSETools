using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Alice.Functional.Monads;

namespace Alice.Functional
{
    static class BindExtension
    {
        public static Error<TResult> SelectMany<T, TResult>
            (this Error<T> source, Func<T, Error<TResult>> selector)
        {
            return source.IsError ? new Error<TResult>(source.Exception) : selector(source.Value);
        }
    }
}
