using System.Linq;
using System.Collections.Generic;
using System;
using Alice.Extensions;

namespace Fulyn
{

	class Program
	{
		public static void Main(string[] args)
		{
            var code = @"
gcd :: [int => int => int]
   
   main
     a :: int; b :: int
     result = gcd(1547, ~
     2093)
     print(result)
   end
   
   gcd (x, y)
     tmp = x
     x = ? less(x, y) 
         | true -> y 
         | () -> x
     y = ? equal(tmp, x) 
         | true -> y 
         | () -> tmp 
     ? rem(x, y) 
     | 0 -> x  
     | () -> gcd(y, rem(x, y))
   end".Replace(" ","").Split(new []{@"
"}, StringSplitOptions.None);

            var cmp = new FulynCompiler();
            cmp.Parse(code);

            Console.Read();
		}
	}
}