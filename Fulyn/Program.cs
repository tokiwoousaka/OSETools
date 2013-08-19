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

        public static bool ReadableMode { get; set; }

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
  --silent, -s        ... silent mode
  --readable, -r      ... show binary more readable(compile a little slowly)");
                return;
            }

            var list = new List<string>(args);
            var output = "a.out";
            var executePath = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
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

                    else if (x.Contains("--readable") || x.Contains("-r"))
                    {
                        FulynOption.ReadableMode = true;
                        list.Remove(x);
                    }
                });
            if (!new[] { Path.Combine(Environment.CurrentDirectory, "stdlib.fl"), Path.Combine(executePath, "stdlib.fl") }.Any(File.Exists))
            {
                Console.WriteLine("fatal: Couldn't find stdlib.fl in " + Path.Combine(Environment.CurrentDirectory, "stdlib.fl") + " or " + Path.Combine(executePath, "stdlib.fl"));
                return;
            }

            var code = list.Concat(new []{"stdlib.fl"})
                           .Select(x => new[] { Path.Combine(Environment.CurrentDirectory,x), Path.Combine(executePath, x) }.First(File.Exists))
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