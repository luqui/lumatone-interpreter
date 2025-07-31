
import numpy as np
import math
from fractions import Fraction
from itertools import product

# Parameters
edo = 31
d = 2 ** (1 / edo)  # Step size in EDO
limit = 8           # Max denominator for JI ratios
max_degree = 5      # Max exponent to try for A and B

# Generate all JI ratios a/b between 1 and 2 with b < limit
ratios = [
    Fraction(a, b) for b in range(1, limit+1)
    for a in range(b + 1, 2 * b)
    if math.gcd(a, b) == 1
]

equations = []
log_ratios = []
raw_ratios = []

for r in ratios:
    target_cents = 1200 * math.log2(float(r))
    target_steps = round(target_cents / (1200 / edo))
    best_err = float('inf')
    best_exponents = None

    # Try combinations of A^a * B^b
    for a, b in product(range(-max_degree, max_degree + 1), repeat=2):
        steps = a * 5 + b * 3
        if steps == target_steps:
            approx_log = a * math.log(d ** 5) + b * math.log(d ** 3)
            err = abs(math.log(float(r)) - approx_log)
            if err < best_err:
                best_err = err
                best_exponents = (a, b)

    if best_exponents:
        a, b = best_exponents
        equations.append((a, b))
        log_ratios.append(math.log(float(r)))
        raw_ratios.append(r)

# Solve using linear regression on log-space
X = np.array(equations)
y = np.array(log_ratios)
coeffs, _, _, _ = np.linalg.lstsq(X, y, rcond=None)
log_A, log_B = coeffs
A = math.exp(log_A)
B = math.exp(log_B)

# Print results
print(f"A ≈ {A:.6f}")
print(f"B ≈ {B:.6f}")

# Print table with error in cents
print("\n{:<10} {:>6} {:>6} {:>10} {:>10} {:>10}".format("Ratio", "A_exp", "B_exp", "Approx", "Expected", "Error (cents)"))
print("-" * 64)
for (a, b), r in zip(equations, raw_ratios):
    log_approx = a * log_A + b * log_B
    approx = math.exp(log_approx)
    expected = float(r)
    cents_error = 1200 * math.log2(approx / expected)
    print(f"{str(r):<10} {a:>6} {b:>6} {approx:>10.5f} {expected:>10.5f} {cents_error:>10.3f}")
