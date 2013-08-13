using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Fulyn
{
    public enum FulynErrorCode
    {
        IDNotFound,
        WrongType,
        Uncallable,
        WrongSyntax,
        Undefined
    }

    public class FulynException : Exception
    {
        public FulynErrorCode ErrorCode { get; set; }

        public FulynException(FulynErrorCode code, string message)
            : base(message)
        {
            ErrorCode = code;
        }
    }
}
