$$
\begin{align}
Expr &\to Operand \ | \ Operand \ Unary \ | \  Operand \ Binary \ Operand
\\
Operand &\to \text{ID} \ | \ \text{'('} \ Expr \ \text{')'}
\\
Unary &\to \text{'*'}
\\
Binary &\to \text{'|'} \ | \ \text{ε}
\end{align}
$$
