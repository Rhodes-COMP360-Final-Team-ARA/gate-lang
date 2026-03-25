# Gate-Lang Project Proposal
**Team Members:**
Remy Bozung (team leader), Aiden Scoren, Apollo Popowitz

## AI-Policy
Our policy is have AI-Assistance that will help with planning/development and debugging. However, the main goal of using AI is to learn how and why something isn't working instead of asking it complete certain tasks.

## Language Name and Description
**Name:** Gate-Lang

**Domain:** Digital logic / hardware description

**Gate-Lang** and is a hardware description language that defines digital logic circuits using primitive boolean operations and composable components. Gate-Lang provides a simple, purpose-built language for pipelining base logic gates (AND, OR, XOR, NOT) into higher-level reusable components. 

## Example Programs:

### Adder
```
comp halfadder(a:1, b:1) {
    sum:1 = a xor b;
    carry:1 = a and b;
    return sum, carry;
}

comp fulladder(a:1, b:1, cin:1) {
    ha1_sum:1, ha1_carry:1 = halfadder(a, b);
    sum:1, ha2_carry:1 = halfadder(ha1_sum, cin);
    cout:1 = ha1_carry or ha2_carry;
    return sum, cout;
}
```

## Checkpoint 1 Commitments:

Infrastructure - Have CMake set up, create the code architecture, and setup GitHub processes
Design - Create the syntax and semantics of the DSL
Parsing - Have the grammar rules in place to create the ASTs

