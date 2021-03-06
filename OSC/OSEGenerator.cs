﻿using Alice.Extensions;
using System;
using System.Globalization;
using System.IO;
using System.Linq;

namespace OSC
{
    /// <summary>
    /// OSECPU命令を作成します。
    /// </summary>
    public class OSEGenerator : IDisposable
    {
        BinaryWriter writer;

        public bool SilentMode { get; set; }
        public bool ReadableMode { get; set; }
        public Numbers<int> Labels { get; set; }
        public Numbers<byte> LocalRegister { get; set; }
        public Numbers<byte> GlobalRegister { get; set; }
        public Numbers<byte> FuncRegister { get; set; }
        public Numbers<byte> LocalPegister { get; set; }
        public Numbers<byte> GlobalPegister { get; set; }
        public Numbers<byte> FuncPegister { get; set; }
        
        public OSEGenerator(Stream output, bool silent = false, bool readable = false)
        {
            writer = new BinaryWriter(output);
            
            // (r|p)egisters
            Labels = new Numbers<int>(0, 4096, x => x);
            LocalRegister = new Numbers<byte>(0, 32, x => (byte)x);
            LocalPegister = new Numbers<byte>(1, 31, x => (byte)x);
            GlobalRegister = new Numbers<byte>(0x20, 8, x => (byte)x);
            GlobalPegister = new Numbers<byte>(0x20, 8, x => (byte)x);
            FuncRegister = new Numbers<byte>(0x31, 11, x => (byte)x);
            FuncPegister = new Numbers<byte>(0x31, 11, x => (byte)x);

            SilentMode = silent;
            ReadableMode = readable;

            // write signsture (OSE1, NOP)
            Emit(0x05, 0xE1, 0x00);
        }
        
        public void Dispose()
        {
            writer.Close();
            writer.Dispose();
        }
        
        public void Emit(params byte[] buf)
        {
            if (!SilentMode)
                Console.WriteLine("emit: " + (this.ReadableMode && OSECode.AllCodes.ContainsKey(buf.First()) ? OSECode.AllCodes[buf.First()] + " " + buf.Skip(1).Select(x => x.ToString("X2")).JoinToString(" ") : buf.Select(x => x.ToString("X2")).JoinToString(" ")));
        	writer.Write(buf);
        }
        
        public void Emit(OSECode1 code)
        {
            writer.Write((byte)code);
        }
        
        public void Emit(OSECode2 code, byte value)
        {
            Emit((byte)code, value);
        }
        
        public void Emit(OSECode3 code, byte left, byte right)
        {
            if(code == OSECode3.PCopy)
                Emit((byte)code, left, right);
            
            else
                Emit((byte)code, left, right, 0xFF);
        }
        
        public void Emit(OSECode4 code, byte to, byte left, byte right)
        {
            Emit((byte)code, to, left, right);
        }
        
        public void Emit(OSECode6 code, byte num, int value)
        {
            Emit(new[] { (byte)code, num }.Concat(BitConverter.GetBytes(value).Reverse()).ToArray());
        }
        
        public void EmitRem(byte remCode, params byte[] buf)
        {
            Emit(new[] { (byte)0xFE, remCode }.Concat(buf.Take(remCode)).ToArray());
        }

        public void EmitMacro(string macro, MacroValues values = null, params byte[] buf)
        {
            var conv = macro;
            if(values != null)
            foreach (var pair in values.Data)
                conv = conv.Replace("{" + pair.Key + "}", 
                    pair.Value.GetBytes()
                        .Select(x => x.ToString("X2"))
                        .JoinToString(" "));
            conv = string.Format(conv, buf.Select(x => x.ToString("X2")).ToArray());
            Emit(conv.Split(' ').Select(x => byte.Parse(x, NumberStyles.HexNumber)).ToArray());
        }
    }

    public static class BytesExtension
    {
        public static byte[] GetBytes(this int source)
        {
            return BitConverter.GetBytes(source).Reverse().Select(x => x).ToArray();
        }
    }
}
