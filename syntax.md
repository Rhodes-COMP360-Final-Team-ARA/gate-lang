## SYNTAX IDEAS

# Defining Components

*full-adder*
>comp FullAdder(a:1, b:1, cin:1) 
>{
>Instantiate first HalfAdder
>ha1-sum:1, ha1-carry:1 = HalfAdder(a, b);
>    
>Instantiate the second HalfAdder using the output from the first
>sum:1, ha2-carry:1 = HalfAdder(ha1-sum, cin);
>    
>Wire the carries together
>cout:1 = ha1-carry OR ha2-carry;
>    
>return sum, cout;
>}
>

*half-adder*
> comp HalfAdder(a:1, b:1) 
> {
> sum:1 = a XOR b;
> carry:1 = a AND b;
>     
> return sum, carry;
> }
> 

# Using Base Operations

xor-result = a XOR b;

not-result = NOT a;

and-result = a AND b;

or-result = a OR b;

## other operations using base OPs

nand-result = NOT (a AND b);

nor-result = NOT (a OR b);

xnor-result = NOT (a XOR b);


# File Imports
import "filetoimport.gate"


# Comments

> //this is a comment, anything after a double slash is a comment.