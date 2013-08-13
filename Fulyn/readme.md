The Fulyn Language
------------------

# About Fulyn

Fulyn（ふうりん）は、OSECPUのための関数型言語です。

Fulynは完全な高級言語で、これを用いることで強力なコーディングを可能にします。

# Grammer

Fulynのプログラムは、関数宣言と変数（プロトタイプ）宣言のみで構成されます。

syntax:

    <program> ::= ( ( <declare> | <func> ) '\n' )+

    <declare> ::= <identity> : <type> 



## Function

Fulynの関数は変数宣言と代入式で構成されます。

全ての構文が戻り値を持ち、変数に代入されるか、捨てられます。

Fulynは、main関数から開始されます。

Fulynでは、ラムダ式を用いた単文関数と、関数宣言を用いた複文関数を定義できます。

ラムダ式はリテラルですが、globalであっても代入ができます。

関数は、func型のimmutableな変数として利用でき、引数にとったり返したりできます。

引数がない場合は、省略できます。

関数宣言には、プロトタイプ宣言が必要です。mainは不要です。

プロトタイプ宣言は、変数宣言と同一です。

func型は、funcとは書かずに、[ （引数の型1 -> 引数の型2 -> ... 引数の型n =>) 戻り値の型 ] と書きます。

もし引数がない場合でも、[]で囲って[int]のように書きます。

syntax:

    <stmt> ::= ( <declare> | <subst> ) '\n'
    
    <subst> ::= ( <identity> '=' |  [ ( '_' | '$' ) '=' ] ) <expr>
    
    <func> ::= <identity> '(' <identity> { ',' <identity> } ')' '\n'
                   <stmt>+
               'end'
               |


               <identity> = <lambda>

### Return

Fulynの複文関数内から値を返すには、値を捨てます。

値を捨てるには、\_ に代入するか、式のみを書きます。（どちらも意味は同じです）

戻り値と異なる型の値も捨てられますが、無視され、返されません。

値を捨てても実行は止まらないので、breakするには、 \_ の代わりに $ を用います。

$ は \_ と参照先は同じですが、特殊な変数で、評価された瞬間にbreakされます。

以下の関数の動作は全て同じです。

example:

    func1
        _ = 0
    end

    func2
        0
    end

    func3
        $ = 0
    end

    func4
        _ = 0
        $
    end


## Statement and Expression

Fulynは、現状でint型とfunc型をサポートします。

変数宣言は、名前の直後にコロンを置いて型名を書きます。

関数の内部では、宣言は省略可能です。

グローバル変数は宣言できますが、初期化はできず、関数内からしか値を代入できず、宣言が必須です。

__命令の区切りは改行です。__複数行にわたって命令を繋げる場合は~を使います。

syntax:

	<expr> ::= <literal> | <call> | <match> | <optr> | <var>

	<type> ::= 'int' | '[' [ <type> { '->' <type> } => ] <type> ']'

### Literal Expression

リテラルとして、数字とラムダ式があります。

ラムダ式は後述します。

syntax:

    <number> ::= ( '0' | '1' | ... '9' )+
	<literal> ::= <number> | <lambda>

### Call Expression

関数呼び出しです。

func型の変数を指定出来ます。

syntax:

    <call> ::= <identity> '(' <expr> { ',' <expr> } ')'

### Match Expression

Fulynでは、if文もfor文もサポートせず、パターンマッチのみがあります。

trueとfalseというimmutableな変数が用意されており、if文のように用いることもできます。

()は必須で、マッチするものがなかった時に呼ばれます。

falseは()と同値です。

syntax:

    <not-false> ::= '|' ( 'true' | <expr>) '->' <expr> 
	<false> ::= '|' ( 'false' | '()' ) '->' <expr>
    
    <match> ::= '?' <expr> '\n'
	           { <not-false> '\n' }
			     <false>
			   { '\n' <not-false> }
	              

### Operator Expression

Fulynでは、演算子は全てマクロで、最終的に関数呼び出しに変換されます。

syntax: 

    <optr> ::= <expr> <identity> <expr>

もしかしたら演算子の宣言もサポートするかもしれません。

### Lambda Expression

ラムダ式は、関数のリテラルです。

内部に式を1つだけ含むことができ、その戻り値が返されます。

引数がない場合は、()を用います。

syntax:

    <lambda> ::= ( ( <identity> '=>' )+ | '()' '=>' ) <expr>

次の2つの構文のコンパイル結果は同じです。

    func1 = x => x * x
    
    func2 (x)
        x * x
    end

### Variable

変数です。

syntax:

	<var> ::= <identity>


## Technique

Fulynにはループがないため、再帰関数を組む必要があります。

再帰の際は引数が破壊されるため、再帰呼び出し後に引数の値を利用したい場合は前もってローカル変数に代入しておく必要があります。

逆に、最後に呼び出された再帰の引数を取得できるので、これを用いればメモリ使用量を抑えることが可能です。

また、サンプルのユークリッドの互除法のように、再帰後に引数を利用しないコードを書くことでも節約出来ます。

# Sample

## 1から10000の和

    sum : [int -> int => int]

    main
        result = sum(1, 10000)
        print(result)
    end

    sum = x => y => (x + y) * (y - x + 1) / 2

## ユークリッドの互除法

    gcd : [int -> int => int]

    main
        result = gcd(1547, 2093)
        print(result)
    end
    
    gcd (x, y)
        // x > yになるように入れ替える
        tmp = x
        x = ? x < y
            | true -> y
            | false -> x
        y = ? tmp == x
            | true -> y
            | false -> tmp 
        // 非0なら再帰
        ? x % y
        | 0 -> x
        | () -> gcd(y, x % y)
    end
