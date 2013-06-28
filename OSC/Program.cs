
using System.Linq;
using System.Collections.Generic;
using System;
using System.Text;
using System.IO;
using Alice.Extensions;

namespace OSC
{
    class Test
    {
        public static void Main()
        {
            var nums = new Numbers<byte>(0, 32, x => (byte)x);
            var count = 1;
            foreach (var i in nums)
            {
                Console.Write(i + " ");
                if (count % 2 == 0)
                    nums.Free(nums.Locked.Randomize().First());
                count++;
            }
            Console.WriteLine();
            Console.WriteLine(count);

            using(var str = File.Open("test.ose", FileMode.OpenOrCreate, FileAccess.Write))
            using(var ose = new OSEGenerator(str))
            {
             
                ose.Emit(OSECode6.LoadImm, 0x00, 0x12345678);
                ose.Emit(OSECode6.LoadImm, 0x01, 0x24682468);
                ose.Emit(OSECode4.CompLess, 0x02, 0x00, 0x01);
                ose.Emit(OSECode3.Copy, 0x03, 0x02);
                ose.EmitMacro(Macro.CompIf, new MacroValues(to => 0x1ABE1), (byte)OSECode4.CompEq, 0x01, 0x02);
            }

            var read = from x in File.OpenRead("test.ose").ToUsing()
                       from y in new BinaryReader(x).ToUsing()
                       select y.ReadBytes((int)x.Length);

            foreach (var b in read)
                Console.Write(b.ToString("X2") + " ");
            
            Console.Read();
        }
    }


}