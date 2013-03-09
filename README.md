Required Tools
==============
* nasm
* bison
* flex
* jam
* somewhat recent g++

Todo
====
* Arrays
* Pointers
* Different int sizes
* String manipulation
* Modules
* Improve syntax error messages
* Setup tests for compilation errors

Grammar
=======

    <program> ::= { <function> }

    <function> ::= ( <type> <id> <argument_list> <block> ) | ( "extern" <type> <id> <argument_list> ";" )

    <argument_list> ::= "(" <type> <id> { "," <type> <id> } ")"

    <block> ::= "{" { <statement> } "}"

    <statement> ::= <declaration> | <assignment> | <while> | <if> | <return> | (<function_call> ";")

    <assigment> ::= <id> "=" <expr> ";"

    <declaration> ::= <type> <id> ";"

    <return> ::= "return" <expr> ";"

    <function_call> ::= <id> "(" <expr> { "," <expr> } ")"

    <while> ::= "while" "(" <expr> ")" <block>

    <if> ::= "if" "(" <expr> ")" <block> { "elif" "(" <expr> ")" <block> } [ "else" <block> ]

    <value> ::= <id> | <literal> | <function_call>

    <literal> ::= <number> | "True" | "False" | '"',{all characters - '"'},'"'

    <expr>   ::= <expr_1> [ "||"                      <expr>   ]
    <expr_1> ::= <expr_2> [ "&&"                      <expr_1> ]
    <expr_2> ::= <expr_3> [ ("==" | "!=")             <expr_2> ]
    <expr_3> ::= <expr_4> [ ("<" | "<=" | ">" | ">=") <expr_3> ]
    <expr_4> ::= <expr_5> [ ("+" | "-")               <expr_4> ]
    <expr_5> ::= <expr_6> [ ("/" | "*" | "%")         <expr_5> ]
    <expr_6> ::=          [ ("!" | "-") ]             <expr_7>
    <expr_7> ::= <value> | ( "(" <expr> ")" )

    <type> ::= "int" | "int64" | "bool" | "string"

    <id> ::= <letter_> { <letter_> | <digit> }
    <letter_> ::= A-Z | a-z | _
    <digit> ::= 0-9

    <number> ::= 1-9{ 0-9 }
