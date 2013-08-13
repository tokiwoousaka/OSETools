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
            var code = args.Select(x => from a in File.OpenRead(x).ToUsing()
                                        from b in new StreamReader(a).ToUsing()
                                        select b.ReadToEnd())
                           .JoinToString()
                           .Replace(" ", "")
                           .Replace("\r", "")
                           .Split('\n');
                       

            var cmp = new FulynCompiler("stdlib.ose");
            cmp.Parse(code);
            cmp.Compile();

            var read = from x in File.OpenRead("stdlib.ose").ToUsing()
                       from y in new BinaryReader(x).ToUsing()
                       select y.ReadBytes((int)x.Length);

            foreach (var b in read)
                Console.Write(b.ToString("X2") + " ");
            
            Console.Read();
		}
	}
}