using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using OSC;
using Fulyn.AST;
using Alice.Extensions;
using Alice.Functional.Monads;
namespace Fulyn
{
    /// <summary>
    /// コンパイラのパーサ部分
    /// </summary>
    public partial class FulynCompiler
    {
        /// <summary>
        /// 抽象構文木を取得します。
        /// </summary>
        public FulynProg AST { get; private set; }

        /// <summary>
        /// 型決定用ローカル型リスト
        /// </summary>
        Dictionary<string, IType> local = new Dictionary<string, IType>();
        /// <summary>
        /// 型決定用グローバル型リスト
        /// </summary>
        Dictionary<string, IType> global = new Dictionary<string, IType>();

        /// <summary>
        /// 行継続子と行分割子の処理
        /// </summary>
        IEnumerable<string> PreProcess(string[] lines)
        {
            var hash = new HashSet<int>();
            var count = 0;
            foreach (var s in lines)
            {
                if (!hash.Contains(count))
                {
                    var ret = s;
                    var i = 1;
                    while (true)
                    {
                        if (ret.EndsWith("~") || (count < lines.Length - 1 && lines[count + i].StartsWith("|")))
                        {
                            ret = ret.Replace("~", "");
                            ret += lines[count + i];
                            hash.Add(count + i);
                            i++;
                        }
                        else break;
                    }
                    foreach (var t in ret.Split(';'))
                        yield return t;
                }
                count++;
            }
        }

        /// <summary>
        /// Memberを列挙
        /// </summary>
        IEnumerable<IMember> ParseBase(IEnumerable<string> lines)
        {
            var buf = new List<string>();
            var seek = false;
            foreach (var s in lines)
            {
                if (!seek)
                {
                    // declare
                    if (s.Contains("::"))
                    {
                        var x = s.Split(new[] { "::" }, StringSplitOptions.None);
                        var id = x[0]; var type = ParseType(x[1]);
                        global.Add(id, type);
                        yield return new Declare() { Identity = id, Type = type };
                    }

                    // lambda
                    else if (s.Contains("=>"))
                        yield return ParseFunc(new[] { s });

                    // 複文関数の場合、舐める
                    else if (s.Contains("(") || s == "main")
                        seek = true;
                }

                // 舐める処理
                if (seek)
                {
                    buf.Add(s);
                    // endに到達したらパーサにかける
                    if (s == "end")
                    {
                        seek = false;
                        yield return ParseFunc(buf.ToArray());
                        buf.Clear();
                    }
                }

            }
        }

        /// <summary>
        /// 関数パーサ
        /// </summary>
        Function ParseFunc(string[] lines)
        {
            var cache = new Dictionary<string, IType>(local);
            var ret = new Function();

            // ローカル型リストを退避
            local.Clear();

            // 複文
            if (lines.Length > 1)
            {
                var body = lines[0].SkipWhile(x => x != '(').JoinToString();
                var id = lines[0].TakeWhile(x => x != '(').JoinToString();
                var args = body.Length > 0 ? body.Remove(body.Length - 1).Remove(0, 1).Split(',').ToArray() : new string[] { };
                var type = id == "main" ? new FuncType() : (FuncType)global[id];
                if (id != "main")
                    type.ArgsType.Zip(args, Tuple.Create).ForEach(x => local.Add(x.Item2, x.Item1));
                ret = new Function()
                {
                    Identity = id,
                    Args = args,
                    Stmts = lines.Skip(1).TakeWhile(x => x != "end").Select(ParseStmt).ToArray(),
                    Type = type
                };
            }

            // lambda
            else
            {
                var body = lines[0].SkipWhile(x => x != '=').Skip(1).JoinToString().ParseIndent("(", "=>", ")");
                var id = lines[0].TakeWhile(x => x != '(').JoinToString();
                ret = new Function()
                {
                    Identity = id,
                    Args = body.Take(body.Count() - 1).ToArray(),
                    Stmts = new[] { new Subst() { Identity = Subst.Trash, Expr = ParseExpr(body.Last()) } },
                    Type = id == "main" ? new FuncType() : (FuncType)global[id]
                };
            }

            // ローカル型リストを復元
            local = cache;
            return ret;
        }

        /// <summary>
        /// 式パーサ
        /// </summary>
        IExpr ParseExpr(string text)
        {
            // 整数リテラル
            if (text.All(char.IsDigit))
                return new IntLiteral() { Value = int.Parse(text) };

            // lambda
            else if (text.Contains("=>"))
            {
                var x = text.ParseIndent("(", "=>", ")");
                return new LambdaLiteral() { Args = x.Take(x.Count() - 1).ToArray(), Expr = ParseExpr(x.Last()) };
            }

            // パターンマッチ
            else if (text.StartsWith("?"))
            {
                var x = text.ParseIndent("(", "|", ")").ToArray();

                // true,falseと()の混在なんて、そんなの私が許さない！
                if (new[] { "true", "false" }.Any(x.Contains) && x.Contains("()"))
                    throw new FulynException(FulynErrorCode.WrongSyntax, "パターンマッチにおいてtrue, falseと()の混在は許されません。");

                return new Match()
                {
                    Expr = ParseExpr(x[0].Replace("?", "")),
                    Cases = x.Skip(1).Where(y => !y.Contains("()"))
                                     .Select(y => y.Split(new[] { "->" }, StringSplitOptions.None))
                                     .Select(y => Tuple.Create(ParseExpr(y[0]), ParseExpr(y[1]))).ToArray(),
                    Otherwise = x.Where(y => y.Contains("()")).Select(y => y.Split(new[] { "->" }, StringSplitOptions.None))
                                 .Select(y => ParseExpr(y[1])).FirstOrDefault(),
                    Type = x.Skip(1).Select(y => y.Split(new[] { "->" }, StringSplitOptions.None)).Select(y => ParseExpr(y[1])).First().Type
                };
            }

            // 演算子
            else if (Optr.Operators.Keys.Any(text.Contains))
            {
                throw new NotImplementedException();
                // TODO: 演算子はプリプロセス時に処理すべきか、コンパイル時に処理するべきか
            }

            // 関数呼び出し
            else if (text.Contains("(") && !text.StartsWith("("))
            {
                var name = text.TakeWhile(x => x != '(').JoinToString();
                var body = text.SkipWhile(x => x != '(').JoinToString();
                var error = new Error<IType>(() => ((FuncType)(global.Any(x => x.Key == name) ? global : local).FirstOrDefault(x => x.Key == name).Value).ReturnType);
                return new Call()
                {
                    Identity = name,
                    Args = body.Remove(body.Length - 1)
                               .Remove(0, 1)
                               .ParseIndent("(", ",", ")")
                               .Select(ParseExpr)
                               .ToArray(),
                    Type = error.IsError ? null : error.Value
                };
            }

            // 変数
            else
            {
                var error = new Error<IType>(() => (global.Any(x => x.Key == text) ? global : local).FirstOrDefault(x => x.Key == text).Value);
                return new Variable() { Identity = text, Type = error.IsError ? null : error.Value };
            }
        }

        /// <summary>
        /// 型パーサ
        /// </summary>
        IType ParseType(string text)
        {
            // int
            if (!text.Contains("["))
                return new IntType();

            // func
            var x = text.Remove(0, 1).Remove(text.Length - 2).ParseIndent("[", "=>", "]");
            return new FuncType() { ReturnType = ParseType(x.Last()), ArgsType = x.Take(x.Count() - 1).Select(ParseType).ToArray() };
        }

        /// <summary>
        /// 文パーサ
        /// </summary>
        IStmt ParseStmt(string text)
        {
            // インラインアセンブラ
            if (text.StartsWith("inline:"))
                return new Inline() { Text = text.Replace("inline:", "") };

            // 宣言
            else if (text.Contains("::"))
            {
                var x = text.Split(new[] { "::" }, StringSplitOptions.None);
                return new Declare() { Identity = x[0], Type = ParseType(x[1]) };
            }

            // 代入
            else if (text.Contains("="))
            {
                var x = text.Split('=');
                var name = x[0]; var expr = ParseExpr(x[1]);
                if (!global.Any(y => y.Key == name) && !local.Any(y => y.Key == name))
                    local.Add(name, expr.Type);
                return new Subst() { Identity = name, Expr = expr };
            }

            // 暗黙代入
            else
            {
                var expr = ParseExpr(text);
                local[Subst.Trash] = expr.Type;
                return new Subst() { Identity = Subst.Trash, Expr = expr };
            }
        }

    }
}
