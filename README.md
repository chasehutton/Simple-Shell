# Simple-Shell
ncsh (not complex shell) is a first attempt at a command language interpreter. The grammar it supports is defined here: 

E = T Eopt \
Eopt = "&" | ";" | ∧ \
T = P Topt \
Topt = "&&" P Topt | "||" P Topt | "&" P Topt | ";" P Topt | ∧ \
P = C Popt \
Popt = "|" C Popt | ∧ \
C = S | "(" E ")" \
S = String R \
R = ">" String | "<" String | ∧

This grammar is a subset of the BASH grammar as defined here: https://cmdse.github.io/pages/appendix/bash-grammar.html 

Using methods described in this set of notes from the University of Copenhagen: https://www.academia.edu/2801015/Grammars_and_parsing_with_C_2_0

the grammar is transformed to a more implementation friendly format.

Effectively this shell supports a simplified version of Pipelines, Lists, AND lists, OR lists, Redirection, Grouping, Simple commands, and glob patterns. 
