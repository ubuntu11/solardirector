function perfect(n) {
  print("The perfect numbers up to " + n + " are:");
  // We build sumOfDivisors[i] to hold a string expression for
  // the sum of the divisors of i, excluding i itself.
  var sumOfDivisors = new ExprArray(n + 1, 1);
  for (var divisor = 2; divisor <= n; divisor++) {
    for (var j = divisor + divisor; j <= n; j += divisor) {
      sumOfDivisors[j] += " + " + divisor;
    }
    // At this point everything up to 'divisor' has its sumOfDivisors
    // expression calculated, so we can determine whether it's perfect
    // already by evaluating.
    if (eval(sumOfDivisors[divisor]) == divisor) {
      print("" + divisor + " = " + sumOfDivisors[divisor]);
    }
  }
  delete sumOfDivisors;
  print("That's all.");
}
