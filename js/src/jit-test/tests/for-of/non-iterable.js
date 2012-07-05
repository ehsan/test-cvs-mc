// Iterating over non-iterable values throws a TypeError.

load(libdir + "asserts.js");

var misc = [
    {}, {x: 1}, Math, isNaN,
    Object.create(null),
    Object.create(Array.prototype),
    null, undefined,
    true, 0, 3.1416, "", "ponies",
    new Boolean(true), new Number(0), new String("ponies")];

for (var i = 0; i < misc.length; i++) {
    let v = misc[i];
    var testfn = function () {
        for (var _ of v)
            throw 'FAIL';
        throw 'BAD';
    };
    assertThrowsInstanceOf(testfn, TypeError);
}
