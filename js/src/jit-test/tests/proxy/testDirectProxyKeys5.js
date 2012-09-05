load(libdir + "asserts.js");

/*
 * Throw a TypeError if the trap reports a new own property on a non-extensible
 * object
 */
var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        keys: function (target) {
            return [ 'foo' ];
        }
    }));
}, TypeError);
