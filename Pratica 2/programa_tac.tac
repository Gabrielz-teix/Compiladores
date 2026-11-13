a = 5
b = 2
s = 0
if a > b goto L1
goto L2
L1:
t1 = a - b
s = t1
goto L3
L2:
t2 = b - a
s = t2
L3:
print s
