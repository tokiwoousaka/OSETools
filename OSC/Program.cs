using System.Linq;
using System.Collections.Generic;
using System;
using System.Text;
using System.IO;

namespace OSC
{
	class Test
	{
		public static void Main()
		{
			var sb = new StringBuilder();
			using(var str = File.Open("test.ose", FileMode.OpenOrCreate, FileAccess.Write))
	        using(var ose = new OSEGenerator(str))
			{
				ose.Emit(OSECode6.LoadImm, 0x00, 0x12345678);
				ose.Emit(OSECode6.LoadImm, 0x01, 0x24682468);
				ose.Emit(OSECode4.CompLess, 0x02, 0x00, 0x01);
			}
			
			using(var str = File.OpenRead("test.ose"))
			using(var rdr = new BinaryReader(str))
			{
				foreach(var b in rdr.ReadBytes((int)str.Length))
					Console.Write(b.ToString("X2") + " ");
                // 05 E1 00 02 00 12 34 56 78 02 01 24 68 24 68 22 02 00 01
			}
			
			Console.Read();
		}
	}
}