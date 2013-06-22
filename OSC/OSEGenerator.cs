using System.Linq;
using System.Collections.Generic;
using System;
using System.IO;

namespace OSC
{
    /// <summary>
    /// OSECPU命令を作成します。
    /// </summary>
    public class OSEGenerator : IDisposable
    {
        BinaryWriter writer;
        
        public OSEGenerator(Stream output)
        {
            writer = new BinaryWriter(output);        
            // write signsture (OSE1, NOP)
            this.Write(0x05, 0xE1, 0x00);
        }
        
        public void Dispose()
        {
            writer.Close();
            writer.Dispose();
        }
        
        void Write(params byte[] buf)
        {
            writer.Write(buf);
        }
        
        public void Emit(OSECode1 code)
        {
            writer.Write((byte)code);
        }
        
        public void Emit(OSECode2 code, byte value)
        {
            this.Write((byte)code, value);
        }
        
        public void Emit(OSECode3 code, byte left, byte right)
        {
            if(code == OSECode3.PCopy)
                this.Write((byte)code, left, right);
            
            else
                this.Write((byte)code, left, right, 0xFF);
        }
        
        public void Emit(OSECode4 code, byte to, byte left, byte right)
        {
            this.Write((byte)code, to, left, right);
        }
        
        public void Emit(OSECode6 code, byte num, int value)
        {
            this.Write((byte)code, num);
            writer.Write(BitConverter.GetBytes(value).Reverse().Select(x => x).ToArray());
        }
        
        
        
        public void EmitRem(byte remCode, params byte[] buf)
        {
            this.Write(0xFE, remCode);
            writer.Write(buf.Take(remCode).ToArray());
        }
    }
}
