The Fulyn Language
------------------

# About Fulyn

Fulyn�i�ӂ����j�́AOSECPU�̂��߂̊֐��^����ł��B

Fulyn�͊��S�ȍ�������ŁA�����p���邱�Ƃŋ��͂ȃR�[�f�B���O���\�ɂ��܂��B

# Grammer

Fulyn�́A���̑���݂̂��T�|�[�g���܂��B

## Statement and Expression

Fulyn�̊֐��͕ϐ��錾�Ƒ�����ō\������܂��B

�S�Ă̍\�����߂�l�������A�ϐ��ɑ������邩�A�̂Ă��܂��B

Fulyn�́A�����int�^��func�^���T�|�[�g���܂��B

func�^�́Afunc�Ƃ͏������ɁA[ �i�����̌^1 -> �����̌^2 -> ... �����̌^n�j=> �߂�l�̌^ ] �Ə����܂��B

�ϐ��錾�́A���O�̒���ɃR������u���Č^���������܂��B

�֐��̓����ł́A�錾�͏ȗ��\�ł��B

�O���[�o���ϐ��͐錾�ł��܂����A�������͂ł����A�֐������炵���l�����ł����A�錾���K�{�ł��B

__���߂̋�؂�͉��s�ł��B__�����s�ɂ킽���Ė��߂��q����ꍇ��~���g���܂��B

syntax:

    <program> ::= ( ( <prototype> | <declare> | <func> ) '\n' )+

    <stmt> ::= ( <declare> | <subst> ) '\n'

	<expr> ::= ( <literal> | <call> | <match> | <optr> )

	<type> ::= 'int' | '[' <type> { '->' <type> } => <type> ']'

    <declare> ::= <identity> : <type> 

    <subst> ::= ( <identity> '=' |  [ ( '_' | '$' ) '=' ] ) <expr>

### Literal Expression

���e�����Ƃ��āA�����ƃ����_��������܂��B

�����_���͌�q���܂��B

syntax:

    <literal> ::= <number> ::= ( '0' | '1' | ... '9' )+
                  |
	              <lambda>

### Call Expression

�֐��Ăяo���ł��B

func�^�̕ϐ����w��o���܂��B

syntax:

    <call> ::= <identity> '(' <arg1> [', <arg2>, <arg3>, ...])

### Match Expression

Fulyn�ł́Aif����for�����T�|�[�g�����A�p�^�[���}�b�`�݂̂�����܂��B

true��false�Ƃ���immutable�ȕϐ����p�ӂ���Ă���Aif���̂悤�ɗp���邱�Ƃ��ł��܂��B

()�͕K�{�ŁA�}�b�`������̂��Ȃ��������ɌĂ΂�܂��B

false��()�Ɠ��l�ł��B

syntax:

    <not-false> ::= '|' ( 'true' | <expr>) '->' <expr> 
	<false> ::= '|' ( 'false' | '()' ) '->' <expr>

    <match> ::= '?' <expr> '\n'
	           { <not-false> '\n' }
			     <false>
			   { '\n' <not-false> }
	              

### Operator Expression

Fulyn�ł́A���Z�q�͑S�ă}�N���ŁA�ŏI�I�Ɋ֐��Ăяo���ɕϊ�����܂��B

syntax: 

    <optr> ::= <expr> <identity> <expr>

�����������牉�Z�q�̐錾���T�|�[�g���邩������܂���B

## Function

Fulyn�́Amain�֐�����J�n����܂��B

Fulyn�ł́A�����_����p�����P���֐��ƁA�֐��錾��p���������֐����`�ł��܂��B

�����_���̓��e�����ł����Aglobal�ł����Ă�������ł��܂��B

�֐��́Afunc�^��immutable�ȕϐ��Ƃ��ė��p�ł��A�����ɂƂ�����Ԃ�����ł��܂��B

�������Ȃ��ꍇ�́A�ȗ��ł��܂��B

�֐��錾�ɂ́A�v���g�^�C�v�錾���K�v�ł��Bmain�͕s�v�ł��B

�v���g�^�C�v�錾�́A�����ڏ�͕ϐ��錾�Ɠ���ł��B

syntax:
    
    <func> ::= <identity> '(' <identity> { ',' <identity> } ')' '\n'
			       <stmt>+
               'end'
               |
               <identity> = <lambda>

### Return

Fulyn�̕����֐�������l��Ԃ��ɂ́A�l���̂Ă܂��B

�l���̂Ă�ɂ́A\_ �ɑ�����邩�A���݂̂������܂��B�i�ǂ�����Ӗ��͓����ł��j

�߂�l�ƈقȂ�^�̒l���̂Ă��܂����A��������A�Ԃ���܂���B

�l���̂ĂĂ����s�͎~�܂�Ȃ��̂ŁAbreak����ɂ́A \_ �̑���� $ ��p���܂��B

$ �� \_ �ƎQ�Ɛ�͓����ł����A����ȕϐ��ŁA�]�����ꂽ�u�Ԃ�break����܂��B

�ȉ��̊֐��̓���͑S�ē����ł��B

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

### Lambda Expression

�����_���́A�֐��̃��e�����ł��B

�����Ɏ���1�����܂ނ��Ƃ��ł��A���̖߂�l���Ԃ���܂��B

�������Ȃ��ꍇ�́A()��p���܂��B

syntax:

    <lambda> ::= ( ( <identity> '=>' )+ | '()' '=>' ) <expr>

����2�̍\���̃R���p�C�����ʂ͓����ł��B

    func1 = x => x * x
    
    func2 (x)
        x * x
    end

### Technique

Fulyn�ɂ̓��[�v���Ȃ����߁A�ċA�֐���g�ޕK�v������܂��B

�ċA�̍ۂ͈������j�󂳂�邽�߁A�ċA�Ăяo����Ɉ����̒l�𗘗p�������ꍇ�͑O�����ă��[�J���ϐ��ɑ�����Ă����K�v������܂��B

�t�ɁA�Ō�ɌĂяo���ꂽ�ċA�̈������擾�ł���̂ŁA�����p����΃������g�p�ʂ�}���邱�Ƃ��\�ł��B

�܂��A�T���v���̃��[�N���b�h�̌ݏ��@�̂悤�ɁA�ċA��Ɉ����𗘗p���Ȃ��R�[�h���������Ƃł��ߖ�o���܂��B

# Sample

## 1����10000�̘a

    sum : [int -> int => int]

    main
        result = sum(1, 10000)
        print(result)
    end

    sum = x => y => (x + y) * (y - x + 1) / 2

## ���[�N���b�h�̌ݏ��@

    gcd : [int -> int => int]

    main
        result = gcd(1547, 2093)
        print(result)
    end
    
    gcd (x, y)
        // x > y�ɂȂ�悤�ɓ���ւ���
        tmp = x
        x = ? x < y
            | true -> y
            | false -> x
        y = ? tmp == x
            | true -> y
            | false -> tmp 
        // ��0�Ȃ�ċA
        ? x % y
        | 0 -> x
        | () -> gcd(y, x % y)
    end
