using System.Linq;
using System.Collections.Generic;
using System;
using System.IO;
using Alice.Extensions;

namespace Fulyn
{

	class Program
	{
		public static void Main(string[] args)
		{
            var list = new List<string>(args);
            var output = "a.out";
            list.ToArray().ForEach(x =>
                {
                    if (x.Contains("output") && x.Contains("="))
                    {
                        output = x.Replace("output", "").Replace(" ", "").Replace("=", "");
                        list.Remove(x);
                    }
                });

            var code = list.Concat(new []{"stdlib.fl"})
                           .Reverse()
                           .Select(x => from a in File.OpenRead(x).ToUsing()
                                        from b in new StreamReader(a).ToUsing()
                                        select b.ReadToEnd())
                           .JoinToString("\n")
                           .Replace(" ", "")
                           .Replace("\r", "")
                           .Replace("\t", "")
                           .Split('\n');

            File.Delete(output);

            Console.WriteLine("compile log:");
            var cmp = new FulynCompiler(output);
            cmp.Parse(code);
            cmp.Compile();
		}
	}
}