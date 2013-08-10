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
        MemoryStream ms;
        OSEGenerator ose;

        public FulynCompiler()
        {
            ms = new MemoryStream();
            ose = new OSEGenerator(ms);
            AST = new FulynProg();
        }

        /// <summary>
        /// string[] 配列で与えられたコードからASTを生成します。
        /// </summary>
        /// <param name="lines">コード行</param>
        public void Parse(string[] lines)
        {
            AST = new FulynProg() { Members = ParseBase(PreProcess(lines)).ToArray() };
        }

        /// <summary>
        /// 生成したASTからコンパイルを行います。
        /// </summary>
        /// <returns>コンパイル済バイト列</returns>
        public byte[] Compile()
        {
            throw new NotImplementedException();
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
        /// 式を計算し、結果が代入されたレジスタ番号を返す
        /// 再帰し、再帰深度が深い順にEmitされる
        /// returntoはresultレジスタの既定値を設定可能、省略で新規生成
        /// </summary>
        /// <param name="e"></param>
        /// <returns></returns>
        byte CompileExpr(IExpr e, bool isLocal, byte? returnto = null)
        {
            // 使うレジスタ管理器
            var resrpers = e.Type == typeof(IntType) ? (isLocal ? ose.LocalRegister : ose.GlobalRegister) 
                                                     : (isLocal ? ose.LocalPegister : ose.GlobalPegister);
            
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
                    CompileExpr(reg, isLocal, (reg.Type == typeof(IntType) ? ose.FuncRegister : ose.FuncPegister).Lock());
                
                // 呼び出しをEmit
                if (isPointer)
                    ose.EmitMacro(Macro.PCall, new MacroValues(return_label => ose.Labels.Lock()), lval.First(x => x.Key == call.Identity).Value);
                else
                    ose.EmitMacro(Macro.Call, new MacroValues(return_label => ose.Labels.Lock(), func_label => gval.First(x => x.Key == call.Identity).Value));

                // 結果をreturnレジスタにコピー、戻り値はR30 or P30に入ってくる
                ose.Emit(call.Type == typeof(IntType) ? OSECode3.Copy : OSECode3.PCopy, result, 30);

                // 引数レジスタはもう使用しないので、全解放
                ose.FuncRegister.Reset(); ose.FuncPegister.Reset();
            }


            // パターンマッチ
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
                    // マッチ終了
                    var endif = ose.Labels.Lock();
                    // true処理開始 : thenラベル
                    var then = ose.Labels.Lock();

                    // trueならばthenラベルに飛ぶ
                    // こういう処理なのは、右辺を評価した時に命令数が膨れ上がってもいいように
                    ose.Emit(OSECode2.Cond, expr);
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => then));

                    // false時の右辺を評価し、resultにコピー
                    var falsereg = CompileExpr(match.Cases.First(x => (x.Item1 as Variable) != null && (x.Item1 as Variable).Identity == "false").Item2, isLocal, result);
                    // マッチ終了へ
                    ose.EmitMacro(Macro.Goto, new MacroValues(to => endif));

                    // thenラベル
                    ose.Emit(OSECode6.Label, 01, then);
                    // true時の右辺を評価し、resultにコピー
                    var truereg = CompileExpr(match.Cases.First(x => (x.Item1 as Variable) != null && (x.Item1 as Variable).Identity == "true").Item2, isLocal, result);

                    // マッチ終了
                    ose.Emit(OSECode6.Label, 01, endif);
                }

                // hoge-piyo-otherwiseの時
                else
                {
                    //TODO: hoge-piyo-otherwiseの時を実装
                }

                // マッチ対象式のレジスタはもう使用しないので、解放
                (match.Expr.Type == typeof(IntType) ? (isLocal ? ose.LocalRegister : ose.GlobalRegister)
                                                    : (isLocal ? ose.LocalPegister : ose.GlobalPegister)
                ).Free(expr);
            }


            // 変数
            else if (e.GetType() == typeof(Variable))
            {
                var val = (Variable)e;
                var reg = (lval.Any(x => x.Key == val.Identity) ? lval : gval).First(x => x.Key == val.Identity).Value;
                // returnレジスタを破棄し、代わりに変数のレジスタを返す
                resrpers.Free(result);
                result = reg;
            }

            return result;
        }
    }
}