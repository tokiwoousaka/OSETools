using System.Linq;
using System.Collections.Generic;
using System;
using System.IO;
using Alice.Extensions;

namespace Fulyn
{
    class FulynOption
    {
        public static bool SilentMode { get; set; }

        public static void DebugWrite(string text)
        {
            if(!SilentMode)
                Console.WriteLine(text);
        }
    }

	class Program
	{
		public static void Main(string[] args)
		{
            if (args.Length == 0)
            {
                Console.WriteLine(
@"Usage: 

fulyn [options] [filenames]

Options: 

  --output, -o [path] ... set output file name
  --silent, -s        ... silent mode ");
                return;
            }

            var list = new List<string>(args);
            var output = "a.out";
            list.ToArray().ForEach(x =>
                {
                    if ((x.Contains("--output") || x.Contains("-o")) && x.Contains("="))
                    {
                        output = x.Replace("--output", "").Replace("-o","").Replace(" ", "").Replace("=", "");
                        list.Remove(x);
                    }
                    else if (x.Contains("--silent") || x.Contains("-s"))
                    {
                        FulynOption.SilentMode = true;
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

            FulynOption.DebugWrite("compile log:");
            var cmp = new FulynCompiler(output);
            cmp.Parse(code);
            cmp.Compile();
		}
	}
}