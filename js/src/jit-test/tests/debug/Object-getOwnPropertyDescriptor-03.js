// obj.getOwnPropertyDescriptor works on global objects.

var g = newGlobal('new-compartment');
g.eval("var v; const k = 42;");
this.eval("var v; const k = 42;");

var dbg = Debugger();
var obj = dbg.addDebuggee(g);

function test(name) {
    var desc = obj.getOwnPropertyDescriptor(name);
    assertEq(desc instanceof Object, true);
    var expected = Object.getOwnPropertyDescriptor(this, name);
    assertEq(Object.prototype.toString.call(desc), Object.prototype.toString.call(expected));
    assertEq(desc.enumerable, expected.enumerable);
    assertEq(desc.configurable, expected.configurable);
    assertEq(desc.writable, expected.writable);
    assertEq(desc.value, expected.value);
}

test("Infinity");
test("v");
test("k");
