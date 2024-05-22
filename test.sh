#!/usr/bin/env bash

input="abcdefghijklmnopqrstuvwxyzz
ABCDEFGHIJKLMNOPQRSTUVWXYZZ
z03bbU z03a0U
z127Uz1e17Uz13dUz139Uz153Uz1e88Uz1feUz17fUz1e39Uz111U
za zb zc zd ze zg zh zi zl zm zn zp zq zr zs zt zu zv
ZL ZR ZM ZN ZC
Z0T Z3T
Z1H Z3H
Z9H"

expected="abcdefghijklmnopqrstuvwxyz
ABCDEFGHIJKLMNOPQRSTUVWXYZ
λ Π
ħḗĽĹœẈǾſḹđ
& | ^ $ = > # . < - ! + ' \ / * _ %
( ) [ ] :
() (,,)
(# #) (#,,#)
(#,,,,,,,,#)"

diff <(echo "$input" | ./main) <(echo "$expected")
