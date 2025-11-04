import math

# General N and C Calculator
epsilon_d = 0.000792
epsilon_a = 0.2
PNB = 137
alpha = 100
m = 256 - PNB

N = (
    (
        math.sqrt(alpha * math.log(4))
        + 3
        * math.sqrt(
            1.0
            - ((epsilon_d**4) * (epsilon_a**4))
            / ((epsilon_d**2) * (epsilon_a**2))
        )
    )
    / math.sqrt((epsilon_d**2) * (epsilon_a**2))
) ** 2

print("--------------------")
print("General N and C Calculator")
print("--------------------")
print("N =", N)
print("N log base 2 =", math.log2(N))


C = 2**m * N + 2**(256-alpha) + 2**(PNB)
print("C =", C)
print("C log base 2 =", math.log2(C))