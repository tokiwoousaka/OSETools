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
    /// コンパイラの機械語生成部分
    /// </summary>
    public partial class FulynCompiler
    {
        Stream ms;
        OSEGenerator ose;
        int funcend;

        public FulynCompiler(string outputFile)
        {
            ms = File.Open(outputFile, FileMode.OpenOrCreate, FileAccess.Write);
            ose = new OSEGenerator(ms);
            AST = new FulynProg();
        }

        /// <summary>
        /// string[] 配列で与えられたコードからASTを生成します。
        /// </summary>
        /// <param name="lines">Code lines</param>
        public void Parse(string[] lines)
        {
            lines = lines.Where(x => !x.StartsWith("//"))
                         .Select(x => x.IndexOf("//") > 0 ? x.Remove(x.IndexOf("//")) : x)
                         .ToArray();
            AST = new FulynProg() { Members = ParseBase(PreProcess(lines)).ToArray() };
        }

        /// <summary>
        /// 生成したASTからコンパイルを行います。
        /// </summary>
        /// <returns>Compiled bytes</returns>
        public void Compile()
        {
            CompileProg(AST.Members);
            ms.Close();
        }

        /// <summary>
        /// グローバル変数レジスタ
        /// </summary>
        Dictionary<string, byte> gval = new Dictionary<string, byte>();
        /// <summary>
        /// グローバル関数ラベル
        /// </summary>
        Dictionary<string, int> gfunc = new Dictionary<string, int>();
        /// <summary>
        /// ローカル変数レジスタ
        /// </summary>
        Dictionary<string, byte> lval = new Dictionary<string, byte>();
        /// <summary>
        /// ローカル関数レジスタ
        /// </summary>
        Dictionary<string, byte> lfunc = new Dictionary<string,byte>();

        /// <summary>
        /// 式を評価し、結果が代入されたレジスタ番号を返す
        /// 再帰し、再帰深度が深い順にEmitされる
        /// returntoはresultレジスタの既定値を設定可能、省略で新規生成
        /// </summary>
        /// <param name="e">Expression</param>
        /// <returns>Result register number</returns>
        byte CompileExpr(IExpr e, bool isLocal = true, byte? returnto = null)
        {
            // _ と $ は特殊なので別処理
            if (e.GetType() == typeof(Variable))
            {
                var val = (Variable)e;
                if (val.Identity == "_" || val.Identity == "$")
                    return (byte)(returnsPointer ? 0x31 : 0x30);
            }

            // 使うレジスタ管理器
            var resrpers = e.Type.GetType() == typeof(IntType) ? (isLocal ? ose.LocalRegister : ose.GlobalRegister) 
                                                     : (isLocal ? ose.LocalPegister : ose.GlobalPegister);

            // 使うレジスタDictionary

            var resrpdic = e.Type.GetType() == typeof(IntType) ? (isLocal ? lval : gval) : lfunc;
            
            // 返すレジスタ番号
            var result = returnto == null ? resrpers.Lock() : (byte)returnto;


            // 整数リテラル
            // 再帰はリテラルで帰結する
            if (e.GetType() == typeof(IntLiteral))
                ose.Emit(OSECode6.LoadImm, result, ((IntLiteral)e).Value);


            // ラムダリテラル
            // ラムダは扱いがめんどくさそうなので、関数パーサ構築まで放置
            else if (e.GetType() == typeof(LambdaLiteral))
                throw new NotImplementedException();


            // 呼び出し
            else if (e.GetType() == typeof(Call))
            {
                var call = (Call)e;

                // Pxxの場合、CallマクロではなくPCallマクロ
                var isPointer = lfunc.Any(x => x.Key == call.Identity);

                // 引数をR30 ~ R3B, P30 ~ P3Bに代入
                foreach (var reg in call.Args)
                    // 引数の式を再帰評価し、引数レジスタに代入
                    // まだEmitしていないので深度が深いほど先にEmitされる
                    CompileExpr(reg, isLocal, (reg.Type.GetType() == typeof(IntType) ? ose.FuncRegister : ose.FuncPegister).Lock());
                
                // 呼び出しをEmit
                if (isPointer)
                    ose.EmitMacro(Macro.PCall, new MacroValues(return_label => ose.Labels.Lock()), lfunc.First(x => x.Key == call.Identity).Value);
                else
                    ose.EmitMacro(Macro.Call, new MacroValues(return_label => ose.Labels.Lock(), func_label => gfunc.First(x => x.Key == call.Identity).Value));

                // 結果をreturnレジスタにコピー、戻り値はR30 or P31に入ってくる
                ose.Emit(call.Type.GetType() == typeof(IntType) ? OSECode3.Copy : OSECode3.PCopy, result, call.Type.GetType() == typeof(IntType) ? (byte)0x30 : (byte)0x31);

                // 引数レジスタはもう使用しないので、全解放
                ose.FuncRegister.Reset(); ose.FuncPegister.Reset();
            }


            // パターンマッチ
            // ラベルをたくさんたべるよ！
            else if (e.GetType() == typeof(Match))
            {
                var match = (Match)e;

                // true-falseか、hoge-piyo-otherwiseかを調べる
                var isIf = match.Otherwise == null;

                // マッチ対象を再帰評価
                var expr = CompileExpr(match.Expr, isLocal);
               　
                // true-falseの時
                if (isIf)
                {
                    // マッチ終了ラベル
                    var endif = ose.Labels.Lock();
                    // true処理開始 : thenラベル
                    var then = ose.Labels.Lock();

                    // trueならばthenラベルに飛ぶ
                    // こういう処理なのは、右辺を評価した時に命令数が膨れ上がってもいいように
                    ose.Emit(OSECode2.Cond, expr);
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => then));

                    // false時の右辺を評価し、resultにコピー
                    CompileExpr(match.Cases.First(x => (x.Item1 as Variable) != null && (x.Item1 as Variable).Identity == "false").Item2, isLocal, result);
                    // マッチ終了へ
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => endif));

                    // thenラベル
                    ose.Emit(OSECode6.Label, 0x01, then);
                    // true時の右辺を評価し、resultにコピー
                    CompileExpr(match.Cases.First(x => (x.Item1 as Variable) != null && (x.Item1 as Variable).Identity == "true").Item2, isLocal, result);

                    // マッチ終了
                    ose.Emit(OSECode6.Label, 0x01, endif);
                }

                // hoge-piyo-otherwiseの時
                else
                {
                    // マッチ型のレジスタ管理器
                    var matchreg = match.Type.GetType() == typeof(IntType) ? ose.LocalRegister : ose.LocalPegister;
                    // マッチ終了ラベル
                    var endmatch = ose.Labels.Lock();
                    // otherwiseラベル
                    var otherwise = ose.Labels.Lock();
                    // 各ケース用ラベル
                    var cases = Enumerable.Range(0, match.Cases.Length).Select(x => ose.Labels.Lock());
                    // マッチ用テンポラリレジスタ1: 結果用
                    var temp = matchreg.Lock();
                    // マッチ用テンポラリレジスタ2: 比較用
                    var ismatch = ose.LocalRegister.Lock();

                    // 分岐部分
                    foreach (var x in match.Cases.Zip(cases, Tuple.Create))
                    {
                        // ケースの左辺を評価
                        var left = CompileExpr(x.Item1.Item1, isLocal, temp);
                        // exprとtempを比較し、ismatchに代入
                        ose.Emit(match.Type.GetType() == typeof(IntType) ? OSECode4.CompEq : OSECode4.PCompEq, ismatch, temp, expr);
                        // 評価
                        ose.Emit(OSECode2.Cond, ismatch);
                        // ケースのラベルに飛ぶ
                        ose.EmitMacro(Macro.Goto, new MacroValues(to => x.Item2));
                    }
                    // どれにもマッチしなかったらotherwiseに飛ぶ
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => otherwise));

                    // 評価部分
                    foreach (var x in match.Cases.Zip(cases, Tuple.Create))
                    {
                        // ケースのラベル
                        ose.Emit(OSECode6.Label, 0x01, x.Item2);
                        // 右辺を評価し、resultにコピー
                        CompileExpr(x.Item1.Item2, isLocal, result);
                        // マッチ終了へ
                        ose.EmitMacro(Macro.Goto, new MacroValues(to => endmatch));
                    }


                    // otherwise
                    ose.Emit(OSECode6.Label, 0x01, otherwise);
                    // otherwise時の右辺を評価し、resultにコピー
                    CompileExpr(match.Otherwise, isLocal, result);

                    // マッチ終了
                    ose.Emit(OSECode6.Label, 0x01, endmatch);

                    // テンポラリレジスタを解放
                    matchreg.Free(temp);
                    ose.LocalRegister.Free(ismatch);
                }

                // マッチ対象式のレジスタはもう使用しないので、解放
                (match.Expr.Type.GetType() == typeof(IntType) ? (isLocal ? ose.LocalRegister : ose.GlobalRegister)
                                                    : (isLocal ? ose.LocalPegister : ose.GlobalPegister)
                ).Free(expr);
            }


            // 変数
            else if (e.GetType() == typeof(Variable))
            {
                var val = (Variable)e;
                var reg = resrpdic.First(x => x.Key == val.Identity).Value;
                // returnレジスタを破棄し、代わりに変数のレジスタを返す
                resrpers.Free(result);
                result = reg;
            }

            return result;
        }

        /// <summary>
        /// 文を評価しEmitします
        /// </summary>
        /// <param name="e">Statement</param>
        void CompileStmt(IStmt e)
        {
            // インラインアセンブラ
            // {name} でレジスタ値orラベル値を挿入可能
            if (e.GetType() == typeof(Inline))
            {
                var inline = (Inline)e;
                gval.Concat(lval).Concat(lfunc).ToArray().ForEach(x => inline.Text = inline.Text.Replace("{" + x.Key + "}", x.Value.ToString()));
                gfunc.ToArray().ForEach(x => inline.Text = inline.Text.Replace("{" + x.Key + "}", BitConverter.GetBytes(x.Value).Reverse().JoinToString()));
                var parse = inline.Text.Where(char.IsNumber).JoinToString().Split(2);
                ose.Emit(parse.Select(x => byte.Parse(x, System.Globalization.NumberStyles.HexNumber)).ToArray());
            }

            // 宣言、ロックするだけ
            else if (e.GetType() == typeof(Declare))
            {
                var dec = (Declare)e;
                (dec.Type.GetType() == typeof(IntType) ? lval : lfunc).Add(dec.Identity, (dec.Type.GetType() == typeof(IntType) ? ose.LocalRegister : ose.LocalPegister).Lock());
            }

            // 代入
            else if (e.GetType() == typeof(Subst))
            {
                var sub = (Subst)e;
                var regs = sub.Expr.Type.GetType() == typeof(IntType) ? lval : lfunc;
                
                // 存在しなかった場合はロック
                if(!regs.Any(x => x.Key == sub.Identity))
                    regs.Add(sub.Identity, (sub.Expr.Type.GetType() == typeof(IntType) ? ose.LocalRegister : ose.LocalPegister).Lock());
                ose.Emit(sub.Expr.Type.GetType() == typeof(IntType) ? OSECode3.Copy : OSECode3.PCopy, regs[sub.Identity], CompileExpr(sub.Expr));

                // $だったらgoto funcend
                if (sub.Identity == "$")
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => funcend));
            }
        }

        bool returnsPointer = false;

        /// <summary>
        /// 関数をEmitします
        /// </summary>
        /// <param name="f">Function</param>
        void CompileFunc(Function f)
        {
            // 関数のラベル
            var labelnum = gfunc.First(x => x.Key == f.Identity).Value;
            funcend = ose.Labels.Lock();
            
            // 関数の始まり
            ose.EmitMacro(Macro.FuncStart, new MacroValues(label => labelnum));

            // 引数をDictionaryに加える
            if(f.Identity != "main")
                f.Args.Zip(f.Type.ArgsType, Tuple.Create).ToArray().ForEach(x =>
            {
                var areg = (x.Item2.GetType() == typeof(IntType) ? ose.FuncRegister : ose.FuncPegister).Lock();
                (x.Item2.GetType() == typeof(IntType) ? lval : lfunc).Add(x.Item1, areg);
            });

            // ポインタを返すか
            returnsPointer = f.Type.GetType() != typeof(IntType);

            // ローカルに_と$を追加
            // これで自動で戻り値が返される
            lfunc.Add("_", 0x31); lfunc.Add("$", 0x31);
            lval.Add("_", 0x30); lval.Add("$", 0x30);
            
            // 引数レジスタは破壊可能なので全部解放
            ose.FuncRegister.Reset(); ose.FuncPegister.Reset();

            // 文をコンパイル
            f.Stmts.ToArray().ForEach(x => CompileStmt(x));

            // ローカルを空にする
            ose.LocalRegister.Reset();
            ose.LocalPegister.Reset();
            lfunc.Clear(); lval.Clear();

            // 関数の終わり
            ose.Emit(OSECode6.Label, 0x01, funcend);
            ose.EmitMacro(Macro.FuncEnd);
        }

        /// <summary>
        /// IMember[] 列をEmitします
        /// </summary>
        /// <param name="ms">Members</param>
        void CompileProg(IMember[] ms)
        {
            foreach (var m in ms)
            {
                // 関数
                if (m.GetType() == typeof(Function))
                    CompileFunc((Function)m);

                // グローバル変数
                else if (m.GetType() == typeof(Declare))
                {
                    var dec = (Declare)m;
                    if (dec.Type.GetType() == typeof(IntType))
                        gval.Add(dec.Identity, ose.GlobalRegister.Lock());
                    else
                        gfunc.Add(dec.Identity, ose.Labels.Lock());
                }
            }
        }
    }
}