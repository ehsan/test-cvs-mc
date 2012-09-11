// vim: set ts=4 sw=4 tw=99 et:

function S() {
  var t = new Float32Array(1);
  var k = 0;
  var xx = 19;
  var gridRes = 64;
  for (var i = 0; i < 100; i++) {
      t[k] = -1 + 2 * xx / gridRes;
  }
  return t[0];
}
assertEq(S(), -0.40625);
