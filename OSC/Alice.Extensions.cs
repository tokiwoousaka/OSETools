using System;
using System.Linq;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace Alice.Extensions
{
    /// <summary>
    /// Alice's extension methods.
    /// </summary>
    public static class Extensions
    {
        /// <summary>
        /// Gets an enumerable object whose element is included in both of enumerable objects.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='target'>
        /// The enumerable object.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> And<T>(this IEnumerable<T> e, IEnumerable<T> target)
        {
            return 
                from x in e
                from y in target
                where x.Equals(y)
                select x;
        }
        
        /// <summary>
        /// Gets all combinations of each element of this and each element on the specified enumerable object.
        /// <example>
        /// new []{0,1,2}.Conbinate(new []{"a","b"}) -> {0,"a"},{0,"b"},{1,"a"},{1,"b"},{2,"a"},{2,"b"}
        /// </example>
        /// </summary>
        /// <param name="e">The enumerable object.</param>
        /// <param name="Target">The enumerable object.</param>
        /// <typeparam name="T1">The 1st type parameter.</typeparam>
        /// <typeparam name="T2">The 2nd type parameter.</typeparam>
        /// <returns>Tuples of conbinated objects.</returns>
        public static IEnumerable<Tuple<T1,T2>> Conbinate<T1, T2>(this IEnumerable<T1> e, IEnumerable<T2> target)
        {
            return
                from x in e
                from y in target
                select Tuple.Create(x, y);
        }
        
        /// <summary>
        /// Enumerates the lines of the specified stream reader.
        /// </summary>
        /// <returns>
        /// The lines.
        /// </returns>
        /// <param name='streamReader'>
        /// Stream reader.
        /// </param>
        public static IEnumerable<string> EnumerateLines(this StreamReader streamReader)
        {
            while(!streamReader.EndOfStream)
                yield return streamReader.ReadLine();
        }
        
        /// <summary>
        /// Performs the specified action on each element on the enumerable object.
        /// </summary>
        /// <param name="e">The enumerable object.</param>
        /// <param name="Action">Action.</param>
        /// <typeparam name="T">The 1st type parameter.</typeparam>
        public static void ForEach<T>(this IEnumerable<T> e, Action<T> action)
        {
            foreach(T item in e)
                action(item);
        }
        
        /// <summary>
        /// Joins the enumerable object to string.
        /// </summary>
        /// <returns>
        /// The string.
        /// </returns>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        public static string JoinToString<T>(this IEnumerable<T> e)
        {
            return string.Concat<T>(e);
        }
        
        /// <summary>
        /// Joins the enumerable object to string.
        /// </summary>
        /// <returns>
        /// The string.
        /// </returns>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='separator'>
        /// Separator.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static string JoinToString<T>(this IEnumerable<T> e, string separator)
        {
            return string.Join<T>(separator, e);
        }
                 
        /// <summary>
        /// Gets the enumerable object whose element is included in either this and the specified enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='target'>
        /// The enumerable object.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> Or<T>(this IEnumerable<T> e, IEnumerable<T> target)
        {
            return e.Union(target);
        }
        
        /// <summary>
        /// Randomize the specified enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> Randomize<T>(this IEnumerable<T> e)
        {
            IEnumerable<T> x = new T[0];
            var randomize = new Random();
            while(e.Any())
            {
                var rand = randomize.Next(e.Count());
                x = x.Concat(new []{e.Skip(rand).First()});               
                e = e.Skip(rand, 1);
            }
            return x;
        }
        
        /// <summary>
        /// Skip the element that is specified by the predicate.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='predicate'>
        /// Predicate.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> SkipWhile<T>(this IEnumerable<T> e, Func<T,int,bool> predicate)
        {
            var cnt = 0;
            foreach(var x in e)
            {
                if(!predicate(x, cnt))
                    yield return x;
                cnt++;
            }
        }
        
        /// <summary>
        /// Skip the element that is specified by the start index and the range.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='start'>
        /// Start index.
        /// </param>
        /// <param name='range'>
        /// Range.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> Skip<T>(this IEnumerable<T> e, int start, int range)
        {
            return SkipWhile(e, (_, x) => x >= start && x < start + range);
        }
        
        /// <summary>
        /// Split each element on the enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='separator'>
        /// Separator.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<IEnumerable<T>> Split<T>(this IEnumerable<T> e, params T[] separator)
        {
            return Split(e, separator.Contains);
        }
        
        /// <summary>
        /// Split each element on the enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='separator'>
        /// Separator.
        /// </param>
        /// <param name='options'>
        /// Options.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<IEnumerable<T>> Split<T>(this IEnumerable<T> e, T[] separator, StringSplitOptions options)
        {
            return Split(e, separator.Contains)
                  .Where(x => options == StringSplitOptions.RemoveEmptyEntries ? x.Any() : true);
        }
        
        /// <summary>
        /// Split each element on the enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='separator'>
        /// Separator.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<IEnumerable<T>> Split<T>(this IEnumerable<T> e, Func<T,bool> separator)
        {
            IEnumerable<T> x = new T[0];
            var count = 0;
            foreach(var t in e)
            {
                count++;
                if(separator(t))
                {
                    yield return x;
                    x = new T[0];
                }
                else
                {
                    x = x.Concat(new T[]{t});
                    if(!e.Skip(count).Any())
                        yield return x;
                }
            }
        }
        
        /// <summary>
        /// Split each element on the enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='separator'>
        /// Separator.
        /// </param>
        /// <param name='options'>
        /// Options.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<IEnumerable<T>> Split<T>(this IEnumerable<T> e, Func<T,bool> separator, StringSplitOptions options)
        {
            return Split(e, separator)
                  .Where(x => options == StringSplitOptions.RemoveEmptyEntries ? x.Any() : true);
        }
        
        /// <summary>
        /// Split the specified text by the regexes.
        /// </summary>
        /// <param name='e'>
        /// The text.
        /// </param>
        /// <param name='separator'>
        /// Regex separators.
        /// </param>
        public static IEnumerable<string> Split(this string e, params Regex[] separator)
        {
            return Split(e, separator, StringSplitOptions.None);
        }
        
        /// <summary>
        /// Split the specified text by the regexes.
        /// </summary>
        /// <param name='e'>
        /// The text.
        /// </param>
        /// <param name='separator'>
        /// Regex separators.
        /// </param>
        /// <param name='options'>
        /// Options.
        /// </param>
        public static IEnumerable<string> Split(this string e, IEnumerable<Regex> separator, StringSplitOptions options)
        {
            return 
                from x in separator
                from y in ToEnumerable(x.Matches(e))
                select y.Value;
        }
        
        static IEnumerable<Match> ToEnumerable(this MatchCollection e)
        {
            foreach(var x in e)
                yield return (Match)x;
        }
        
        /// <summary>
        /// Performs "Exclusive OR" to the elements in both of this and specified enumerable object.
        /// </summary>
        /// <param name='e'>
        /// The enumerable object.
        /// </param>
        /// <param name='target'>
        /// The enumerable object.
        /// </param>
        /// <typeparam name='T'>
        /// The 1st type parameter.
        /// </typeparam>
        public static IEnumerable<T> Xor<T>(this IEnumerable<T> e, IEnumerable<T> target)
        {
            return e.Where(x => !target.Any(y => y.Equals(x)))
                    .Concat(target.Where(x => !e.Any(y => y.Equals(x))));
        }
        
        /* 
        /// <summary>
     /// Converts camelCase text to snake_case.
     /// </summary>
     /// <returns>The snake_case text.</returns>
     /// <param name="e">The camelCase text.</param>
     public static string ToSnakeCase(this string e)
     {
         return e.Select(x => char.IsUpper(x) ? "_" + x.ToString().ToLower() : x.ToString()).JoinToString();
     }*/

    }
}

